#include "pch.h"
#include "datamgr.h"
#include <string>

void DataMgr::Init() {
    std::string path = MOD_DATA_PATH("data/");

    for (const auto& e : std::filesystem::directory_iterator(path)) {
        if (e.is_regular_file() && e.path().extension() == ".json") {
            std::string filename = e.path().filename().string();
            int key = std::stoi(e.path().stem().string());
            std::ifstream file(e.path());
            try {
                data[key] = nlohmann::json::parse(file);
                LOG_VERBOSE("Successfully registered file '{}'", e.path().filename().string());
            }
            catch (const nlohmann::json::parse_error& ex) {
                gLogger->error("Failed to parse JSON in file '{}': {}", e.path().string(), ex.what());
            }
        }
    }
}
