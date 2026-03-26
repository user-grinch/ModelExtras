#pragma once
#include "IEditor.hpp"
#include <map>
#include <string>
#include <vector>

class LightsModule : public IEditorModule {
    std::string selectedLight;

    struct LightTemplate {
        const char* node;
        json data;
    };

    // --- STATIC DATA SOURCE (Prevents Heap Overflow) ---
    static const std::map<std::string, std::map<std::string, LightTemplate>>& GetTemplates() {
        static std::map<std::string, std::map<std::string, LightTemplate>> templates;

        if (templates.empty()) {
            // 1. Turn Signals
            auto& turn = templates["Turn Signals"];
            turn["Front Left"]   = { "indicator_lf", {{"corona", {{"color", {{"red", 183}, {"green", 255}, {"blue", 0}, {"alpha", 200}}}, {"size", 0.5}, {"type", "directional"}}}} };
            turn["Front Right"]  = { "indicator_rf", {{"corona", {{"color", {{"red", 255}, {"green", 58},  {"blue", 0}, {"alpha", 200}}}, {"size", 0.5}, {"type", "directional"}}}} };
            turn["Middle Left"]  = { "indicator_lm", {{"corona", {{"color", {{"red", 182}, {"green", 255}, {"blue", 0}, {"alpha", 200}}}, {"size", 0.4}, {"type", "directional"}}}} };
            turn["Middle Right"] = { "indicator_rm", {{"corona", {{"color", {{"red", 255}, {"green", 57},  {"blue", 0}, {"alpha", 200}}}, {"size", 0.4}, {"type", "directional"}}}} };
            turn["Rear Left"]    = { "indicator_lr", {{"corona", {{"color", {{"red", 181}, {"green", 255}, {"blue", 0}, {"alpha", 200}}}, {"size", 0.5}, {"type", "directional"}}}} };
            turn["Rear Right"]   = { "indicator_rr", {{"corona", {{"color", {{"red", 255}, {"green", 56},  {"blue", 0}, {"alpha", 200}}}, {"size", 0.5}, {"type", "directional"}}}} };

            // 2. Brake & Reverse
            auto& brake = templates["Brake & Reverse"];
            brake["Standard Brake L"] = { "breaklight_l", {{"corona", {{"color", {{"red", 184}, {"green", 255}, {"blue", 0}, {"alpha", 200}}}, {"size", 0.5}, {"type", "directional"}}}} };
            brake["Standard Brake R"] = { "breaklight_r", {{"corona", {{"color", {{"red", 255}, {"green", 59},  {"blue", 0}, {"alpha", 200}}}, {"size", 0.5}, {"type", "directional"}}}} };
            brake["Reverse L"]        = { "reversingl_l", {{"corona", {{"color", {{"red", 255}, {"green", 173}, {"blue", 0}, {"alpha", 200}}}, {"size", 0.5}, {"type", "directional"}}}} };
            brake["Reverse R"]        = { "reversingl_r", {{"corona", {{"color", {{"red", 0},   {"green", 255}, {"blue", 198},{"alpha", 200}}}, {"size", 0.5}, {"type", "directional"}}}} };
            brake["NA Brake L (US)"]  = { "nabrakelight_l",{{"corona", {{"color", {{"red", 255}, {"green", 200}, {"blue", 5}, {"alpha", 200}}}, {"size", 0.5}, {"type", "directional"}}}} };
            brake["NA Brake R (US)"]  = { "nabrakelight_r",{{"corona", {{"color", {{"red", 255}, {"green", 200}, {"blue", 6}, {"alpha", 200}}}, {"size", 0.5}, {"type", "directional"}}}} };
            brake["STT L (Retro)"]    = { "sttlight_l",    {{"corona", {{"color", {{"red", 255}, {"green", 200}, {"blue", 3}, {"alpha", 200}}}, {"size", 0.5}, {"type", "directional"}}}} };
            brake["STT R (Retro)"]    = { "sttlight_r",    {{"corona", {{"color", {{"red", 255}, {"green", 200}, {"blue", 4}, {"alpha", 200}}}, {"size", 0.5}, {"type", "directional"}}}} };

            // 3. Driving Lights
            auto& drive = templates["Driving Lights"];
            drive["Fog Light L"]  = { "foglight_l",   {{"corona", {{"color", {{"red", 255}, {"green", 174}, {"blue", 0},   {"alpha", 200}}}, {"size", 0.6}, {"type", "directional"}}}} };
            drive["Fog Light R"]  = { "foglight_r",   {{"corona", {{"color", {{"red", 0},   {"green", 255}, {"blue", 199}, {"alpha", 200}}}, {"size", 0.6}, {"type", "directional"}}}} };
            drive["Side Light L"] = { "sidelight_l",  {{"corona", {{"color", {{"red", 255}, {"green", 200}, {"blue", 1},   {"alpha", 200}}}, {"size", 0.3}, {"type", "directional"}}}, {"shadow", {{"size", 1.0}, {"texture", "pointlight"}}}} };
            drive["Side Light R"] = { "sidelight_r",  {{"corona", {{"color", {{"red", 255}, {"green", 200}, {"blue", 2},   {"alpha", 200}}}, {"size", 0.3}, {"type", "directional"}}}, {"shadow", {{"size", 1.0}, {"texture", "pointlight"}}}} };

            // 4. Time Based
            auto& time = templates["Time-Based"];
            time["Day Only"]   = { "light_day",    {{"corona", {{"color", {{"red", 0},   {"green", 18},  {"blue", 255}, {"alpha", 200}}}, {"size", 0.5}, {"type", "non-directional"}}}} };
            time["Night Only"] = { "light_night",  {{"corona", {{"color", {{"red", 0},   {"green", 16},  {"blue", 255}, {"alpha", 200}}}, {"size", 0.5}, {"type", "non-directional"}}}} };
            time["All Day"]    = { "light_allday", {{"corona", {{"color", {{"red", 0},   {"green", 17},  {"blue", 255}, {"alpha", 200}}}, {"size", 0.5}, {"type", "non-directional"}}}} };

            // 5. Special / Emergency
            auto& special = templates["Special / Emergency"];
            special["Spotlight Dummy"] = { "spotlight_dummy", {{"corona", {{"color", {{"red", 255}, {"green", 200}, {"blue", 7}, {"alpha", 220}}}, {"size", 1.0}, {"type", "directional"}}}, {"shadow", {{"size", 4.0}, {"texture", "spotlight"}}}} };
            special["Strobe 1"]        = { "strobe_light1",   {{"corona", {{"color", {{"red", 255}, {"green", 199}, {"blue", 1}, {"alpha", 255}}}, {"size", 0.8}, {"type", "non-directional"}}}, {"strobedelay", 100}} };
            special["Strobe 2"]        = { "strobe_light2",   {{"corona", {{"color", {{"red", 255}, {"green", 199}, {"blue", 2}, {"alpha", 255}}}, {"size", 0.8}, {"type", "non-directional"}}}, {"strobedelay", 100}} };
            special["Strobe 3"]        = { "strobe_light3",   {{"corona", {{"color", {{"red", 255}, {"green", 199}, {"blue", 3}, {"alpha", 255}}}, {"size", 0.8}, {"type", "non-directional"}}}, {"strobedelay", 100}} };
            special["Strobe 255"]      = { "strobe_light255", {{"corona", {{"color", {{"red", 255}, {"green", 199}, {"blue", 255},{"alpha", 255}}}, {"size", 0.8}, {"type", "non-directional"}}}, {"strobedelay", 100}} };
        }
        return templates;
    }

public:
    const char* GetName() const override { return "Lights"; }

