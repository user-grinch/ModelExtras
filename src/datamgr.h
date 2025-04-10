#pragma once
#include <map>
#include <json.hpp>

class DataMgr
{
private:
    static inline std::map<int, nlohmann::json> data;

public:
    static void Init();
    static void Parse();
    static nlohmann::json &Get(int model);
};