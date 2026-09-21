#include "api.h"
#include <iostream>
#include <zlib.h>

static std::string gzip_compress(const std::string& data) {
    z_stream stream{};
    if (deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        return data;
    }
    stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));
    stream.avail_in = data.size();

    std::string compressed;
    compressed.resize(data.size() + data.size() / 100 + 32);

    stream.next_out = reinterpret_cast<Bytef*>(&compressed[0]);
    stream.avail_out = compressed.size();

    if (deflate(&stream, Z_FINISH) != Z_STREAM_END) {
        deflateEnd(&stream);
        return data;
    }
    compressed.resize(stream.total_out);
    deflateEnd(&stream);
    return compressed;
}

Api::Api(DataLoader& loader, const std::string& static_dir)
    : loader_(loader), static_dir_(static_dir) {}

bool Api::wants_flatbuffers(const httplib::Request& req) const {
    if (req.has_param("format") && req.get_param_value("format") == "fb") {
        return true;
    }
    auto it = req.headers.find("Accept");
    if (it != req.headers.end()) {
        if (it->second.find("application/flatbuffers") != std::string::npos ||
            it->second.find("application/octet-stream") != std::string::npos) {
            return true;
        }
    }
    return false;
}

void Api::register_routes(httplib::Server& server) {
    server.Get("/api/maps", [this](const auto& req, auto& res) {
        handle_get_maps(req, res);
    });

    server.Get("/api/dates", [this](const auto& req, auto& res) {
        handle_get_dates(req, res);
    });

    server.Get("/api/matches", [this](const auto& req, auto& res) {
        handle_get_matches(req, res);
    });

    server.Get("/api/match/([^/]+)", [this](const auto& req, auto& res) {
        handle_get_match_detail(req, res);
    });

    server.Get("/api/heatmap", [this](const auto& req, auto& res) {
        handle_get_heatmap(req, res);
    });

    server.Get("/api/heatmaps", [this](const auto& req, auto& res) {
        handle_get_all_heatmaps(req, res);
    });

    server.Get("/api/stats", [this](const auto& req, auto& res) {
        handle_get_stats(req, res);
    });

    server.set_mount_point("/", static_dir_);

    server.set_static_file_compression(true);

    server.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, OPTIONS"},
        {"Access-Control-Allow-Headers", "Accept, Content-Type"}
    });
}

void Api::handle_get_maps(const httplib::Request& req, httplib::Response& res) {
    if (wants_flatbuffers(req)) {
        auto data = fb_serializer_.serialize_maps(loader_.map_configs());
        send_flatbuffers(res, data);
    } else {
        json result = json::array();
        for (const auto& cfg : loader_.map_configs()) {
            result.push_back(cfg);
        }
        send_json(res, result);
    }
}

void Api::handle_get_dates(const httplib::Request& req, httplib::Response& res) {
    if (wants_flatbuffers(req)) {
        auto data = fb_serializer_.serialize_dates(loader_.available_dates());
        send_flatbuffers(res, data);
    } else {
        json result = loader_.available_dates();
        send_json(res, result);
    }
}

void Api::handle_get_matches(const httplib::Request& req, httplib::Response& res) {
    std::string map_id = req.has_param("map") ? req.get_param_value("map") : "";
    std::string date = req.has_param("date") ? req.get_param_value("date") : "";

    auto matches = loader_.get_matches(map_id, date);

    if (wants_flatbuffers(req)) {
        auto data = fb_serializer_.serialize_matches(matches);
        send_flatbuffers(res, data);
    } else {
        json result = json::array();
        for (const auto& m : matches) {
            result.push_back(m);
        }
        send_json(res, result);
    }
}

void Api::handle_get_match_detail(const httplib::Request& req, httplib::Response& res) {
    std::string match_id = req.matches[1];

    try {
        auto detail = loader_.get_match_detail(match_id);
        if (wants_flatbuffers(req)) {
            auto data = fb_serializer_.serialize_match_detail(detail);
            send_flatbuffers(res, data);
        } else {
            send_json(res, detail);
        }
    } catch (const std::exception& e) {
        send_error(res, e.what(), 404);
    }
}