    bool Render(json& root) override {
        bool dirty = false;
        if (!root.contains("lights")) root["lights"] = json::object();
        json& lights = root["lights"];

        // UI Setup
        if (ImGui::BeginTable("LightsLayout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
            ImGui::TableSetupColumn("List", ImGuiTableColumnFlags_WidthFixed, 220.0f);
            ImGui::TableSetupColumn("Editor", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextRow();

            // --- COLUMN 0: SIDEBAR ---
            ImGui::TableSetColumnIndex(0);
            RenderSidebar(lights, dirty);

            // --- COLUMN 1: EDITOR ---
            ImGui::TableSetColumnIndex(1);
            if (!selectedLight.empty() && lights.contains(selectedLight)) {
                dirty |= RenderLightEditor(lights[selectedLight]);
            } else {
                ImGui::SetCursorPosY(ImGui::GetWindowHeight() * 0.4f);
                ImGui::TextDisabled("      Select or add a light dummy to begin.");
            }

            ImGui::EndTable();
        }
        return dirty;
    }

private:
    void RenderSidebar(json& lights, bool& dirty) {
        // --- Organized Dropdown Menu ---
        if (ImGui::Button("+ Add New Light", ImVec2(-1, 30))) ImGui::OpenPopup("AddLightMenu");

        if (ImGui::BeginPopup("AddLightMenu")) {
            for (const auto& catEntry : GetTemplates()) {
                if (ImGui::BeginMenu(catEntry.first.c_str())) {
                    for (const auto& itemEntry : catEntry.second) {
                        if (ImGui::MenuItem(itemEntry.first.c_str())) {

                            // LOGIC UPDATE: Intelligent Suffixing
                            std::string baseName = itemEntry.second.node;
                            std::string finalName = baseName;
                            int suffix = 2;

                            // If "indicator_lf" exists, try "indicator_lf_2", then "_3", etc.
                            while (lights.contains(finalName)) {
                                finalName = baseName + std::to_string(suffix++);
                            }

                            lights[finalName] = itemEntry.second.data;
                            selectedLight = finalName;
                            dirty = true;
                        }
                    }
                    ImGui::EndMenu();
                }
            }
            ImGui::EndPopup();
        }

        ImGui::Separator();

        // --- Scrollable List ---
        ImGui::BeginChild("LightsScrollList");
        std::string toDelete = "";

        for (auto& [key, val] : lights.items()) {
            bool isSelected = (selectedLight == key);

            // Color coding for easier scanning
            if (key.find("strobe") != std::string::npos) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
            else if (key.find("indicator") != std::string::npos || key.find("turn") != std::string::npos) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.4f, 1.0f));
            else ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_Text]);

