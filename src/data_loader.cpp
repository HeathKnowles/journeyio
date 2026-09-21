#include "data_loader.h"
#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/reader.h>
#include <iostream>
#include <algorithm>
#include <set>
#include <cmath>

DataLoader::DataLoader(const std::string& data_dir) : data_dir_(data_dir) {
    init_map_configs();
}

void DataLoader::init_map_configs() {
    map_configs_ = {
        {"AmbroseValley", 900.0f, -370.0f, -473.0f},
        {"GrandRift", 581.0f, -290.0f, -290.0f},
        {"Lockdown", 1000.0f, -500.0f, -500.0f}
    };
}

const MapConfig& DataLoader::get_map_config(const std::string& map_name) const {
    for (const auto& cfg : map_configs_) {
        if (cfg.name == map_name) return cfg;
    }
    throw std::runtime_error("Unknown map: " + map_name);
}

std::vector<std::filesystem::directory_entry> DataLoader::collect_all_files() {
    std::vector<std::filesystem::directory_entry> all_files;
    std::vector<std::string> date_folders;

    for (const auto& entry : std::filesystem::directory_iterator(data_dir_)) {
        if (entry.is_directory() && entry.path().filename().string().find("February") != std::string::npos) {
            date_folders.push_back(entry.path().filename().string());
        }
    }
    std::sort(date_folders.begin(), date_folders.end());
    dates_ = date_folders;

    for (const auto& folder : date_folders) {
        std::string day_path = data_dir_ + "/" + folder;
        for (const auto& entry : std::filesystem::directory_iterator(day_path)) {
            if (entry.is_regular_file()) {
                all_files.push_back(entry);
            }
        }
    }
    return all_files;
}

static std::vector<Event> parse_file(const std::filesystem::directory_entry& entry) {
    std::vector<Event> events;
    std::string filepath = entry.path().string();

    try {
        auto input_result = arrow::io::ReadableFile::Open(filepath);
        if (!input_result.ok()) return events;

        auto input = input_result.ValueOrDie();
        auto reader_result = parquet::arrow::OpenFile(input, arrow::default_memory_pool());
        if (!reader_result.ok()) return events;

        auto reader = std::move(reader_result).ValueOrDie();
        auto table_result = reader->ReadTable();
        if (!table_result.ok()) return events;

        auto table = table_result.ValueOrDie();
        auto num_rows = table->num_rows();
        if (num_rows == 0) return events;

        auto user_col = table->GetColumnByName("user_id");
        auto match_col = table->GetColumnByName("match_id");
        auto map_col = table->GetColumnByName("map_id");
        auto x_col = table->GetColumnByName("x");
        auto y_col = table->GetColumnByName("y");
        auto z_col = table->GetColumnByName("z");
        auto ts_col = table->GetColumnByName("ts");
        auto event_col = table->GetColumnByName("event");

        if (!user_col || !match_col || !map_col || !x_col || !y_col || !z_col || !ts_col || !event_col) {
            return events;
        }

        auto user_arr = std::dynamic_pointer_cast<arrow::StringArray>(user_col->chunk(0));
        auto match_arr = std::dynamic_pointer_cast<arrow::StringArray>(match_col->chunk(0));
        auto map_arr = std::dynamic_pointer_cast<arrow::StringArray>(map_col->chunk(0));
        auto x_arr = std::dynamic_pointer_cast<arrow::FloatArray>(x_col->chunk(0));
        auto y_arr = std::dynamic_pointer_cast<arrow::FloatArray>(y_col->chunk(0));
        auto z_arr = std::dynamic_pointer_cast<arrow::FloatArray>(z_col->chunk(0));
        auto ts_arr = std::dynamic_pointer_cast<arrow::TimestampArray>(ts_col->chunk(0));
        auto evt_arr = std::dynamic_pointer_cast<arrow::BinaryArray>(event_col->chunk(0));

        if (!user_arr || !match_arr || !map_arr || !x_arr || !y_arr || !z_arr || !ts_arr || !evt_arr) {
            return events;
        }

        events.reserve(num_rows);
        for (int64_t i = 0; i < num_rows; ++i) {
            Event e;
            e.user_id = user_arr->GetString(i);
            e.match_id = match_arr->GetString(i);
            e.map_id = map_arr->GetString(i);
            e.x = x_arr->Value(i);
            e.y = y_arr->Value(i);
            e.z = z_arr->Value(i);
            e.ts = ts_arr->Value(i);
            e.event = parse_event_type(evt_arr->GetString(i));
            events.push_back(std::move(e));
        }
    } catch (...) {}
    return events;
}

