cmake_minimum_required(VERSION 3.15)
project(RateLimiterProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# ---------------------------------------------------------
# 1. External Dependencies (JSON, Asio, Crow, spdlog)
# ---------------------------------------------------------

# JSON
FetchContent_Declare(
  json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(json)

# Asio
FetchContent_Declare(
  asio
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
  GIT_TAG asio-1-30-2
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(asio)

# Manually set the path Crow needs to find Asio
set(ASIO_INCLUDE_DIR ${asio_SOURCE_DIR}/asio/include CACHE PATH "Path to Asio headers")

# Crow
FetchContent_Declare(
    Crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG v1.2.0  
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(Crow)

# spdlog (New addition)
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.12.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(spdlog)

# ---------------------------------------------------------
# 2. Testing Setup (GoogleTest via FetchContent)
# ---------------------------------------------------------
enable_testing()

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

find_package(Threads REQUIRED)

# ---------------------------------------------------------
# 3. Build the Core Logic (The "limiter_core" library)
# ---------------------------------------------------------
add_library(limiter_core 
    src/rate_limit_manager.cpp
)

target_include_directories(limiter_core PUBLIC include)

# Linking spdlog as PUBLIC so the app and tests inherit it
target_link_libraries(limiter_core PUBLIC 
    Threads::Threads 
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

# ---------------------------------------------------------
# 4. Build the Main Server Executable
# ---------------------------------------------------------
add_executable(limiter_app src/main.cpp)

target_link_libraries(limiter_app PRIVATE 
    limiter_core
    Crow::Crow
)

# ---------------------------------------------------------
# 5. Add Tests Subdirectory
# ---------------------------------------------------------
add_subdirectory(tests)