            if (ImGui::Selectable(key.c_str(), isSelected)) selectedLight = key;
            ImGui::PopStyleColor();

            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Delete Light Node")) toDelete = key;
                ImGui::EndPopup();
            }
        }

        if (!toDelete.empty()) {
            lights.erase(toDelete);
            if (selectedLight == toDelete) selectedLight = "";
            dirty = true;
        }
        ImGui::EndChild();
    }

    bool RenderLightEditor(json& l) {
        bool changed = false;

        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Node: %s", selectedLight.c_str());
        ImGui::Separator();
        ImGui::Spacing();

        // 1. CORONA SECTION
        if (ImGui::TreeNodeEx("Corona", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) {
            if (!l.contains("corona")) l["corona"] = json::object();
            json& c = l["corona"];

            changed |= DrawJsonColorPicker("Corona Color", c["color"]);

            float sz = c.value("size", 0.5f);
            if (ImGui::SliderFloat("Corona Size", &sz, 0.05f, 5.0f, "%.2f")) { c["size"] = sz; changed = true; }

            static const char* types[] = { "directional", "inversed-directional", "non-directional" };
            std::string currType = c.value("type", "directional");
            int typeIdx = 0;
            if (currType == "inversed-directional") typeIdx = 1;
            else if (currType == "non-directional") typeIdx = 2;

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
            if (ImGui::Combo("View Type", &typeIdx, types, IM_ARRAYSIZE(types))) {
                c["type"] = types[typeIdx];
                changed = true;
            }
            ImGui::TreePop();
        }

        ImGui::Spacing();

        // 2. SHADOW SECTION
        bool hasShadow = l.contains("shadow");
        if (ImGui::TreeNodeEx("Shadow", ImGuiTreeNodeFlags_Framed)) {
            if (ImGui::Checkbox("Enable Shadow", &hasShadow)) {
                if (hasShadow) l["shadow"] = {{"size", 1.0f}, {"texture", "pointlight"}};
                else l.erase("shadow");
                changed = true;
            }

            if (hasShadow) {
                json& s = l["shadow"];

                bool hasCustomCol = s.contains("color");
                if (ImGui::Checkbox("Override Color", &hasCustomCol)) {
                    if (hasCustomCol) s["color"] = l["corona"]["color"];
                    else s.erase("color");
                    changed = true;
                }
                if (hasCustomCol) changed |= DrawJsonColorPicker("Shadow Color", s["color"]);

                float sSz = s.value("size", 1.0f);
                if (ImGui::SliderFloat("Shadow Size", &sSz, 0.1f, 10.0f, "%.1f m")) { s["size"] = sSz; changed = true; }

                static const char* textures[] = { "pointlight", "round", "narrow", "foglight", "spotlight", "tightfocused" };
                std::string currentTex = s.value("texture", "pointlight");
                if (ImGui::BeginCombo("Texture", currentTex.c_str())) {
                    for (auto& t : textures) {
                        if (ImGui::Selectable(t, currentTex == t)) { s["texture"] = t; changed = true; }
                    }
                    ImGui::EndCombo();
                }

                bool rot = s.value("rotationchecks", false);
                if (ImGui::Checkbox("Allow 360 Projection", &rot)) { s["rotationchecks"] = rot; changed = true; }
                ImGui::SameLine(); DrawHelpMarker("Disables outward-only limits. Good for interior lights.");
            }
            ImGui::TreePop();
        }

        ImGui::Spacing();

        // 3. SPECIAL EFFECTS
        if (ImGui::TreeNodeEx("Special Effects", ImGuiTreeNodeFlags_Framed)) {
            int delay = l.value("strobedelay", 0);
            if (ImGui::SliderInt("Strobe Delay", &delay, 0, 2000, "%d ms")) {
                l["strobedelay"] = delay;
                changed = true;
            }
            ImGui::SameLine(); DrawHelpMarker("Delay for strobe flashing (0 = disabled).");
            ImGui::TreePop();
        }

        return changed;
    }

    // --- HELPER FUNCTIONS ---
    bool DrawJsonColorPicker(const char* label, json& colorNode) {
        float col[4] = {
            colorNode.value("red", 255) / 255.0f,
            colorNode.value("green", 255) / 255.0f,
            colorNode.value("blue", 255) / 255.0f,
            colorNode.value("alpha", 255) / 255.0f
        };

        if (ImGui::ColorEdit4(label, col)) {
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