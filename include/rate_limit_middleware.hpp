#pragma once

#include "crow.h"
#include "rate_limit_manager.hpp"
#include "spdlog/spdlog.h"
#include <string>
#include <vector>

struct RateLimitMiddleware {
    struct context {
        int remaining;
        int limit;
        int reset;
    };

    std::shared_ptr<RateLimitManager> manager;

    // Fast cache for integer headers to avoid std::to_string heap churn
    inline static const std::vector<std::string> STR_CACHE = [] {
        std::vector<std::string> v;
        for (int i = 0; i <= 200; ++i)
            v.push_back(std::to_string(i));
        return v;
    }();

    const std::string &safe_str(int val) {
        if (val <= 0)
            return STR_CACHE[0];
        if (val > 200)
            return STR_CACHE[200];
        return STR_CACHE[val];
    }

    void set_manager(std::shared_ptr<RateLimitManager> m) {
        manager = m;
    }

    void before_handle(crow::request &req, crow::response &res, context &ctx) {
        std::string ip = req.get_header_value("X-Forwarded-For");
        if (ip.empty())
            ip = req.remote_ip_address;

        int rem = 0, wait = 0, reset = 0;
        if (!manager->is_allowed(ip, rem, wait, reset)) {
            res.code = 429;

            res.set_header("X-RateLimit-Limit",
                           safe_str(static_cast<int>(manager->get_config().max_tokens)));
            res.set_header("X-RateLimit-Remaining", "0");
            res.set_header("X-RateLimit-Reset", safe_str(reset));
            res.set_header("Retry-After", safe_str(wait));

            res.end();
            return;
        }

        ctx.remaining = rem;
        ctx.limit = static_cast<int>(manager->get_config().max_tokens);
        ctx.reset = reset;
    }

    void after_handle(crow::request &req, crow::response &res, context &ctx) {
        if (res.code != 429) {
            res.set_header("X-RateLimit-Limit", safe_str(ctx.limit));
            res.set_header("X-RateLimit-Remaining", safe_str(ctx.remaining));
            res.set_header("X-RateLimit-Reset", safe_str(ctx.reset));
        }
    }
};