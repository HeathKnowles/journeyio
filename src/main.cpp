#include <iostream>
#include <string>
#include <chrono>
#include <httplib.h>
#include "data_loader.h"
#include "api.h"

int main(int argc, char* argv[]) {
    std::string data_dir = "../player_data";
    std::string static_dir = "../frontend/dist";
    int port = 8080;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--data" && i + 1 < argc) data_dir = argv[++i];
        else if (arg == "--static" && i + 1 < argc) static_dir = argv[++i];
        else if (arg == "--port" && i + 1 < argc) port = std::stoi(argv[++i]);
    }

    std::cout << "=== Player Journey Visualization Server ===" << std::endl;
    std::cout << "Data directory: " << data_dir << std::endl;
    std::cout << "Static directory: " << static_dir << std::endl;
    std::cout << "Port: " << port << std::endl;
    std::cout << std::endl;

    std::cout << "Loading parquet data..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    DataLoader loader(data_dir);
    loader.load_all();
    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Data loaded in " << ms << "ms" << std::endl;
    std::cout << std::endl;

    httplib::Server server;
    Api api(loader, static_dir);
    api.register_routes(server);

    std::cout << "Server starting on http://localhost:" << port << std::endl;
    std::cout << "Press Ctrl+C to stop." << std::endl;

    server.listen("0.0.0.0", port);
    return 0;
}
