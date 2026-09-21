#include "flatbuf_serializer.h"

PlayerViz::EventType FlatBufferSerializer::to_fb_event_type(EventType t) {
    switch (t) {
        case EventType::Position: return PlayerViz::EventType::Position;
        case EventType::BotPosition: return PlayerViz::EventType::BotPosition;
        case EventType::Kill: return PlayerViz::EventType::Kill;
        case EventType::Killed: return PlayerViz::EventType::Killed;
        case EventType::BotKill: return PlayerViz::EventType::BotKill;
        case EventType::BotKilled: return PlayerViz::EventType::BotKilled;
        case EventType::KilledByStorm: return PlayerViz::EventType::KilledByStorm;
        case EventType::Loot: return PlayerViz::EventType::Loot;
        default: return PlayerViz::EventType::Unknown;
    }
}

PlayerViz::EventType FlatBufferSerializer::to_fb_death_type(EventType t) {
    switch (t) {
        case EventType::Killed: return PlayerViz::EventType::Killed;
        case EventType::BotKilled: return PlayerViz::EventType::BotKilled;
        case EventType::KilledByStorm: return PlayerViz::EventType::KilledByStorm;
        default: return PlayerViz::EventType::Unknown;
    }
}

flatbuffers::Offset<flatbuffers::String> FlatBufferSerializer::intern_string(
    flatbuffers::FlatBufferBuilder& builder,
    const std::string& s
) {
    return builder.CreateString(s);
}

flatbuffers::Offset<PlayerViz::Event> FlatBufferSerializer::serialize_event(
    flatbuffers::FlatBufferBuilder& builder,
    const Event& evt
) {
    return PlayerViz::CreateEvent(
        builder,
        intern_string(builder, evt.user_id),
        intern_string(builder, evt.match_id),
        intern_string(builder, evt.map_id),
        evt.x,
        evt.y,
        evt.z,
        evt.ts,
        to_fb_event_type(evt.event)
    );
}

flatbuffers::Offset<PlayerViz::PlayerSummary> FlatBufferSerializer::serialize_player(
    flatbuffers::FlatBufferBuilder& builder,
    const PlayerSummary& ps
) {
    return PlayerViz::CreatePlayerSummary(
        builder,
        intern_string(builder, ps.user_id),
        ps.is_human,
        ps.event_count,
        ps.first_ts,
        ps.last_ts,
        to_fb_death_type(ps.death_event)
    );
}

flatbuffers::Offset<PlayerViz::MatchSummary> FlatBufferSerializer::serialize_match_summary(
    flatbuffers::FlatBufferBuilder& builder,
    const MatchSummary& ms
) {
    auto human_ids = builder.CreateVectorOfStrings(ms.human_ids);
    return PlayerViz::CreateMatchSummary(
        builder,
        intern_string(builder, ms.match_id),
        intern_string(builder, ms.map_id),
        intern_string(builder, ms.date),
        ms.start_ts,
        ms.end_ts,
        ms.human_count,
        ms.bot_count,
        ms.total_events,
        human_ids
    );
}

flatbuffers::Offset<PlayerViz::HeatmapCell> FlatBufferSerializer::serialize_heatmap_cell(
    flatbuffers::FlatBufferBuilder& builder,
    const HeatmapCell& cell
) {
    return PlayerViz::CreateHeatmapCell(builder, cell.px, cell.py, cell.intensity);
}

flatbuffers::Offset<PlayerViz::MapConfig> FlatBufferSerializer::serialize_map_config(
    flatbuffers::FlatBufferBuilder& builder,
    const MapConfig& cfg
) {
    return PlayerViz::CreateMapConfig(
        builder,
        intern_string(builder, cfg.name),
        cfg.scale,
        cfg.origin_x,
        cfg.origin_z
    );
}

std::vector<uint8_t> FlatBufferSerializer::serialize_stats(
    const std::vector<Event>& all_events,
    const std::unordered_map<std::string, MatchSummary>& matches,
    const std::vector<std::string>& dates,
    const std::vector<MapConfig>& map_configs,
    int total_humans,
    int total_bots
) {
    flatbuffers::FlatBufferBuilder builder(1024 * 1024);

    auto dates_vec = builder.CreateVectorOfStrings(dates);
    std::vector<flatbuffers::Offset<flatbuffers::String>> map_names;
    for (const auto& cfg : map_configs) {
        map_names.push_back(builder.CreateString(cfg.name));
    }
    auto maps_vec = builder.CreateVector(map_names);

    auto root = PlayerViz::CreateStatsResponse(
        builder,
        static_cast<int64_t>(all_events.size()),
        static_cast<int32_t>(matches.size()),
        total_humans,
        total_bots,
        dates_vec,
        maps_vec
    );

    builder.Finish(root);
    auto ptr = builder.GetBufferPointer();
    auto size = builder.GetSize();
    return std::vector<uint8_t>(ptr, ptr + size);
}

