#pragma once
#include <map>
#include <nlohmann\json.hpp>

class DataMgr
{
private:
    static inline std::map<int, nlohmann::json> data;
    static inline std::map<int, std::string> modelPath;

public:
    static void Init();
    static void LoadFile(const std::filesystem::directory_entry &entry);
    static void Reload(int model);
    static nlohmann::json &Get(int model);
    static const std::string &GetPath(int model);
};