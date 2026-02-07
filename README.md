# C++ Rate Limiter

[![C++ CI](https://github.com/MajorDhaliwal/cpp-rate-limiter/actions/workflows/ci.yml/badge.svg)](https://github.com/MajorDhaliwal/cpp-rate-limiter/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A robust, thread-safe rate limiting library implemented in C++. This project features a **Sharded Token Bucket** algorithm designed for high-concurrency environments, minimizing lock contention while ensuring accurate request throttling.

---

## Requirements

Before building the project, ensure you have the following installed:

* **Compiler**: GCC 9+ or Clang 10+ (Requires C++17 support)
* **Build System**: [CMake](https://cmake.org/download/) 3.15 or higher
* **Linting Tools**: `clang-format` and `clang-tidy`
* **Dependencies**: 
  * [Crow](https://crowcpp.org/) (included via CMake FetchContent)
  * [spdlog](https://github.com/gabime/spdlog) (included via CMake FetchContent)

---

## Building the Project

A `build.sh` script is provided to automate the configuration and compilation process.

### Using the Build Script
**Make the script executable**:
   ```bash
   chmod +x build.sh
```

### Docker Setup

Build the Image

```bash
docker build -t cpp-rate-limiter .
```
```bash
docker run -d \
  --name rate-limiter-service \
  -p 8080:18080 \
  cpp-rate-limiter
```
# Performance

The C++ rate limiter was benchmarked using `wrk` against a **1-GB Azure VM with 2 GB swap**.  
All tests were executed for **30 seconds** using increasing concurrency levels to observe scaling behavior and saturation points.

## Test Environment

- Cloud instance: Azure VM (1 GB RAM + 2 GB swap)
- Benchmark tool: `wrk`
- Duration per test: 30 seconds

## Benchmark Results

| Threads | Connections | Requests/sec | Avg Latency | Failures |
|--------|-------------|--------------|-------------|----------|
| 2 | 50  | 306 req/s  | 162 ms | 0 |
| 4 | 100 | 612 req/s  | 162 ms | 3 |
| 4 | 200 | **1205 req/s** | 165 ms | 18 |
| 8 | 300 | 1772 req/s | 166 ms | 19,943 |

## Key Observations

**Linear Scaling up to Saturation**
- Throughput increased nearly linearly from 50 to 200 connections.
- Latency remained stable (~162–165 ms) across these loads.

**Stable Operating Point**
- The system sustained **~1200 requests/sec** with negligible error rates.
- This represents the practical maximum throughput for this instance size.

**Overload Behavior**
- At 300 concurrent connections:
  - Throughput peaked at ~1770 req/sec.
  - Failure rate rose to ~37%.
- This indicates the saturation point of the instance and/or rate limiter.

## Summary

- **Stable throughput:** ~1200 requests/sec  
- **Peak throughput:** ~1770 req/sec (with high rejection rate)  
- **Saturation threshold:** ~250–300 concurrent connections  
- **Latency:** stable across load levels until saturation  

## Notes

- Swap allows the build and runtime to complete on a low-RAM VM, but performance is primarily limited by physical RAM (1 GB).  
- For higher throughput or more concurrent connections, upgrading the VM to more physical RAM is recommended.

