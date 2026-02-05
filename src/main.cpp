#include "crow.h"
#include "rate_limit_manager.hpp"
#include "rate_limit_middleware.hpp"
#include "spdlog/async.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include <iostream>
#include <memory>

void init_logging() {
    try {
        // Initialize thread pool for async logging: 8192 queue size, 1 worker
        // thread
        spdlog::init_thread_pool(8192, 1);

        // Sink 1: Color Console (Useful for debugging)
        auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

        // Sink 2: Rotating File (Max 5MB per file, max 3 files)
        // This prevents the disk from filling up during a stress test
        auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            "logs/limiter.log", 1024 * 1024 * 5, 3);

        std::vector<spdlog::sink_ptr> sinks{stdout_sink, rotating_sink};

        auto logger = std::make_shared<spdlog::async_logger>(
            "limiter", sinks.begin(), sinks.end(), spdlog::thread_pool(),
            spdlog::async_overflow_policy::overrun_oldest);

        spdlog::set_default_logger(logger);

        // Use 'info' for production, 'warn' or 'off' for benchmarking
        spdlog::set_level(spdlog::level::info);
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t] %v");

    } catch (const spdlog::spdlog_ex &ex) {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
    }
}

int main() {
    // 1. Initialize High-Speed Async Logging
    init_logging();
    spdlog::info("Initializing Rate Limiter Service...");

    // 2. Load Config (Assumes your Config struct has these defaults or a loader)
    Config cfg = Config::load_from_file("../config.json");

    // 3. Initialize Manager
    auto manager = std::make_shared<RateLimitManager>(cfg);

    // 4. Define App with Middleware
    crow::App<RateLimitMiddleware> app;

    // 5. Pass manager to middleware instance
    app.get_middleware<RateLimitMiddleware>().set_manager(manager);

    // 6. Routes
    CROW_ROUTE(app, "/")
    ([](const crow::request &req) {
        return crow::response("Access Granted for " + req.remote_ip_address + "\n");
    });

    CROW_ROUTE(app, "/api/data")
    ([]() { return "Protected data endpoint"; });

    spdlog::info("Rate Limiter listening on port 18080...");

    // Set Crow to use multiple threads to match your CPU cores
    app.port(18080).multithreaded().run();
}
