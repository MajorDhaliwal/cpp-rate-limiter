#include "rate_limit_manager.hpp"
#include <chrono>
#include <future>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

// A helper to create a standard test config
Config get_test_cfg() {
    Config cfg;
    cfg.max_tokens = 3.0;
    cfg.refill_rate = 10.0;
    cfg.token_cost = 1.0;
    cfg.shards = 4;
    cfg.max_ips = 100;
    cfg.expiry_timeout = std::chrono::milliseconds(500);
    cfg.janitor_interval = std::chrono::milliseconds(10);
    return cfg;
}

// TEST 1: New users should be allowed and see the correct remaining count
TEST(RateLimiterTest, NewUserStartsFull) {
    RateLimitManager manager(get_test_cfg());
    int remaining;
    int wait_time;
    int reset_after;

    bool allowed = manager.is_allowed("192.168.1.1", remaining, wait_time, reset_after);

    EXPECT_TRUE(allowed);
    EXPECT_EQ(remaining, 2); // 3 total - 1 consumed = 2
    EXPECT_EQ(wait_time, 0);
}

// TEST 2: Users should be blocked once they hit the limit
TEST(RateLimiterTest, BlockedWhenEmpty) {
    RateLimitManager manager(get_test_cfg()); // max 3 tokens, refill 10/sec
    int rem;
    int wait;
    int reset;

    // Use up all tokens
    for (int i = 0; i < 3; ++i) {
        manager.is_allowed("1.1.1.1", rem, wait, reset);
    }

    // This 4th call happens microseconds later
    bool allowed = manager.is_allowed("1.1.1.1", rem, wait, reset);

    EXPECT_FALSE(allowed);
    EXPECT_EQ(rem, 0);

    // Math: If refill is 10/sec, and we need 1 token, wait should be around 0.1s
    // Since we ceil(), it should be 1.
    EXPECT_GE(wait, 1);
}

// TEST 3: Tokens should refill over time
TEST(RateLimiterTest, RecoversOverTime) {
    Config cfg = get_test_cfg();
    cfg.max_tokens = 1.0;
    cfg.refill_rate = 100.0; // Refill very fast
    RateLimitManager manager(cfg);
    int rem;
    int wait;
    int reset;

    // Use the only token
    manager.is_allowed("2.2.2.2", rem, wait, reset);
    EXPECT_FALSE(manager.is_allowed("2.2.2.2", rem, wait, reset));

    // Sleep for 20ms—enough to refill at a rate of 100/sec
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    EXPECT_TRUE(manager.is_allowed("2.2.2.2", rem, wait, reset));
}

TEST(RateLimiterTest, ConcurrentAccess) {
    Config cfg = get_test_cfg();
    cfg.max_tokens = 100.0;
    cfg.token_cost = 1.0;
    RateLimitManager manager(cfg);

    const int num_threads = 10;
    const int requests_per_thread = 5;

    auto hammer = [&]() {
        int rem;
        int wait;
        int reset;
        int successful = 0;
        for (int i = 0; i < requests_per_thread; ++i) {
            if (manager.is_allowed("thread_user", rem, wait, reset)) {
                successful++;
            }
        }
        return successful;
    };

    std::vector<std::future<int>> results;
    for (int i = 0; i < num_threads; ++i) {
        results.push_back(std::async(std::launch::async, hammer));
    }

    int total_allowed = 0;
    for (auto &res : results) {
        total_allowed += res.get();
    }

    // We sent 50 requests, and we had 100 tokens. All should pass.
    EXPECT_EQ(total_allowed, 50);
}

TEST(RateLimiterTest, BurstHandling) {
    Config cfg = get_test_cfg();
    cfg.max_tokens = 5.0;
    RateLimitManager manager(cfg);
    int rem;
    int wait;
    int reset;

    // Use 5 tokens in rapid succession
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(manager.is_allowed("burst_user", rem, wait, reset));
    }

    // 6th one must fail
    EXPECT_FALSE(manager.is_allowed("burst_user", rem, wait, reset));
}

TEST(RateLimiterTest, UserIsolation) {
    RateLimitManager manager(get_test_cfg());
    int rem;
    int wait;
    int reset;

    // Drain User A
    for (int i = 0; i < 3; ++i) {
        manager.is_allowed("User_A", rem, wait, reset);
    }
    EXPECT_FALSE(manager.is_allowed("User_A", rem, wait, reset));

    // User B should still be totally fine
    EXPECT_TRUE(manager.is_allowed("User_B", rem, wait, reset));
    EXPECT_EQ(rem, 2);
}

TEST(RateLimiterTest, ShardDistribution) {
    Config cfg;
    cfg.shards = 4;
    cfg.max_ips = 1000;
    RateLimitManager manager(cfg);
    int rem;
    int wait;
    int reset;

    // 1. Fill with 100 unique IPs
    for (int i = 0; i < 100; ++i) {
        std::string ip = "192.168.1." + std::to_string(i);
        manager.is_allowed(ip, rem, wait, reset);
    }

    // 2. Check the balance
    size_t total_count = 0;
    for (int i = 0; i < cfg.shards; ++i) {
        size_t shard_size = manager.get_shard_size(i);
        total_count += shard_size;
        EXPECT_GT(shard_size, 0) << "Shard " << i << " is empty!";
    }

    EXPECT_EQ(total_count, 100);
}

TEST(RateLimiterTest, JanitorTest) {
    Config cfg;
    cfg.max_tokens = 10.0;
    cfg.refill_rate = 1.0;
    cfg.token_cost = 1.0;
    cfg.expiry_timeout = std::chrono::milliseconds(10);
    cfg.janitor_interval = std::chrono::milliseconds(1);

    RateLimitManager manager(cfg);
    int rem;
    int wait;
    int reset;

    manager.is_allowed("192.168.1.1", rem, wait, reset);

    // Give the Janitor thread a tiny window to work
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // If it worked, the user was erased and recreated fresh
    manager.is_allowed("192.168.1.1", rem, wait, reset);
    EXPECT_EQ(rem, 9);
}

TEST(RateLimiterTest, ResetTimeCalculation) {
    Config cfg;
    cfg.max_tokens = 10.0;
    cfg.refill_rate = 1.0; // 1 token per second
    RateLimitManager manager(cfg);
    int rem;
    int wait;
    int reset;

    // Use 5 tokens. We need 5 more to hit max (10).
    // At 1 token/sec, reset_after should be 5 seconds.
    for (int i = 0; i < 5; ++i) {
        manager.is_allowed("reset_user", rem, wait, reset);
    }

    EXPECT_EQ(reset, 5);
}