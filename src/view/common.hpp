#pragma once
#include <imgui.h>

static inline void DrawVehicleRequiredMessage() {
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Warning: Player must be inside a vehicle.");
    ImGui::TextDisabled("Please enter a vehicle to access these tools.");
}