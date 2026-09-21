# 🎮 LILA BLACK — Player Journey Visualization Tool

> A high-performance web telemetry visualizer built for **LILA Games Level Designers** to analyze player pathing, combat choke points, storm fatalities, and loot distribution across all maps in **LILA BLACK**.

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![React](https://img.shields.io/badge/React-18-61dafb.svg)](https://reactjs.org/)
[![Vite](https://img.shields.io/badge/Vite-6-646cff.svg)](https://vitejs.dev/)
[![Leaflet](https://img.shields.io/badge/Leaflet-1.9-green.svg)](https://leafletjs.com/)
[![Apache Arrow](https://img.shields.io/badge/Apache-Arrow-orange.svg)](https://arrow.apache.org/)

---

## 📌 Table of Contents
- [Overview](#-overview)
- [Key Features for Level Designers](#-key-features-for-level-designers)
- [Tech Stack](#-tech-stack)
- [Quick Start](#-quick-start)
- [Configuration & CLI Options](#-configuration--cli-options)
- [Coordinate Transformation System](#-coordinate-transformation-system)
- [Deployment Guide](#-deployment-guide)
- [Associated Documentation](#-associated-documentation)

---

## 🌟 Overview

**LILA BLACK** is an extraction shooter where players explore maps, loot items, engage bots and other players, and extract before being consumed by a one-directional storm.

This visualization tool parses **5 days of production telemetry data** (1,243 Apache Parquet files, ~89,104 events across 796 matches) and provides Level Designers with an interactive spatial playground to diagnose map flow, identify neglected areas, and balance combat zones.

---

## 🎯 Key Features for Level Designers

### 1. 🗺️ Orthographic Minimap Projection
- Renders top-down high-resolution 1024×1024 minimaps for **Ambrose Valley**, **Grand Rift**, and **Lockdown**.
- Accurately converts 3D world space coordinates $(X, Z)$ to 2D Cartesian Leaflet map space (`L.CRS.Simple`).

### 2. 🧭 Live World Coordinate Inspector HUD
- Hover anywhere on the map to display the exact **in-game world coordinates** $(X, Z)$ and UV fractions in a real-time HUD.
- Allows Level Designers to effortlessly look up suspicious terrain, dead ends, or choke points directly in Unreal Engine / Unity.

### 3. ⏱️ Interactive Match Timeline & Playback Scrubber
- Watch any match unfold sequentially from start to end with an interactive slider.
- **Playback Controls**: Play/Pause (Spacebar), Reset, and Variable Speed (`0.5x`, `1x`, `2x`, `5x`, `10x`).
- **Live Roster Telemetry**: Real-time counter of remaining alive humans (👤), bots (🤖), and loot collected as the match progresses.
- **Dynamic Entity Heads**: Active players are rendered with pulsing markers and trailing ghost paths indicating their current trajectory.

### 4. 🎛️ Level Designer Layer Toggles
Filter individual telemetry layers dynamically on/off:
- 👤 **Human Players** (Cyan paths & markers) vs 🤖 **Bot AI** (Orange paths & markers)
- 〰️ **Movement Trajectory Paths** (Full journey polylines)
- ⚔️ **Kill Hotspots** (Red combat markers with killer & victim tooltips)
- 💀 **Combat Deaths** (Dark red markers)
- ⚡ **Storm Fatalities** (Purple markers indicating players trapped by the storm)
- 📦 **Loot Pickups** (Gold markers indicating item acquisition locations)

### 5. 🔥 Multi-Type Density Heatmaps
Precomputed 32×32 and 64×64 heatmaps that switch seamlessly without client-side lag:
- **Traffic**: High-density player migration corridors and highways.
- **Kill Hotspots**: Primary firefight and ambush locations.
- **Death Zones**: High-fatality terrain.
- **Loot Density**: Resource saturation across POIs.
- **Storm Fatalities**: Choke points where players fail to evacuate in time.

### 6. 📊 Match & Player Telemetry Breakdown
- **Match Metrics**: Total duration, combat kills, storm fatality percentage, and loot collection count.
- **Player Focus**: Click any player or bot in the roster to isolate their specific journey, see their combat kills and loot count, and inspect their final fate (*Survived/Extracted*, *Killed in Combat*, or *Killed by Storm*).

---

## 🛠️ Tech Stack

- **Backend (C++20)**:
  - **Apache Arrow & Parquet**: Multi-threaded, memory-mapped Parquet reader parsing all 1,243 files in **~240ms** across 24 worker threads.
  - **cpp-httplib**: Multi-threaded lightweight HTTP/1.1 REST server.
  - **FlatBuffers (`schemas/api.fbs`)**: High-efficiency zero-copy binary serialization support.
  - **zlib**: Transparent on-the-fly gzip compression for responses exceeding 1 KB.
- **Frontend (React 18 + Vite)**:
  - **Leaflet (`L.CRS.Simple`)**: Smooth hardware-accelerated pan/zoom Cartesian mapping.
  - **Vite 6**: Sub-second builds and HMR.
  - **Vanilla CSS**: Modern dark glassmorphism theme designed for prolonged level design review sessions.

---

## 🚀 Quick Start

### Prerequisites
- **C++ Compiler**: GCC 11+, Clang 13+, or MSVC supporting C++20
- **CMake**: Version 3.20 or newer
- **Ninja**: Build system
- **vcpkg** or system dependencies (`arrow`, `parquet`, `cpp-httplib`, `nlohmann-json`, `flatbuffers`)
- **Node.js**: v18+ & npm

### 1. Build the Frontend
```bash
cd frontend
npm install
npm run build
cd ..
```

### 2. Build the C++ Backend Engine
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
```

### 3. Run the Application
```bash
./build/player_viz --data ../player_data --static ./frontend/dist --port 8080
```
Open **`http://localhost:8080`** in your browser.

---

## ⚙️ Configuration & CLI Options

| Argument | Description | Default |
| :--- | :--- | :--- |
| `--data <path>` | Path to directory containing `player_data` date folders (`February_*`) | `../player_data` |
| `--static <path>` | Path to compiled frontend build directory | `../frontend/dist` |
| `--port <int>` | Port for the HTTP server to listen on | `8080` |

---

## 📐 Coordinate Transformation System

The in-game world uses arbitrary 3D units $(x, y, z)$. For 2D minimaps ($1024 \times 1024$ pixels):
- Elevation $y$ represents 3D vertical height and is omitted for 2D floor plans.
- Coordinates map to normalized UV space ($[0, 1]$):
  $$u = \frac{x - \text{origin}_x}{\text{scale}}, \quad v = \frac{z - \text{origin}_z}{\text{scale}}$$
- Because Leaflet's `L.CRS.Simple` defines Latitude increasing upwards ($0$ at South/Bottom, $1024$ at North/Top):
  $$\text{Leaflet Lat} = v \times 1024 = 1024 - \text{pixel}_y$$
  $$\text{Leaflet Lng} = u \times 1024 = \text{pixel}_x$$

### Map Reference Table

| Map ID | Scale | Origin X | Origin Z | Minimap Asset |
| :--- | :--- | :--- | :--- | :--- |
| **AmbroseValley** | 900 | -370 | -473 | `AmbroseValley_Minimap.png` |
| **GrandRift** | 581 | -290 | -290 | `GrandRift_Minimap.png` |
| **Lockdown** | 1000 | -500 | -500 | `Lockdown_Minimap.jpg` |

---

## 🐳 Deployment Guide

### Option A: Docker (Railway / Render / Fly.io / Self-Hosted)
Build and run the multi-stage Docker container:
```bash
docker build -t lila-player-viz .
docker run -p 8080:8080 lila-player-viz
```

### Option B: Cloud Hosting (Railway / Render)
1. Link your GitHub repository to **Railway** or **Render**.
2. Select **Dockerfile** as the build configuration.
3. Set the port to `8080` (or allow the platform `$PORT` binding).
4. The deployment will automatically build both frontend assets and the C++ engine.

---

## 📄 Associated Documentation

- 📐 [**ARCHITECTURE.md**](file:///home/heathknowles/Documents/Code/Misc/player_viz/ARCHITECTURE.md) — Comprehensive 1-page architecture breakdown, data flow, Leaflet coordinate math, assumptions, and tradeoffs table.
- 💡 [**INSIGHTS.md**](file:///home/heathknowles/Documents/Code/Misc/player_viz/INSIGHTS.md) — Three deep, data-backed level design discoveries (Lockdown storm trap, solo-PvE bot reality, and Ambrose Valley loot highways).
- 📜 [**player_data/README.md**](file:///home/heathknowles/Documents/Code/Misc/player_data/README.md) — Telemetry format specifications, Parquet column schemas, and event dictionary.
