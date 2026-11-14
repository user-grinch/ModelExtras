#pragma once
#include "studio_config.h"
#include "studio_support.h"
#include <imgui.h>
#include <string>
#include <string_view>
#include <vector>

struct ImGuiTextFilter;

namespace StudioWorkshop {
using StudioConfig::Json;

std::string Label(std::string_view key);
bool Combo(Json &node, const char *label, const std::vector<std::string_view> &choices);
bool RemoveFeatureButton(const ImVec2 &rowStart, float right);
bool Edit(Json &node, std::vector<std::string> &path, const Json &root, ImGuiTextFilter &filter,
          const std::vector<std::string> &frames, const std::vector<std::string> &audio, unsigned &shown,
          const StudioSupport::Model &support, bool *remove = nullptr);

} // namespace StudioWorkshop
