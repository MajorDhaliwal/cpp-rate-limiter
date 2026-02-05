#include "rate_limit_manager.hpp"
#include <algorithm>
#include <cmath>

RateLimitManager::RateLimitManager(const Config &config) : cfg_(config), running_(true) {

    max_ips_per_shard_ = cfg_.max_ips / cfg_.shards;
    if (max_ips_per_shard_ < 1) {
        max_ips_per_shard_ = 1;
    }

    for (int i = 0; i < cfg_.shards; ++i) {
        auto shard = std::make_unique<Shard>();
        shard->buckets.reserve(max_ips_per_shard_);
        shards_.push_back(std::move(shard));
    }

    janitor_thread_ = std::thread(&RateLimitManager::run_janitor, this);
}

RateLimitManager::~RateLimitManager() {
    {
        std::lock_guard<std::mutex> lock(janitor_mtx_);
        running_ = false;
    }
    cv_.notify_all();
    if (janitor_thread_.joinable()) {
        janitor_thread_.join();
    }
}

size_t RateLimitManager::get_shard_index(const std::string &key) {
    return std::hash<std::string>{}(key) % cfg_.shards;
}

size_t RateLimitManager::get_shard_size(size_t index) {
    if (index >= shards_.size()) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(shards_[index]->mtx);
    return shards_[index]->buckets.size();
}

bool RateLimitManager::is_allowed(const std::string &key, int &remaining, int &wait_time,
                                  int &reset_after) {
    auto &shard = shards_[get_shard_index(key)];
    std::lock_guard<std::mutex> lock(shard->mtx);

    auto now = std::chrono::steady_clock::now();
    auto &bucket = shard->buckets[key];

    // Refill logic
    auto elapsed = std::chrono::duration<double>(now - bucket.last_update).count();
    bucket.tokens = std::min(cfg_.max_tokens, bucket.tokens + elapsed * cfg_.refill_rate);
    bucket.last_update = now;
    bucket.last_access = now;

    // Reset calculation: Time to reach max_tokens
    double missing = cfg_.max_tokens - bucket.tokens;
    reset_after = static_cast<int>(std::ceil(missing / cfg_.refill_rate));

    if (bucket.tokens >= cfg_.token_cost) {
        bucket.tokens -= cfg_.token_cost;
        remaining = static_cast<int>(std::floor(bucket.tokens));
        wait_time = 0;
        return true;
    }

    // Denied logic: wait_time is time until 1 full token is available
    remaining = 0;
    wait_time = static_cast<int>(std::ceil((cfg_.token_cost - bucket.tokens) / cfg_.refill_rate));
    return false;
}

void RateLimitManager::run_janitor() {
    while (true) {
        std::unique_lock<std::mutex> lock(janitor_mtx_);
        if (cv_.wait_for(lock, cfg_.janitor_interval, [this] { return !running_; })) {
            break; // Shutdown signal received
        }

        auto now = std::chrono::steady_clock::now();

        for (auto &shard : shards_) {
            std::lock_guard<std::mutex> shard_lock(shard->mtx);
            auto it = shard->buckets.begin();
            while (it != shard->buckets.end()) {
                if (now - it->second.last_access > cfg_.expiry_timeout) {
                    it = shard->buckets.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
}