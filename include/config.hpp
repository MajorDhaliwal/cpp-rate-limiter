#pragma once

#include <chrono>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

// ANSI Color Codes for terminal feedback
#define ANSI_COLOR_RED "\033[31m"
#define ANSI_COLOR_GREEN "\033[32m"
#define ANSI_COLOR_YELLOW "\033[33m"
#define ANSI_COLOR_RESET "\033[0m"

struct Config {
    // --- General Settings ---
    int max_ips = 1000000;
    int shards = 16;

    // --- Token Bucket Settings ---
    double max_tokens = 100.0;
    double refill_rate = 10.0;
    double token_cost = 1.0;

    // --- Janitor & Cleanup Settings ---
    // Using milliseconds internally to allow high-precision unit testing
    std::chrono::milliseconds expiry_timeout = std::chrono::minutes(10);
    std::chrono::milliseconds janitor_interval = std::chrono::seconds(60);

    // Schema hint for error reporting
    inline static const std::string SCHEMA_HINT = R"(
{
  "max_ips": 1000000,
  "shards": 16,
  "token_bucket": {
    "max_tokens": 100.0,
    "refill_rate": 10.0,
    "token_cost": 1.0,
    "expiry_seconds": 600,
    "janitor_interval_seconds": 60
  }
}
)";

    /**
     * Loads configuration from a JSON file.
     * Falls back to defaults if file is missing or malformed.
     */
    static Config load_from_file(const std::string &path) {
        Config cfg;
        std::ifstream file(path);

        if (!file.is_open()) {
            std::cerr << ANSI_COLOR_YELLOW << "WARN: Config file not found (" << path
                      << "). Using defaults." << ANSI_COLOR_RESET << std::endl;
            return cfg;
        }

        try {
            nlohmann::json j;
            file >> j;

            // Top-level settings
            cfg.max_ips = j.value("max_ips", cfg.max_ips);
            cfg.shards = j.value("shards", cfg.shards);

            // Nested Token Bucket settings
            if (j.contains("token_bucket")) {
                auto &tb = j["token_bucket"];
                cfg.max_tokens = tb.value("max_tokens", cfg.max_tokens);
                cfg.refill_rate = tb.value("refill_rate", cfg.refill_rate);
                cfg.token_cost = tb.value("token_cost", cfg.token_cost);

                // Load Timeouts (Reading as int seconds, converting to ms)
                if (tb.contains("expiry_seconds")) {
                    cfg.expiry_timeout = std::chrono::seconds(tb["expiry_seconds"].get<int>());
                }
                if (tb.contains("janitor_interval_seconds")) {
                    cfg.janitor_interval =
                        std::chrono::seconds(tb["janitor_interval_seconds"].get<int>());
                }
            }

            std::cout << ANSI_COLOR_GREEN << "SUCCESS: Configuration loaded from " << path
                      << ANSI_COLOR_RESET << std::endl;

        } catch (const std::exception &e) {
            std::cerr << ANSI_COLOR_RED << "ERROR: JSON Parsing failed! " << e.what()
                      << ANSI_COLOR_RESET << std::endl;
            std::cerr << ANSI_COLOR_YELLOW << "Expected Schema:\n"
                      << SCHEMA_HINT << ANSI_COLOR_RESET << std::endl;
            std::cerr << "Falling back to default values..." << std::endl;
        }

        return cfg;
    }
};