#pragma once

#include "models.h"
#include "thread_pool.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>

class DataLoader {
public:
    DataLoader(const std::string& data_dir);

    void load_all();

    const std::unordered_map<std::string, MatchSummary>& matches() const { return matches_; }
    const std::unordered_map<std::string, std::vector<Event>>& match_events() const { return match_events_; }
    const std::vector<Event>& all_events() const { return all_events_; }
    const std::vector<std::string>& available_dates() const { return dates_; }
    const std::vector<MapConfig>& map_configs() const { return map_configs_; }

    const MapConfig& get_map_config(const std::string& map_name) const;

    std::vector<MatchSummary> get_matches(
        const std::string& map_id = "",
        const std::string& date = ""
    ) const;

    MatchDetail get_match_detail(const std::string& match_id) const;

    const PrecomputedHeatmap& get_heatmap(
        const std::string& map_id,
        const std::string& event_type,
        int grid_size = 64
    ) const;

    std::vector<PrecomputedHeatmap> get_all_heatmaps() const { return heatmap_cache_; }

private:
    std::string data_dir_;
    std::vector<Event> all_events_;
    std::unordered_map<std::string, std::vector<Event>> match_events_;
    std::unordered_map<std::string, MatchSummary> matches_;
    std::vector<std::string> dates_;
    std::vector<MapConfig> map_configs_;

    std::unordered_map<std::string, std::vector<std::string>> matches_by_map_;
    std::unordered_map<std::string, std::vector<std::string>> matches_by_date_;

    std::vector<PrecomputedHeatmap> heatmap_cache_;
    std::unordered_map<std::string, size_t> heatmap_index_;

    void init_map_configs();
    void index_events();
    void build_match_indexes();
    void precompute_all_heatmaps();
    std::string make_heatmap_key(const std::string& map_id, const std::string& event_type, int grid_size) const;

    std::vector<std::filesystem::directory_entry> collect_all_files();
    void load_file_batch(const std::vector<std::filesystem::directory_entry>& files, ThreadLocalBuffer& buffer);
    void merge_thread_buffers(std::vector<ThreadLocalBuffer>& buffers);
};
