#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

enum class EventType : uint8_t {
    Position,
    BotPosition,
    Kill,
    Killed,
    BotKill,
    BotKilled,
    KilledByStorm,
    Loot,
    Unknown
};

inline EventType parse_event_type(std::string_view s) {
    if (s == "Position") return EventType::Position;
    if (s == "BotPosition") return EventType::BotPosition;
    if (s == "Kill") return EventType::Kill;
    if (s == "Killed") return EventType::Killed;
    if (s == "BotKill") return EventType::BotKill;
    if (s == "BotKilled") return EventType::BotKilled;
    if (s == "KilledByStorm") return EventType::KilledByStorm;
    if (s == "Loot") return EventType::Loot;
    return EventType::Unknown;
}

inline std::string_view event_type_to_string(EventType t) {
    switch (t) {
        case EventType::Position: return "Position";
        case EventType::BotPosition: return "BotPosition";
        case EventType::Kill: return "Kill";
        case EventType::Killed: return "Killed";
        case EventType::BotKill: return "BotKill";
        case EventType::BotKilled: return "BotKilled";
        case EventType::KilledByStorm: return "KilledByStorm";
        case EventType::Loot: return "Loot";
        default: return "Unknown";
    }
}

struct Event {
    std::string user_id;
    std::string match_id;
    std::string map_id;
    float x;
    float y;
    float z;
    int64_t ts;
    EventType event;

    bool is_human() const {
        return user_id.size() > 6;
    }

    bool is_position() const {
        return event == EventType::Position || event == EventType::BotPosition;
    }

    bool is_kill() const {
        return event == EventType::Kill || event == EventType::BotKill;
    }

    bool is_death() const {
        return event == EventType::Killed || event == EventType::BotKilled || event == EventType::KilledByStorm;
    }

    bool is_loot() const {
        return event == EventType::Loot;
    }

    bool is_storm_death() const {
        return event == EventType::KilledByStorm;
    }
};

struct PlayerSummary {
    std::string user_id;
    bool is_human;
    int64_t event_count;
    int64_t first_ts;
    int64_t last_ts;
    EventType death_event;

    bool has_died() const {
        return death_event != EventType::Unknown;
    }
};

struct MatchSummary {
    std::string match_id;
    std::string map_id;
    std::string date;
    int64_t start_ts;
    int64_t end_ts;
    int human_count;
    int bot_count;
    int total_events;
    std::vector<std::string> human_ids;
};

struct MatchDetail {
    std::string match_id;
    std::string map_id;
    std::string date;
    std::vector<Event> events;
    std::vector<PlayerSummary> players;
    int64_t duration_ms;
};

struct MapConfig {
    std::string name;
    float scale;
    float origin_x;
    float origin_z;

    std::pair<float, float> world_to_uv(float wx, float wz) const {
        float u = (wx - origin_x) / scale;
        float v = (wz - origin_z) / scale;
        return {u, v};
    }

    std::pair<int, int> world_to_pixel(float wx, float wz) const {
        auto [u, v] = world_to_uv(wx, wz);
        int px = static_cast<int>(u * 1024);
        int py = static_cast<int>((1.0f - v) * 1024);
        return {px, py};
    }
};

struct HeatmapCell {
    int px;
    int py;
    float intensity;
};

struct PrecomputedHeatmap {
    std::string map_id;
    std::string event_type;
    int grid_size;
    std::vector<HeatmapCell> cells;
};

inline void to_json(json& j, const Event& e) {
    j = json{
        {"user_id", e.user_id},
        {"match_id", e.match_id},
        {"map_id", e.map_id},
        {"x", e.x},
        {"y", e.y},
        {"z", e.z},
        {"ts", e.ts},
        {"event", event_type_to_string(e.event)}
    };
}

inline void to_json(json& j, const PlayerSummary& p) {
    j = json{
        {"user_id", p.user_id},
        {"is_human", p.is_human},
        {"event_count", p.event_count},
        {"first_ts", p.first_ts},
        {"last_ts", p.last_ts},
        {"death_event", event_type_to_string(p.death_event)}
    };
}

inline void to_json(json& j, const MatchSummary& m) {
    j = json{
        {"match_id", m.match_id},
        {"map_id", m.map_id},
        {"date", m.date},
        {"start_ts", m.start_ts},
        {"end_ts", m.end_ts},
        {"human_count", m.human_count},
        {"bot_count", m.bot_count},
        {"total_events", m.total_events},
        {"human_ids", m.human_ids}
    };
}

inline void to_json(json& j, const MatchDetail& m) {
    j = json{
        {"match_id", m.match_id},
        {"map_id", m.map_id},
        {"date", m.date},
        {"events", m.events},
        {"players", m.players},
        {"duration_ms", m.duration_ms}
    };
}

inline void to_json(json& j, const MapConfig& m) {
    j = json{
        {"name", m.name},
        {"scale", m.scale},
        {"origin_x", m.origin_x},
        {"origin_z", m.origin_z}
    };
}

inline void to_json(json& j, const HeatmapCell& h) {
    j = json{{"px", h.px}, {"py", h.py}, {"intensity", h.intensity}};
}

inline void to_json(json& j, const PrecomputedHeatmap& h) {
    j = json{
        {"map_id", h.map_id},
        {"event_type", h.event_type},
        {"grid_size", h.grid_size},
        {"cells", h.cells}
    };
}
