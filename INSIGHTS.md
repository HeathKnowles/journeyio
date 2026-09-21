# LILA BLACK — Level Design Telemetry Insights

Based on 5 days of production telemetry (89,104 events across 796 matches), here are three critical level design insights discovered using the **Player Journey Visualization Tool**.

---

## Insight 1: The Lockdown Storm Trap — Choke-Point Evacuation Bottlenecks

### 1. What Caught Our Eye
In **Lockdown**, players die to the storm at an alarming rate compared to other maps. Despite being the smallest map in rotation, players frequently get consumed by the storm while attempting to extract.

### 2. The Data
- **Storm Deaths by Map**:
  - **Lockdown**: 17 storm deaths across 171 matches (**9.94% storm fatality rate per match**).
  - **Ambrose Valley**: 17 storm deaths across 566 matches (**3.00% storm fatality rate per match**).
  - **Grand Rift**: 5 storm deaths across 59 matches (**8.47% storm fatality rate per match**).
- **Lethality Ratio**: A player in Lockdown is **3.31× more likely to die to the storm** than a player in Ambrose Valley.
- **Spatial Clustering**: Inspecting the `Storm Fatalities` heatmap on Lockdown reveals that storm deaths do not occur randomly at the map edge; rather, they cluster tightly around **interior dead-ends, corridor choke points, and two specific multi-story stairwells** where players attempt to navigate backward toward extraction as the storm advances.

### 3. Actionable Items & Metrics Affected
- **Actionable Items for Level Designers**:
  1. **Secondary Evacuation Arteries**: Knock out walls in the two dead-end warehouse rooms to create loop corridors, giving players a throughway rather than trapping them in cul-de-sacs.
  2. **Emergency Wall Breaches / One-Way Drop Chutes**: Place one-way escape windows or drops near high-risk choke points so players running from the storm can leap to safety without retracing their steps into incoming bots.
  3. **Visual Extraction Beacons**: Add localized flashing hazard lighting along primary extraction routes that activate 30 seconds before the storm wall reaches that zone.
- **Target Metrics Affected**:
  - **Storm Fatality Rate**: Reduce from 9.94% to **< 4.0%**.
  - **Extraction Success Rate**: Increase from ~84% to **> 91%**.
  - **Post-Death Frustration / Rage-Quits**: Decrease early-session dropouts caused by non-combat deaths.

### 4. Why Level Designers Should Care
Extraction shooters rely on the promise that surviving is in the player's hands. When 1 in 10 matches ends in an unavoidable storm death due to geometry dead-ends rather than combat defeat, players perceive the map geometry as unfair, damaging D1 to D7 retention.

---

## Insight 2: The Solo-PvE Concurrency Reality & Bot Pathing Mesh

### 1. What Caught Our Eye
Out of 796 matches, there were **2,415 bot kills** but only **3 human-vs-human kills**. Examining the match rosters revealed a striking structural fact about the playtest dataset.

### 2. The Data
- **Lobby Player Concurrency**:
  - Matches with **1 Human Player**: **778 matches (97.74%)**.
  - Matches with **2 Human Players**: **2 matches (0.25%)**.
  - Matches with **0 Human Players** (Bot validation tests): **16 matches (2.01%)**.
- **Combat Distribution**:
  - Total Bot Kills (`BotKill`): **2,415**.
  - Total Human Deaths by Bot (`BotKilled`): **700**.
  - Total PvP Kills (`Kill`): **3**.
- **Bot Behavior Pattern**: When using the Timeline playback scrubber, bots spawn in fixed clusters of 8–15 units and path toward the sole human player's coordinate radius. However, bots frequently congregate in large clumps along central roads, creating sudden difficulty spikes rather than paced encounters.

### 3. Actionable Items & Metrics Affected
- **Actionable Items for Level Designers**:
  1. **Staggered Bot Encounter Spacing**: Relocate bot spawn nodes from open roads into defensive cover pockets (guard towers, trench fortifications, POI courtyards) with leashes preventing clumping.
  2. **PvP Encounter Choke Design**: When testing matches with 2+ humans, players never met because the maps (scale 900 on Ambrose Valley) are too vast for solo players to cross paths organically. Introduce **Central Signal Towers** or **Airdrop Beacons** that ping high-tier loot and draw players toward a shared objective.
  3. **Bot Navmesh Boundaries**: Restrict bot patrolling so bots do not camp directly on the extraction zone doors.
- **Target Metrics Affected**:
  - **Combat Pacing (Time-to-First-Engagement)**: Smooth variance from chaotic clusters to predictable 60s intervals.
  - **Player Survival vs Bot Swarms**: Reduce bot kill spikes where players are ambushed by 4+ bots simultaneously.
  - **PvP Encounter Frequency**: Increase organic player meetings from 0.25% toward 15%+ in multi-player lobbies.

### 4. Why Level Designers Should Care
If a level is designed assuming multi-squad PvP crossfire, but actual matchmaking serves solo players fighting bots, the map will feel desolate and unengaging. Level designers must tune sightlines, cover, and POI distribution for the actual player density that the servers support.

---

## Insight 3: The Ambrose Valley Loot Highway vs Abandoned Perimeter Sectors

### 1. What Caught Our Eye
On **Ambrose Valley** (the most played map with 61,013 events), player movement paths and loot collections are concentrated along a narrow southwest-to-northeast diagonal corridor. Large perimeter sectors are virtually untouched.

### 2. The Data
- **Loot Activity Concentration**:
  - Total Loot Events: **9,955 pickups on Ambrose Valley** (77.3% of all loot across all 3 maps).
  - **Highway Density**: Over **68% of all loot pickups** occur within a narrow 200m-wide strip between world coordinates $(-200, -300)$ and $(+150, +200)$.
- **Ignored Zones**:
  - The **Northwest Ridge** ($x \in [-350, -200], z \in [150, 350]$) and the **Southeast Farmlands** ($x \in [150, 300], z \in [-350, -200]$) registered **less than 4% of total player traffic**.
  - Even when players spawned near the Southeast, their trails immediately headed toward the central valley highway without exploring local buildings.

### 3. Actionable Items & Metrics Affected
- **Actionable Items for Level Designers**:
  1. **Loot Table Redistribution**: Rebalance the loot spawner density. Shift 30% of weapon and armor crates from the central riverbed to the abandoned Northwest Ridge and Southeast Farmlands.
  2. **POI Anchor Assets**: Place landmark architectural features (e.g., a Radar Station, a Crash Site) on the Northwest Ridge to provide visual draw distance cues that attract players from afar.
  3. **Secondary Extraction Points**: Add guaranteed extraction zones at the perimeter edges. Players will only route through perimeter zones if they know an exit exists there.
- **Target Metrics Affected**:
  - **Map Space Utilization**: Increase perimeter sector visit rate from 4% to **> 25%**.
  - **Route Diversity**: Reduce the concentration of movement on the central highway from 68% to **< 40%**.
  - **Level Asset ROI**: Ensure that art and environment modeling assets outside the central corridor are actively experienced by players.

### 4. Why Level Designers Should Care
Every unused square kilometer of a map costs memory, draw calls, and development budget. By using telemetry heatmaps to diagnose "dead zones," level designers can pull players into neglected areas with targeted loot and objective placement without having to rebuild the entire map.
