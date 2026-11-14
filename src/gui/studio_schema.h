#pragma once
#include "studio_config.h"
#include <array>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// Editor vocabulary follows the actual JSONC consumers. Defaults are suggestions,
// never inserted merely by viewing a page (DFF/INI defaults may be different).
namespace StudioSchema {
using StudioConfig::Json;
inline constexpr std::array<std::pair<int, std::string_view>, 6> MinimumVersions = {{{10600, "v1.6"}, {10700, "v1.7"}, {20000, "v2.0"}, {20100, "v2.1"}, {30000, "v3.0"}, {30100, "v3.1"}}};

const Json &Color();
const Json &Material();
const Json &PointLight();
const Json &Light();
const Json &SirenMaterial();
void AddField(Json &node, const std::vector<std::string> &path, const std::string &name, const Json &value);
inline bool Prefix(std::string_view name, std::string_view prefix) { return name.starts_with(prefix); }
const char *Help(std::string_view section);
Json NodeFields(std::string_view section, std::string_view name);
const Json &Sections();
Json Fields(const std::vector<std::string> &p);
std::pair<double, double> NumericRange(const std::vector<std::string> &path, const Json &node, const Json &root);
bool IsColor(const Json &v, const std::vector<std::string> &p);
bool DecodeColor(const Json &v, float *rgba, const Json &refs, unsigned depth = 0);
void EncodeColor(Json &v, const float *rgba);
Json ObjectColor(const Json &value);
std::vector<std::string_view> Choices(const std::vector<std::string> &p);
} // namespace StudioSchema
