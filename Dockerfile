# =======================================================
# Stage 1: Build Frontend (Vite + React)
# =======================================================
FROM node:20-slim AS frontend-builder
WORKDIR /app/frontend
COPY frontend/package*.json ./
RUN npm ci
COPY frontend/ ./
RUN npm run build

# =======================================================
# Stage 2: Build C++ Backend Engine
# =======================================================
FROM debian:bookworm-slim AS backend-builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    git \
    curl \
    python3 \
    flex \
    bison \
    zip \
    unzip \
    tar \
    pkg-config \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

RUN git clone --depth 1 https://github.com/microsoft/vcpkg.git /opt/vcpkg \
    && /opt/vcpkg/bootstrap-vcpkg.sh -disableMetrics

WORKDIR /app

# Install manifest dependencies before copying source for better layer caching.
COPY vcpkg.json ./
RUN /opt/vcpkg/vcpkg install --triplet x64-linux

# Copy source and CMake files
COPY CMakeLists.txt ./
COPY schemas/ ./schemas/
COPY src/ ./src/

# Compile with CMake and Ninja
RUN cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_TARGET_TRIPLET=x64-linux
RUN ninja -C build

# =======================================================
# Stage 3: Minimal Production Runtime
# =======================================================
FROM debian:bookworm-slim AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    zlib1g \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy binary and static assets
COPY --from=backend-builder /app/build/journeyio ./journeyio
COPY --from=frontend-builder /app/frontend/dist ./frontend/dist
COPY --from=backend-builder /app/build/vcpkg_installed/x64-linux/lib ./lib

ENV PORT=8080
ENV LD_LIBRARY_PATH=/app/lib
EXPOSE 8080

CMD ["sh", "-c", "./journeyio --data /app/player_data --static /app/frontend/dist --port ${PORT:-8080}"]