void DataLoader::load_all() {
    auto all_files = collect_all_files();
    std::cout << "Found " << all_files.size() << " files to load" << std::endl;

    unsigned int num_threads = ThreadPool::optimal_threads();
    std::cout << "Using " << num_threads << " threads for parallel loading" << std::endl;

    std::vector<ThreadLocalBuffer> buffers(num_threads);

    {
        ThreadPool pool(num_threads);
        std::atomic<size_t> file_idx(0);
        std::atomic<int> files_done(0);
        int total = static_cast<int>(all_files.size());

        for (unsigned int t = 0; t < num_threads; ++t) {
            pool.submit([&, t] {
                ThreadLocalBuffer& buf = buffers[t];
                while (true) {
                    size_t idx = file_idx.fetch_add(1, std::memory_order_relaxed);
                    if (idx >= all_files.size()) break;

                    auto events = parse_file(all_files[idx]);
                    for (auto& evt : events) {
                        std::string match_id = evt.match_id;
                        buf.match_events[match_id].push_back(std::move(evt));
                    }

                    int done = files_done.fetch_add(1, std::memory_order_relaxed) + 1;
                    if (done % 200 == 0 || done == total) {
                        std::cout << "  Loaded " << done << "/" << total << " files" << std::endl;
                    }
                }
            });
        }
        pool.wait_all();
    }

    merge_thread_buffers(buffers);
    index_events();
    build_match_indexes();
    precompute_all_heatmaps();

    std::cout << "Loaded " << all_events_.size() << " events from "
              << matches_.size() << " matches across "
              << dates_.size() << " days" << std::endl;
    std::cout << "Pre-computed " << heatmap_cache_.size() << " heatmaps" << std::endl;
}

void DataLoader::merge_thread_buffers(std::vector<ThreadLocalBuffer>& buffers) {
    for (auto& buf : buffers) {
        for (auto& [match_id, events] : buf.match_events) {
            auto& target = match_events_[match_id];
            target.insert(target.end(),
                std::make_move_iterator(events.begin()),
                std::make_move_iterator(events.end()));
        }
    }

    size_t total = 0;
    for (auto& [id, events] : match_events_) {
        total += events.size();
    }
    all_events_.reserve(total);
    for (auto& [id, events] : match_events_) {
        all_events_.insert(all_events_.end(), events.begin(), events.end());
    }

    for (const auto& entry : std::filesystem::directory_iterator(data_dir_)) {
        if (entry.is_directory() && entry.path().filename().string().find("February") != std::string::npos) {
            std::string folder = entry.path().filename().string();
            std::string day_path = data_dir_ + "/" + folder;
            for (const auto& file : std::filesystem::directory_iterator(day_path)) {
                if (!file.is_regular_file()) continue;
                std::string fn = file.path().filename().string();

                size_t first_underscore = fn.find('_');
                if (first_underscore == std::string::npos) continue;

                std::string match_id = fn.substr(first_underscore + 1);

                auto it = match_events_.find(match_id);
                if (it != match_events_.end() && !it->second.empty()) {
                    auto& ms = matches_[match_id];
                    ms.match_id = match_id;
                    ms.map_id = it->second.front().map_id;
                    ms.date = folder;
                }
            }
        }
    }
}

void DataLoader::index_events() {
    for (auto& [match_id, events] : match_events_) {
        std::sort(events.begin(), events.end(),
            [](const Event& a, const Event& b) { return a.ts < b.ts; });

        auto& ms = matches_[match_id];
        ms.start_ts = events.front().ts;
        ms.end_ts = events.back().ts;
        ms.total_events = events.size();

        std::unordered_map<std::string, bool> human_map;
        for (const auto& e : events) {
            if (human_map.find(e.user_id) == human_map.end()) {
                human_map[e.user_id] = e.is_human();
            }
        }

        for (const auto& [uid, is_human] : human_map) {
            if (is_human) {
                ms.human_count++;
                ms.human_ids.push_back(uid);
            } else {
                ms.bot_count++;
            }
        }
    }

    std::sort(dates_.begin(), dates_.end());
}

void DataLoader::build_match_indexes() {
    for (const auto& [match_id, ms] : matches_) {
        matches_by_map_[ms.map_id].push_back(match_id);
        matches_by_date_[ms.date].push_back(match_id);
    }

    for (auto& [key, ids] : matches_by_map_) {
        std::sort(ids.begin(), ids.end(), [this](const std::string& a, const std::string& b) {
            return matches_.at(a).start_ts < matches_.at(b).start_ts;
        });
    }
    for (auto& [key, ids] : matches_by_date_) {
        std::sort(ids.begin(), ids.end(), [this](const std::string& a, const std::string& b) {
            return matches_.at(a).start_ts < matches_.at(b).start_ts;
        });
    }
}

std::string DataLoader::make_heatmap_key(
    const std::string& map_id, const std::string& event_type, int grid_size
) const {
    return map_id + "|" + event_type + "|" + std::to_string(grid_size);
}

