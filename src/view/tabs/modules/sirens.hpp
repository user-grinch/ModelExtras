#pragma once
#include "IEditor.hpp"
#include <string>
#include <vector>
#include <sstream>
#include <numeric> // For std::accumulate
#include <cstdio>

class SirensModule : public IEditorModule {
    std::string selectedGroup;
    std::string selectedSirenId;
    bool requestScrollToBottom = false; // UX: Auto-scroll when adding items

    // --- Helpers ---
    std::vector<int> ParsePatternString(const std::string& str) {
        std::vector<int> pattern;
        std::stringstream ss(str);
        std::string segment;
        while (std::getline(ss, segment, ',')) {
            try {
                segment.erase(0, segment.find_first_not_of(" \t\n\r\f\v"));
                if(!segment.empty()) pattern.push_back(std::stoi(segment));
            } catch(...) {}
        }
        return pattern;
    }

    std::string PatternToString(const json& arr) {
        std::string s = "";
        if (arr.is_array()) {
            for (size_t i = 0; i < arr.size(); ++i) {
                if (arr[i].is_number()) s += std::to_string(arr[i].get<int>()) + (i < arr.size() - 1 ? ", " : "");
            }
        }
        return s;
    }

public:
    const char* GetName() const override { return "Sirens"; }

    bool Render(json& root) override {
        bool dirty = false;
        if (!root.contains("sirens")) root["sirens"] = json::object();
        json& sirens = root["sirens"];

        // --- 1. HEADER & GLOBALS ---
        // Used a cleaner "framed" header style
        if (ImGui::TreeNodeEx("Global Configuration", ImGuiTreeNodeFlags_Framed)) {
            bool imveh = sirens.value("ImVehFt", false);
            if (ImGui::Checkbox("ImVehFt Compatibility Mode", &imveh)) { sirens["ImVehFt"] = imveh; dirty = true; }
            ImGui::SameLine(); DrawHelpMarker("Enable this if the vehicle model is adapted for IVF.");
            ImGui::TreePop();
        }

        ImGui::Spacing();

        // --- 2. GROUPS (STATES) ---
        if (!sirens.contains("states")) sirens["states"] = json::object();
        json& states = sirens["states"];

        ImGui::TextDisabled("Siren Groups (Switch with '1-9' keys)");

        bool openGroupPopup = false;

        if (ImGui::BeginTabBar("SirenGroups", ImGuiTabBarFlags_AutoSelectNewTabs)) {
            // "Add Group" Tab Button
            if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip)) {
                openGroupPopup = true;
            }

