#pragma once
#include "IEditor.hpp"

class SpoilerModule : public IEditorModule {
public:
    const char* GetName() const override { return "Spoilers"; }

    bool Render(json& root) override {
        bool dirty = false;

        if (!root.contains("spoilers")) root["spoilers"] = json::object();
        json& spoilers = root["spoilers"];

        // --- Action Header ---
        if (ImGui::Button("+ Add Spoiler", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            AddNewSpoiler(spoilers);
            dirty = true;
        }

        ImGui::Separator();
        ImGui::Spacing();

        // --- List Section ---
        std::string keyToDelete = "";

        if (spoilers.empty()) {
            ImGui::TextDisabled("No spoilers defined.");
        } else {
            for (auto& [key, val] : spoilers.items()) {
                ImGui::PushID(key.c_str());

                // Framed node for visual "card" look
                if (ImGui::TreeNodeEx(key.c_str(), ImGuiTreeNodeFlags_Framed)) {

                    dirty |= DrawSpoilerControls(val);

                    ImGui::Spacing();
                    ImGui::Separator();

                    // Full-width Delete button inside the node
                    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.5f));
                    if (ImGui::Button("Remove", ImVec2(-1, 0))) {
                        keyToDelete = key;
                    }
                    ImGui::PopStyleColor();

                    ImGui::TreePop();
                }

                ImGui::PopID();
                ImGui::Spacing();
            }
        }

        // Safe deletion outside the loop
        if (!keyToDelete.empty()) {
            spoilers.erase(keyToDelete);
            dirty = true;
        }

        return dirty;
    }

private:
    void AddNewSpoiler(json& spoilers) {
        std::string name = "movspoiler";
        int suffix = 1;
        // Check for existing names to avoid collisions
        while (spoilers.contains(name + std::to_string(suffix))) {
            suffix++;
        }

        std::string finalName = name + std::to_string(suffix);
        spoilers[finalName] = {
            {"rotation", 30.0f},
            {"time", 500},
            {"triggerspeed", 80}
        };
    }

    bool DrawSpoilerControls(json& val) {
        bool changed = false;

        float rot  = val.value("rotation", 30.0f);
        int time   = val.value("time", 500);
        int speed  = val.value("triggerspeed", 80);

        // Sliders with units for better UX
        if (ImGui::SliderFloat("Max Rotation", &rot, 0.0f, 90.0f, "%.1f deg")) {
            val["rotation"] = rot;
            changed = true;
        }

        if (ImGui::SliderInt("Anim Time", &time, 0, 5000, "%d ms")) {
            val["time"] = time;
            changed = true;
        }

        if (ImGui::SliderInt("Trigger Spd", &speed, 0, 400, "%d km/h")) {
            val["triggerspeed"] = speed;
            changed = true;
        }

        return changed;
    }
};