void DataLoader::precompute_all_heatmaps() {
    const std::vector<std::string> heatmap_types = {"traffic", "kill", "death", "loot", "storm"};
    const std::vector<int> grid_sizes = {32, 64};

    for (const auto& cfg : map_configs_) {
        for (const auto& event_type : heatmap_types) {
            for (int grid_size : grid_sizes) {
                std::vector<HeatmapCell> cells;

                std::vector<std::pair<float, float>> points;
                for (const auto& e : all_events_) {
                    if (e.map_id != cfg.name) continue;
                    if (event_type == "traffic" && e.is_position()) {
                        points.push_back({e.x, e.z});
                    } else if (event_type == "kill" && e.is_kill()) {
                        points.push_back({e.x, e.z});
                    } else if (event_type == "death" && e.is_death()) {
                        points.push_back({e.x, e.z});
                    } else if (event_type == "loot" && e.is_loot()) {
                        points.push_back({e.x, e.z});
                    } else if (event_type == "storm" && e.is_storm_death()) {
                        points.push_back({e.x, e.z});
                    }
                }

                if (!points.empty()) {
                    std::vector<std::vector<float>> grid(grid_size, std::vector<float>(grid_size, 0.0f));
                    for (const auto& [wx, wz] : points) {
                        auto [px, py] = cfg.world_to_pixel(wx, wz);
                        int gx = std::max(0, std::min(grid_size - 1, (px * grid_size) / 1024));
                        int gy = std::max(0, std::min(grid_size - 1, (py * grid_size) / 1024));
                        grid[gy][gx] += 1.0f;
                    }

                    float max_val = 0.0f;
                    for (const auto& row : grid) {
                        for (float v : row) {
                            if (v > max_val) max_val = v;
                        }
                    }

                    if (max_val > 0.0f) {
                        for (int gy = 0; gy < grid_size; ++gy) {
                            for (int gx = 0; gx < grid_size; ++gx) {
                                if (grid[gy][gx] > 0.0f) {
                                    int px = (gx * 1024) / grid_size;
                                    int py = (gy * 1024) / grid_size;
                                    cells.push_back({px, py, grid[gy][gx] / max_val});
                                }
                            }
                        }
                    }
                }

                std::string key = make_heatmap_key(cfg.name, event_type, grid_size);
                size_t index = heatmap_cache_.size();
                heatmap_cache_.push_back({cfg.name, event_type, grid_size, std::move(cells)});
                heatmap_index_[key] = index;
            }
        }
    }
}

std::vector<MatchSummary> DataLoader::get_matches(
    const std::string& map_id,
    const std::string& date
) const {
    std::vector<MatchSummary> result;

    if (!map_id.empty() && !date.empty()) {
        auto map_it = matches_by_map_.find(map_id);
        auto date_it = matches_by_date_.find(date);
        if (map_it == matches_by_map_.end() || date_it == matches_by_date_.end()) {
            return result;
        }

        const auto& map_matches = map_it->second;
        const auto& date_matches = date_it->second;
        std::vector<std::string> intersection;
        std::set_intersection(
            map_matches.begin(), map_matches.end(),
            date_matches.begin(), date_matches.end(),
            std::back_inserter(intersection)
        );

        for (const auto& id : intersection) {
            result.push_back(matches_.at(id));
        }
    } else if (!map_id.empty()) {
        auto it = matches_by_map_.find(map_id);
        if (it != matches_by_map_.end()) {
            for (const auto& id : it->second) {
                result.push_back(matches_.at(id));
            }
        }
    } else if (!date.empty()) {
        auto it = matches_by_date_.find(date);
        if (it != matches_by_date_.end()) {
            for (const auto& id : it->second) {
                result.push_back(matches_.at(id));
            }
        }
    } else {
        for (const auto& [id, ms] : matches_) {
            result.push_back(ms);
        }
        std::sort(result.begin(), result.end(),
            [](const MatchSummary& a, const MatchSummary& b) { return a.start_ts < b.start_ts; });
    }

    return result;
}

MatchDetail DataLoader::get_match_detail(const std::string& match_id) const {
    auto it = match_events_.find(match_id);
    if (it == match_events_.end()) {
        throw std::runtime_error("Match not found: " + match_id);
    }

    MatchDetail detail;
    detail.match_id = match_id;
    detail.events = it->second;

    auto ms_it = matches_.find(match_id);
    if (ms_it != matches_.end()) {
        detail.map_id = ms_it->second.map_id;
        detail.date = ms_it->second.date;
        detail.duration_ms = ms_it->second.end_ts - ms_it->second.start_ts;
    }

    std::unordered_map<std::string, PlayerSummary> player_map;
    for (const auto& e : detail.events) {
        auto& ps = player_map[e.user_id];
        ps.user_id = e.user_id;
        ps.is_human = e.is_human();
        ps.event_count++;
        if (ps.first_ts == 0 || e.ts < ps.first_ts) ps.first_ts = e.ts;
        if (e.ts > ps.last_ts) ps.last_ts = e.ts;

        if (e.is_death()) {
            ps.death_event = e.event;
        }
    }

    for (const auto& [uid, ps] : player_map) {
        detail.players.push_back(ps);
    }

    return detail;
}

const PrecomputedHeatmap& DataLoader::get_heatmap(
    const std::string& map_id,
    const std::string& event_type,
    int grid_size
) const {
    std::string key = make_heatmap_key(map_id, event_type, grid_size);
    auto it = heatmap_index_.find(key);
    if (it == heatmap_index_.end()) {
        throw std::runtime_error("Heatmap not found: " + key);
    }
    return heatmap_cache_[it->second];
}