            for (auto& [groupName, groupData] : states.items()) {
                if (ImGui::BeginTabItem(groupName.c_str())) {
                    selectedGroup = groupName;
                    dirty |= RenderGroupEditor(groupData);
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }

        // --- POPUP HANDLING ---
        if (openGroupPopup) ImGui::OpenPopup("NewGroupPopup");

        if (ImGui::BeginPopup("NewGroupPopup")) {
            ImGui::Text("Create New Siren State");
            ImGui::Separator();
            static char newGrpName[32] = "State 2";
            ImGui::InputText("Name", newGrpName, 32);

            if (ImGui::Button("Create Group", ImVec2(-1, 0))) {
                if (!states.contains(newGrpName)) {
                    states[newGrpName] = json::object();
                    dirty = true;
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }

        return dirty;
    }

private:
    bool RenderGroupEditor(json& groupData) {
        bool dirty = false;

        // Split Layout: [ List (25%) ] | [ Details (75%) ]
        if (ImGui::BeginTable("GroupTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Siren List", ImGuiTableColumnFlags_WidthFixed, 160.0f);
            ImGui::TableSetupColumn("Properties", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextRow();

            // --- COLUMN 0: LIST ---
            ImGui::TableSetColumnIndex(0);

            // Smart Add Button (Finds next ID automatically)
            if (ImGui::Button("+ Add Siren", ImVec2(-1, 30))) {
                int nextId = 1;
                while (groupData.contains(std::to_string(nextId))) nextId++;

                std::string idStr = std::to_string(nextId);
                groupData[idStr] = {
                    {"color", {{"red", 255}, {"green", 0}, {"blue", 0}, {"alpha", 255}}},
                    {"size", 0.4},
                    {"state", 1},
                    {"pattern", json::array({100, 100})} // Default blink
                };
                selectedSirenId = idStr;
                requestScrollToBottom = true;
                dirty = true;
            }

            ImGui::BeginChild("SirenListScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

            std::string toDelete = "";
            for (auto& [id, data] : groupData.items()) {
                // Color Dot Indicator
                ImVec4 indicatorCol = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
                if (data.contains("color") && data["color"].is_object()) {
                    indicatorCol = ImVec4(
                        data["color"].value("red", 255)/255.0f,
                        data["color"].value("green", 255)/255.0f,
                        data["color"].value("blue", 255)/255.0f, 1.0f);
                }

                ImGui::ColorButton("##ind", indicatorCol, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs, ImVec2(10, 10));
                ImGui::SameLine();

                // Selectable Item
                std::string label = "Siren " + id;
                if (ImGui::Selectable(label.c_str(), selectedSirenId == id)) selectedSirenId = id;

                // Context Menu
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Duplicate")) {
                        // Cloning logic
                        int copyId = 1;
                        while (groupData.contains(std::to_string(copyId))) copyId++;
                        groupData[std::to_string(copyId)] = data; // Deep copy
                        dirty = true;
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Delete")) toDelete = id;
                    ImGui::EndPopup();
                }
            }

            if (requestScrollToBottom) {
                ImGui::SetScrollHereY(1.0f);
                requestScrollToBottom = false;
            }

            ImGui::EndChild();

            if (!toDelete.empty()) {
                groupData.erase(toDelete);
                if (selectedSirenId == toDelete) selectedSirenId = "";
                dirty = true;
            }

            // --- COLUMN 1: DETAILS ---
            ImGui::TableSetColumnIndex(1);
            if (!selectedSirenId.empty() && groupData.contains(selectedSirenId)) {
                dirty |= RenderSirenDetails(groupData[selectedSirenId]);
            } else {
                ImGui::SetCursorPosY(ImGui::GetContentRegionAvail().y * 0.4f);
                ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x * 0.3f);
                ImGui::TextDisabled("Select a siren to edit properties");
            }

            ImGui::EndTable();
        }
        return dirty;
    }

    bool RenderSirenDetails(json& s) {
        bool dirty = false;

        // Toolbar
        ImGui::TextColored(ImVec4(1, 0.8f, 0.2f, 1), "ID: %s", selectedSirenId.c_str());
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 100);
        if (ImGui::Button("Clone", ImVec2(50, 0))) { /* Clone handled in list, but could be here */ }
        ImGui::SameLine();
        if (ImGui::Button("Delete", ImVec2(50, 0))) { /* Delete handled in list */ }

        ImGui::Separator();

        // 1. VISUAL TIMELINE (The new feature)
        DrawPatternTimeline(s);

        // 2. TIMING & PATTERN CARD
        if (ImGui::TreeNodeEx("Timing Configuration", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) {
            // Pattern String Input
            std::string patStr = PatternToString(s["pattern"]);
            char buf[256]; snprintf(buf, sizeof(buf), "%s", patStr.c_str());

            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputTextWithHint("##pat", "e.g. 100, 100, 500", buf, sizeof(buf))) {
                s["pattern"] = ParsePatternString(buf);
                dirty = true;
            }
            ImGui::TextDisabled("Pattern Sequence (ms)");

            ImGui::Spacing();

            // Sliders
            int delay = s.value("delay", 0);
            if (ImGui::DragInt("Start Delay", &delay, 10, 0, 5000, "%d ms")) { s["delay"] = delay; dirty = true; }

            float inertia = s.value("inertia", 0.0f);
            if (ImGui::SliderFloat("Inertia (Fade)", &inertia, 0.0f, 5.0f, "%.1f")) { s["inertia"] = inertia; dirty = true; }

            ImGui::TreePop();
        }

        ImGui::Spacing();

        // 3. APPEARANCE CARD
        if (ImGui::TreeNodeEx("Appearance", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) {
            // Color with alpha bar
            if (!s.contains("color")) s["color"] = json::object();

            if (s["color"].is_object()) {
                dirty |= DrawJsonColorPicker("Light Color", s["color"], ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf);
            } else if (s["color"].is_string()) {
                ImGui::TextColored(ImVec4(0.4f, 1, 0.4f, 1), "Linked Reference: %s", s["color"].get<std::string>().c_str());
                if (ImGui::SmallButton("Unlink (Convert to RGBA)")) {
                    s["color"] = {{"red", 255}, {"green", 0}, {"blue", 0}, {"alpha", 255}};
                    dirty = true;
                }
            }

            ImGui::Columns(2, "AppCols", false); // 2-column layout for sliders

            float size = s.value("size", 0.4f);
            if (ImGui::SliderFloat("Size", &size, 0.1f, 5.0f, "%.2f")) { s["size"] = size; dirty = true; }

            ImGui::NextColumn();

            int startState = s.value("state", 1);
            if (ImGui::Checkbox("Starts Active", (bool*)&startState)) { s["state"] = startState ? 1 : 0; dirty = true; }

            ImGui::Columns(1);
            ImGui::TreePop();
        }

        ImGui::Spacing();

        // 4. TYPE & ROTATOR CARD
        if (ImGui::TreeNodeEx("Type & Rotation", ImGuiTreeNodeFlags_Framed)) {
            const char* types[] = { "directional", "non-directional", "inversed-directional", "rotator" };
            std::string currentType = s.value("type", "directional");

            int typeIdx = 0;
            if (currentType == "non-directional") typeIdx = 1;
            else if (currentType == "inversed-directional") typeIdx = 2;
            else if (currentType == "rotator") typeIdx = 3;

            if (ImGui::Combo("Light Mode", &typeIdx, types, IM_ARRAYSIZE(types))) {
                s["type"] = types[typeIdx];
                if (typeIdx == 3 && !s.contains("rotator")) {
                    s["rotator"] = {{"time", 1000}, {"radius", 360}, {"type", "linear"}};
                }
                dirty = true;
            }

            // Specific Rotator UI
            if (typeIdx == 3) {
                ImGui::Indent();
                ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Rotator Settings");
                json& rot = s["rotator"];

                int time = rot.value("time", 1000);
                if (ImGui::DragInt("Cycle Time", &time, 10, 10, 5000, "%d ms")) { rot["time"] = time; dirty = true; }

                float offset = rot.value("offset", 0.0f);
                if (ImGui::SliderFloat("Start Angle", &offset, 0.0f, 360.0f, "%.0f deg")) { rot["offset"] = offset; dirty = true; }

                const char* dirs[] = { "Clockwise", "Counter-CW", "Switch" };
                int dirVal = 0;
                if (rot.contains("direction") && rot["direction"] == "counter-clockwise") dirVal = 1;
                else if (rot.contains("direction") && rot["direction"] == "switch") dirVal = 2;

                if (ImGui::Combo("Direction", &dirVal, dirs, IM_ARRAYSIZE(dirs))) {
                    if(dirVal == 0) rot["direction"] = "clockwise";
                    else if(dirVal == 1) rot["direction"] = "counter-clockwise";
                    else rot["direction"] = "switch";
                    dirty = true;
                }
                ImGui::Unindent();
            }
            ImGui::TreePop();
        }

        return dirty;
    }

    // --- VISUALIZERS & HELPERS ---

    // Draw a visual timeline of the flash pattern
    void DrawPatternTimeline(json& s) {
        if (!s.contains("pattern") || !s["pattern"].is_array() || s["pattern"].empty()) return;

        ImGui::Spacing();
        ImGui::Text("Pattern Preview:");

        // Get draw list
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        float width = ImGui::GetContentRegionAvail().x;
        float height = 20.0f;

        // Calculate total duration
        std::vector<int> timings;
        for (auto& t : s["pattern"]) if(t.is_number()) timings.push_back(t.get<int>());
        int totalTime = std::accumulate(timings.begin(), timings.end(), 0);
        if (totalTime <= 0) totalTime = 1000;

        // Background
        draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), IM_COL32(40, 40, 40, 255));

        // Draw Segments
        float currentX = 0.0f;
        int initialState = s.value("state", 1);
        bool isOn = (initialState == 1);

        for (int t : timings) {
            float segmentWidth = ((float)t / totalTime) * width;

            if (isOn) {
                // Determine color based on JSON color or default yellow
                ImU32 col = IM_COL32(255, 200, 0, 255);
                if (s.contains("color") && s["color"].is_object()) {
                    int r = s["color"].value("red", 255);
                    int g = s["color"].value("green", 255);
                    int b = s["color"].value("blue", 255);
                    col = IM_COL32(r, g, b, 255);
                }
                draw_list->AddRectFilled(ImVec2(p.x + currentX, p.y), ImVec2(p.x + currentX + segmentWidth, p.y + height), col);
            }

            currentX += segmentWidth;
            isOn = !isOn; // Toggle state
        }

        // Border
        draw_list->AddRect(p, ImVec2(p.x + width, p.y + height), IM_COL32(100, 100, 100, 255));
        ImGui::Dummy(ImVec2(width, height + 5)); // Advance cursor
    }

    bool DrawJsonColorPicker(const char* label, json& colorNode, ImGuiColorEditFlags flags = 0) {
        float col[4] = {
            colorNode.value("red", 255) / 255.0f,
            colorNode.value("green", 255) / 255.0f,
            colorNode.value("blue", 255) / 255.0f,
            colorNode.value("alpha", 255) / 255.0f
        };
        if (ImGui::ColorEdit4(label, col, flags)) {
            colorNode["red"] = (int)(col[0] * 255.0f);
            colorNode["green"] = (int)(col[1] * 255.0f);
            colorNode["blue"] = (int)(col[2] * 255.0f);
            colorNode["alpha"] = (int)(col[3] * 255.0f);
            return true;
        }
        return false;
    }

    void DrawHelpMarker(const char* desc) {
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
};