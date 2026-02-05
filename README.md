# C++ High-Performance Rate Limiter

[![C++ CI](https://github.com/MajorDhaliwal/cpp-rate-limiter/actions/workflows/ci.yml/badge.svg)](https://github.com/MajorDhaliwal/cpp-rate-limiter/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A robust, thread-safe rate limiting library implemented in C++. This project features a **Sharded Token Bucket** algorithm designed for high-concurrency environments, minimizing lock contention while ensuring accurate request throttling.

---

## Requirements

Before building the project, ensure you have the following installed:

* **Compiler**: GCC 9+ or Clang 10+ (Requires C++17 support)
* **Build System**: [CMake](https://cmake.org/download/) 3.10 or higher
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
docker build -t cpp-rate-limiter
```
