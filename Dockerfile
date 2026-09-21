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
    zip \
    unzip \
    tar \
    pkg-config \
    ca-certificates \
    libarrow-dev \
    libparquet-dev \
    nlohmann-json3-dev \
    libflatbuffers-dev \
    flatbuffers-compiler \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy source and CMake files
COPY CMakeLists.txt ./
COPY schemas/ ./schemas/
COPY src/ ./src/

# Compile with CMake and Ninja
RUN cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
RUN ninja -C build

# =======================================================
# Stage 3: Minimal Production Runtime
# =======================================================
FROM debian:bookworm-slim AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    libarrow1100 \
    libparquet1100 \
    libflatbuffers2 \
    zlib1g \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy binary and static assets
COPY --from=backend-builder /app/build/journeyio ./journeyio
COPY --from=frontend-builder /app/frontend/dist ./frontend/dist

ENV PORT=8080
EXPOSE 8080

CMD ["sh", "-c", "./journeyio --data /app/player_data --static /app/frontend/dist --port ${PORT:-8080}"]