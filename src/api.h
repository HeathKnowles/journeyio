#pragma once

#include "data_loader.h"
#include "flatbuf_serializer.h"
#include <httplib.h>

class Api {
public:
    Api(DataLoader& loader, const std::string& static_dir);

    void register_routes(httplib::Server& server);

private:
    DataLoader& loader_;
    std::string static_dir_;
    FlatBufferSerializer fb_serializer_;

    bool wants_flatbuffers(const httplib::Request& req) const;

    void handle_get_maps(const httplib::Request& req, httplib::Response& res);
    void handle_get_dates(const httplib::Request& req, httplib::Response& res);
    void handle_get_matches(const httplib::Request& req, httplib::Response& res);
    void handle_get_match_detail(const httplib::Request& req, httplib::Response& res);
    void handle_get_heatmap(const httplib::Request& req, httplib::Response& res);
    void handle_get_all_heatmaps(const httplib::Request& req, httplib::Response& res);
    void handle_get_stats(const httplib::Request& req, httplib::Response& res);

    void send_json(httplib::Response& res, const json& j, int status = 200);
    void send_flatbuffers(httplib::Response& res, const std::vector<uint8_t>& data, int status = 200);
    void send_error(httplib::Response& res, const std::string& msg, int status = 400);
};
