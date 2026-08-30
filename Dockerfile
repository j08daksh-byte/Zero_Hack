# ==========================================
# STAGE 1: BUILD ENVIRONMENT
# ==========================================
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Install standard C++ Build Tools (GCC, CMake, Make) - ZERO third-party libraries
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy CMake configuration, source code, and tests
COPY CMakeLists.txt .
COPY src/ ./src/
COPY novacpp/ ./novacpp/
COPY render/ ./render/
COPY tests/ ./tests/

# Build NovaCPP binary and test suite in Release mode
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build --config Release

# Run automated zero-dependency test suite during build
RUN ./build/nova_tests

# ==========================================
# STAGE 2: PRODUCTION RUNTIME ENVIRONMENT
# ==========================================
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

WORKDIR /app

# Copy compiled Linux binary and test runner
COPY --from=builder /app/build/NovaCPP /app/NovaCPP
COPY --from=builder /app/build/nova_tests /app/nova_tests

# Copy static frontend assets
COPY render/ ./render/

# Expose default HTTP port
EXPOSE 8080

# Launch zero-dependency C++ HTTP application
CMD ["./NovaCPP"]