void Api::handle_get_heatmap(const httplib::Request& req, httplib::Response& res) {
    std::string map_id = req.has_param("map") ? req.get_param_value("map") : "AmbroseValley";
    std::string event_type = req.has_param("type") ? req.get_param_value("type") : "traffic";
    if (event_type == "kills") event_type = "kill";
    else if (event_type == "deaths") event_type = "death";
    else if (event_type == "loots") event_type = "loot";
    else if (event_type == "storms") event_type = "storm";

    int grid_size = 64;
    if (req.has_param("grid")) {
        try {
            grid_size = std::stoi(req.get_param_value("grid"));
            grid_size = (grid_size <= 48) ? 32 : 64;
        } catch (...) {}
    }

    try {
        const auto& hm = loader_.get_heatmap(map_id, event_type, grid_size);
        if (wants_flatbuffers(req)) {
            auto data = fb_serializer_.serialize_heatmap(hm);
            send_flatbuffers(res, data);
        } else {
            json result = json::object();
            result["map_id"] = hm.map_id;
            result["event_type"] = hm.event_type;
            result["grid_size"] = hm.grid_size;
            result["cells"] = hm.cells;
            send_json(res, result);
        }
    } catch (const std::exception& e) {
        if (wants_flatbuffers(req)) {
            PrecomputedHeatmap empty_hm{map_id, event_type, grid_size, {}};
            auto data = fb_serializer_.serialize_heatmap(empty_hm);
            send_flatbuffers(res, data);
        } else {
            json result = json::object();
            result["map_id"] = map_id;
            result["event_type"] = event_type;
            result["grid_size"] = grid_size;
            result["cells"] = json::array();
            send_json(res, result);
        }
    }
}

void Api::handle_get_all_heatmaps(const httplib::Request& req, httplib::Response& res) {
    if (wants_flatbuffers(req)) {
        auto data = fb_serializer_.serialize_all_heatmaps(loader_.get_all_heatmaps());
        send_flatbuffers(res, data);
    } else {
        json result = json::array();
        for (const auto& hm : loader_.get_all_heatmaps()) {
            result.push_back(hm);
        }
        send_json(res, result);
    }
}

void Api::handle_get_stats(const httplib::Request& req, httplib::Response& res) {
    int total_humans = 0;
    int total_bots = 0;
    for (const auto& [id, ms] : loader_.matches()) {
        total_humans += ms.human_count;
        total_bots += ms.bot_count;
    }

    if (wants_flatbuffers(req)) {
        auto data = fb_serializer_.serialize_stats(
            loader_.all_events(), loader_.matches(),
            loader_.available_dates(), loader_.map_configs(),
            total_humans, total_bots
        );
        send_flatbuffers(res, data);
    } else {
        json result = json::object();
        result["total_events"] = loader_.all_events().size();
        result["total_matches"] = loader_.matches().size();
        result["dates"] = loader_.available_dates();
        result["maps"] = json::array();
        for (const auto& cfg : loader_.map_configs()) {
            result["maps"].push_back(cfg.name);
        }
        result["total_human_players"] = total_humans;
        result["total_bot_players"] = total_bots;
        send_json(res, result);
    }
}

void Api::send_json(httplib::Response& res, const json& j, int status) {
    std::string body = j.dump();
    if (body.size() > 1024) {
        body = gzip_compress(body);
        res.set_header("Content-Encoding", "gzip");
    }
    res.set_content(body, "application/json");
    res.status = status;
}

void Api::send_flatbuffers(httplib::Response& res, const std::vector<uint8_t>& data, int status) {
    std::string body(reinterpret_cast<const char*>(data.data()), data.size());
    if (body.size() > 1024) {
        body = gzip_compress(body);
        res.set_header("Content-Encoding", "gzip");
    }
    res.set_content(body, "application/flatbuffers");
    res.status = status;
}

void Api::send_error(httplib::Response& res, const std::string& msg, int status) {
    json err = {{"error", msg}};
    send_json(res, err, status);
}
