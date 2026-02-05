#pragma once
#include "config.hpp"
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

struct TokenBucket {
    double tokens;
    std::chrono::steady_clock::time_point last_update;
    std::chrono::steady_clock::time_point last_access;
};

// Aligned to 64 bytes to prevent false sharing
struct alignas(64) Shard {
    std::unordered_map<std::string, TokenBucket> buckets;
    std::mutex mtx;
};

class RateLimitManager {
  public:
    RateLimitManager(const Config &config);
    ~RateLimitManager();

    bool is_allowed(const std::string &key, int &remaining, int &wait_time, int &reset_after);

    // For testing and internal indexing
    size_t get_shard_index(const std::string &key);
    size_t get_shard_size(size_t index);
    const Config &get_config() const {
        return cfg_;
    }

  private:
    Config cfg_;
    std::vector<std::unique_ptr<Shard>> shards_;
    int max_ips_per_shard_;

    // Janitor synchronization
    bool running_;
    std::thread janitor_thread_;
    std::mutex janitor_mtx_;
    std::condition_variable cv_;

    void run_janitor();
};
