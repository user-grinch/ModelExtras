#pragma once
#include <nlohmann/json.hpp>

namespace StudioLive {
    void ApplyIni(const char *section, const char *key);
    void ApplyModel(int model, const nlohmann::json &before, const nlohmann::json &after);
}
