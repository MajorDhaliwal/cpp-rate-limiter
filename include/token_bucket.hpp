#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>

struct TokenBucket {
    double current_tokens = -1.0;

    // The "Paper Trail" for the Janitor
    std::chrono::steady_clock::time_point last_refill_time;
    std::chrono::steady_clock::time_point last_access;

    // Default constructor (Needed for map's operator[])
    TokenBucket() {
        auto now = std::chrono::steady_clock::now();
        last_refill_time = now;
        last_access = now;
    }

    bool allow(double max_t, double refill_r, double requested) {
        auto now = std::chrono::steady_clock::now();
        last_access = now; // Keep the Janitor informed

        // 1. Initialize if brand new
        if (current_tokens < 0) {
            current_tokens = max_t;
            last_refill_time = now; // Reset refill timer to NOW for a fresh start
                                    // No need to run refill logic on the very first hit
        } else {
            // 2. Refill logic (Only runs for returning users)
            std::chrono::duration<double> elapsed = now - last_refill_time;
            double tokens_to_add = elapsed.count() * refill_r;

            if (tokens_to_add > 0) {
                current_tokens = std::min(max_t, current_tokens + tokens_to_add);
                last_refill_time = now;
            }
        }

        // Double check we aren't overflowing if config changed mid-run
        if (current_tokens > max_t)
            current_tokens = max_t;

        // 3. Consumption
        if (current_tokens >= requested) {
            current_tokens -= requested;
            return true;
        }

        return false;
    }

    int get_tokens_remaining() const {
        return static_cast<int>(std::max(0.0, std::floor(current_tokens)));
    }

    double seconds_until_next(double refill_rate, double cost) const {
        if (current_tokens >= cost)
            return 0.0;
        return (cost - current_tokens) / refill_rate;
    }
};