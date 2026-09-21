#pragma once

#include "models.h"
#include "api_generated.h"
#include <flatbuffers/flatbuffers.h>
#include <vector>
#include <memory>

class FlatBufferSerializer {
public:
    static std::vector<uint8_t> serialize_stats(
        const std::vector<Event>& all_events,
        const std::unordered_map<std::string, MatchSummary>& matches,
        const std::vector<std::string>& dates,
        const std::vector<MapConfig>& map_configs,
        int total_humans,
        int total_bots
    );

    static std::vector<uint8_t> serialize_maps(const std::vector<MapConfig>& maps);

    static std::vector<uint8_t> serialize_dates(const std::vector<std::string>& dates);

    static std::vector<uint8_t> serialize_matches(const std::vector<MatchSummary>& matches);

    static std::vector<uint8_t> serialize_match_detail(const MatchDetail& detail);

    static std::vector<uint8_t> serialize_heatmap(const PrecomputedHeatmap& hm);

    static std::vector<uint8_t> serialize_all_heatmaps(const std::vector<PrecomputedHeatmap>& heatmaps);

private:
    static flatbuffers::Offset<flatbuffers::String> intern_string(
        flatbuffers::FlatBufferBuilder& builder,
        const std::string& s
    );

    static flatbuffers::Offset<PlayerViz::Event> serialize_event(
        flatbuffers::FlatBufferBuilder& builder,
        const Event& evt
    );

    static flatbuffers::Offset<PlayerViz::PlayerSummary> serialize_player(
        flatbuffers::FlatBufferBuilder& builder,
        const PlayerSummary& ps
    );

    static flatbuffers::Offset<PlayerViz::MatchSummary> serialize_match_summary(
        flatbuffers::FlatBufferBuilder& builder,
        const MatchSummary& ms
    );

    static flatbuffers::Offset<PlayerViz::HeatmapCell> serialize_heatmap_cell(
        flatbuffers::FlatBufferBuilder& builder,
        const HeatmapCell& cell
    );

    static flatbuffers::Offset<PlayerViz::MapConfig> serialize_map_config(
        flatbuffers::FlatBufferBuilder& builder,
        const MapConfig& cfg
    );

    static PlayerViz::EventType to_fb_event_type(EventType t);
    static PlayerViz::EventType to_fb_death_type(EventType t);
};
