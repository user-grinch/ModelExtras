#pragma once
#include <map>
#include <json.hpp>

class DataMgr {
public:
    static inline std::map <int, nlohmann::json> data;

    static void Init();

    // template<typename T>
    // static T Get(std::string section, std::string key, T def) {
    //     if (data.contains(section) && data[key].contains(key)) {
    //         return data[section][key];
    //     }
    //     return def;
    // }
};