std::vector<uint8_t> FlatBufferSerializer::serialize_maps(const std::vector<MapConfig>& maps) {
    flatbuffers::FlatBufferBuilder builder(4096);

    std::vector<flatbuffers::Offset<PlayerViz::MapConfig>> fb_maps;
    for (const auto& cfg : maps) {
        fb_maps.push_back(serialize_map_config(builder, cfg));
    }
    auto maps_vec = builder.CreateVector(fb_maps);

    auto root = PlayerViz::CreateMapsResponse(builder, maps_vec);
    builder.Finish(root);

    auto ptr = builder.GetBufferPointer();
    return std::vector<uint8_t>(ptr, ptr + builder.GetSize());
}

std::vector<uint8_t> FlatBufferSerializer::serialize_dates(const std::vector<std::string>& dates) {
    flatbuffers::FlatBufferBuilder builder(4096);

    auto dates_vec = builder.CreateVectorOfStrings(dates);
    auto root = PlayerViz::CreateDatesResponse(builder, dates_vec);
    builder.Finish(root);

    auto ptr = builder.GetBufferPointer();
    return std::vector<uint8_t>(ptr, ptr + builder.GetSize());
}

std::vector<uint8_t> FlatBufferSerializer::serialize_matches(const std::vector<MatchSummary>& matches) {
    flatbuffers::FlatBufferBuilder builder(matches.size() * 512);

    std::vector<flatbuffers::Offset<PlayerViz::MatchSummary>> fb_matches;
    for (const auto& ms : matches) {
        fb_matches.push_back(serialize_match_summary(builder, ms));
    }
    auto matches_vec = builder.CreateVector(fb_matches);

    auto root = PlayerViz::CreateMatchesResponse(builder, matches_vec);
    builder.Finish(root);

    auto ptr = builder.GetBufferPointer();
    return std::vector<uint8_t>(ptr, ptr + builder.GetSize());
}

std::vector<uint8_t> FlatBufferSerializer::serialize_match_detail(const MatchDetail& detail) {
    flatbuffers::FlatBufferBuilder builder(detail.events.size() * 128 + detail.players.size() * 256);

    std::vector<flatbuffers::Offset<PlayerViz::Event>> fb_events;
    for (const auto& evt : detail.events) {
        fb_events.push_back(serialize_event(builder, evt));
    }
    auto events_vec = builder.CreateVector(fb_events);

    std::vector<flatbuffers::Offset<PlayerViz::PlayerSummary>> fb_players;
    for (const auto& ps : detail.players) {
        fb_players.push_back(serialize_player(builder, ps));
    }
    auto players_vec = builder.CreateVector(fb_players);

    auto root = PlayerViz::CreateMatchDetail(
        builder,
        intern_string(builder, detail.match_id),
        intern_string(builder, detail.map_id),
        intern_string(builder, detail.date),
        events_vec,
        players_vec,
        detail.duration_ms
    );

    builder.Finish(root);

    auto ptr = builder.GetBufferPointer();
    return std::vector<uint8_t>(ptr, ptr + builder.GetSize());
}

std::vector<uint8_t> FlatBufferSerializer::serialize_heatmap(const PrecomputedHeatmap& hm) {
    flatbuffers::FlatBufferBuilder builder(hm.cells.size() * 16 + 512);

    std::vector<flatbuffers::Offset<PlayerViz::HeatmapCell>> fb_cells;
    for (const auto& cell : hm.cells) {
        fb_cells.push_back(serialize_heatmap_cell(builder, cell));
    }
    auto cells_vec = builder.CreateVector(fb_cells);

    auto root = PlayerViz::CreateHeatmapResponse(
        builder,
        intern_string(builder, hm.map_id),
        intern_string(builder, hm.event_type),
        hm.grid_size,
        cells_vec
    );

    builder.Finish(root);

    auto ptr = builder.GetBufferPointer();
    return std::vector<uint8_t>(ptr, ptr + builder.GetSize());
}

std::vector<uint8_t> FlatBufferSerializer::serialize_all_heatmaps(const std::vector<PrecomputedHeatmap>& heatmaps) {
    flatbuffers::FlatBufferBuilder builder(1024 * 1024);

    std::vector<flatbuffers::Offset<PlayerViz::PrecomputedHeatmap>> fb_hms;
    for (const auto& hm : heatmaps) {
        std::vector<flatbuffers::Offset<PlayerViz::HeatmapCell>> fb_cells;
        for (const auto& cell : hm.cells) {
            fb_cells.push_back(serialize_heatmap_cell(builder, cell));
        }
        auto cells_vec = builder.CreateVector(fb_cells);

        fb_hms.push_back(PlayerViz::CreatePrecomputedHeatmap(
            builder,
            intern_string(builder, hm.map_id),
            intern_string(builder, hm.event_type),
            hm.grid_size,
            cells_vec
        ));
    }
    auto hm_vec = builder.CreateVector(fb_hms);

    auto root = PlayerViz::CreateAllHeatmapsResponse(builder, hm_vec);
    builder.Finish(root);

    auto ptr = builder.GetBufferPointer();
    return std::vector<uint8_t>(ptr, ptr + builder.GetSize());
}
