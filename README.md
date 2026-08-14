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

You can either build and run natively or use Docker. Both run the same server (listening on port `18080`); with Docker it is exposed on port `8080` via `-p 8080:18080`.

### Option 1: Docker (Recommended)

**Build the image:**

```bash
docker build -t cpp-rate-limiter .
```

**Run the container:**

```bash
docker run -d \
  --name rate-limiter-service \
  -p 8080:18080 \
  cpp-rate-limiter
```

**Verify it's running:**

```bash
curl http://localhost:8080/
docker logs rate-limiter-service
```

### Option 2: Build Natively

A `build.sh` script is provided to automate the configuration and compilation process.

**Make the script executable and build:**

```bash
chmod +x build.sh
sh build.sh
```

**Run the server:**

```bash
./build/limiter_app
```

**Verify it's running:**

```bash
curl http://localhost:18080/
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

