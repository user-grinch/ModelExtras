#pragma once
#include <unordered_map>
#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <filesystem>
#include <nlohmann/json.hpp>

using ModelDataListener_t = std::function<void(int model, const nlohmann::json &data)>;

class DataMgr
{
private:
    static inline std::unordered_map<int, nlohmann::json> data;
    static inline std::unordered_map<int, std::string> modelPath;
    static inline std::vector<std::pair<std::string, ModelDataListener_t>> listeners;

public:
    static void Init();
    static void Convert();
    static void LoadFile(const std::filesystem::directory_entry &entry);
    static void Reload(int model);
    static bool Has(int model);
    static const nlohmann::json* Find(int model);
    static nlohmann::json &Get(int model);
    static const std::string &GetPath(int model);
    static void RegisterListener(std::string_view name, ModelDataListener_t listener);
};
