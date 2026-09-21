# System Architecture — Player Journey Visualization Tool

## 1. What We Built With & Why

| Component | Technology | Rationale |
| :--- | :--- | :--- |
| **Backend Engine** | **C++20** (`Arrow`, `Parquet`, `cpp-httplib`) | High-throughput parsing of 1,243 Parquet files without JIT overhead. Ingests all 89k events across 796 matches in **~240ms** using 24-thread parallel workers. |
| **Data Serialization** | **Dual JSON & FlatBuffers** (`api.fbs`) | Standard JSON for rapid client prototyping; zero-copy FlatBuffers binary transport with on-the-fly **gzip** compression (>1KB) for high performance. |
| **Frontend Framework**| **React 18 + Vite** | Instant HMR, minimal footprint, declarative component state for filters, match scrubbing, and Level Designer HUD tools. |
| **Map Engine** | **Leaflet** (`L.CRS.Simple`) | Cartesian 2D coordinate system without planetary projections; provides high-performance smooth panning, zooming, and canvas marker layers. |

---

## 2. End-to-End Data Pipeline

```
[Parquet Files: 1,243 files in player_data/]
                      │
                      ▼
[DataLoader (C++20)] ── Multi-threaded Arrow Parquet Reader (24 threads)
                      │  - Decodes byte strings to UTF-8 event names
                      │  - Disregards 3D elevation 'y' for 2D positioning
                      │  - Segregates Human (UUID) vs Bot (Numeric ID)
                      ▼
[In-Memory Index & Precomputed Heatmaps]
  ├── Matches Index (by Map ID, Date, Start/End Timestamps)
  └── 30 Heatmaps (3 Maps × 5 Event Types [Traffic, Kill, Death, Loot, Storm] × 2 Grid Sizes [32, 64])
                      │
                      ▼
[HTTP REST Server (cpp-httplib + gzip)]
  ├── GET /api/stats, /api/maps, /api/dates, /api/matches
  ├── GET /api/match/:id (Full timeline + player summaries)
  └── GET /api/heatmap?map=...&type=...&grid=...
                      │
                      ▼
[Browser Client (React 18 + Leaflet)]
  ├── Match Browser & Level Designer Layer Toggles (Humans, Bots, Trails, Kills, Deaths, Storm, Loot)
  ├── Interactive Timeline Scrubber & Live Playback Engine
  └── Coordinate Inspector HUD (Hover Leaflet LatLng -> In-Game World X, Z)
```

---

## 3. Coordinate Mapping Approach (The Tricky Part)

The game world operates in arbitrary Cartesian 3D coordinates $(x, y, z)$. The minimap is a top-down $1024 \times 1024$ pixel orthographic projection where:
- $x$ maps horizontally (East-West).
- $z$ maps vertically (North-South).
- $y$ is vertical elevation/height in Unreal/Unity world space, which is omitted for 2D minimap plotting.

### The Conversion Equations:
1. **World to Normalized UV Coordinates ($[0, 1]$)**:
   $$u = \frac{x - \text{origin}_x}{\text{scale}}, \quad v = \frac{z - \text{origin}_z}{\text{scale}}$$
2. **UV to Standard Image Pixel Space ($1024 \times 1024$, origin top-left)**:
   $$\text{pixel}_x = u \times 1024, \quad \text{pixel}_y = (1 - v) \times 1024$$
3. **The Leaflet `L.CRS.Simple` Trap**:
   In standard canvas/DOM rendering, $(0,0)$ is top-left and $Y$ goes downwards. However, Leaflet's `L.CRS.Simple` treats `lat` as Cartesian $Y$ that **increases upwards** ($lat = 0$ is South/Bottom, $lat = 1024$ is North/Top).
   Passing $[\text{pixel}_y, \text{pixel}_x]$ directly into Leaflet causes an inverted map. The correct transformation for Leaflet $[\text{lat}, \text{lng}]$ is:
   $$\text{lat} = 1024 - \text{pixel}_y = v \times 1024$$
   $$\text{lng} = \text{pixel}_x = u \times 1024$$
4. **Reverse Inspector (Leaflet to Game World)**:
   $$x = \left(\frac{\text{lng}}{1024}\right) \times \text{scale} + \text{origin}_x, \quad z = \left(\frac{\text{lat}}{1024}\right) \times \text{scale} + \text{origin}_z$$
   This enables the real-time HUD displaying exact world coordinates on mouse hover.

---

## 4. Assumptions & Ambiguities Handled

1. **Parquet Files without Extensions**: Files like `{uid}_{mid}.nakama-0` have no `.parquet` suffix. The loader passes files directly to the Arrow `ReadableFile` buffer, validating magic headers.
2. **Binary Column Types**: The `event` column is stored as raw binary bytes. Decoded via `GetString(i)` or `.decode('utf-8')`.
3. **Bot vs Human Identification**: `user_id` values $\le 6$ characters (or purely numeric strings like `1440`) are identified as Bots; standard 36-character UUIDs are classified as Humans.
4. **Timestamp Offsets**: The `ts` column represents match-elapsed milliseconds (Epoch starting Jan 1970). Matches are normalized by calculating offset $t_{\text{rel}} = ts - ts_{\text{start}}$ for timeline scrubbing.
5. **Partial Days**: February 14 is a partial recording day; matches with single-event fragments are indexed gracefully without crashing.

---

## 5. Major Tradeoffs Considered

| Architecture Decision | Options Considered | Decision | Tradeoff & Justification |
| :--- | :--- | :--- | :--- |
| **Data Ingestion Engine** | Python Pandas vs C++ Arrow vs DuckDB | **C++20 Arrow Multi-threaded** | C++ loads all 1,243 files in **240ms** vs 8–12s in Python. Enables near-instant cold boot and zero-dependency production binaries. |
| **Heatmap Generation** | Client-side KDE vs Server-side Precomputed Grid | **Server Precomputation (32 & 64 Grid)** | Precomputing 30 density matrices on startup eliminates client-side frame stutter when toggling heatmaps. |
| **API Protocol** | Pure JSON vs GraphQL vs FlatBuffers | **Dual JSON + FlatBuffers** | JSON provides rapid browser debugging; FlatBuffers enables zero-copy binary serialization for large match files. |
| **Map Rendering** | Canvas 2D vs WebGL vs Leaflet | **Leaflet (CRS.Simple)** | Native tile/image overlay, smooth touch/mouse inertia, and vector marker layers with tooltip support out of the box. |
| **Deployment Strategy** | Standalone Desktop vs Container vs Static Host | **Containerized Web + Static Fallback** | Docker container runs anywhere (Railway, Render, Fly.io); pre-exported JSON enables static hosting on Vercel/Netlify. |
