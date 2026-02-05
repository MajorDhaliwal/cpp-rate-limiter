# --- Stage 1: Build ---
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Install system dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    wget \
    git \
    && rm -rf /var/lib/apt/lists/*

# Install specific CMake version 3.15.5
RUN wget https://github.com/Kitware/CMake/releases/download/v3.15.5/cmake-3.15.5-Linux-x86_64.sh \
    -q -O /tmp/cmake-install.sh \
    && chmod +x /tmp/cmake-install.sh \
    && /tmp/cmake-install.sh --prefix=/usr/local --skip-license \
    && rm /tmp/cmake-install.sh

WORKDIR /app

# Copy source code
COPY . .

# Run the build script
RUN chmod +x build.sh && ./build.sh

# --- Stage 2: Runtime ---
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libstdc++6 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# We use /app as the base
WORKDIR /app

# 1. Put the binary in a subfolder called 'bin'
# 2. Put the config in the parent folder '/app'
COPY --from=builder /app/build/limiter_app ./bin/limiter_app
COPY config.json .

# Set the working directory to the 'bin' folder so the app runs from there
WORKDIR /app/bin

#Crow uses port 18080. This can be changed in main.cpp
EXPOSE 18080

# Now when the app looks for ../config.json, it finds /app/config.json
CMD ["./limiter_app"]
