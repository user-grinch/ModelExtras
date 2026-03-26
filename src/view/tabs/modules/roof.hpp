#pragma once
#include "IEditor.hpp"

class RoofModule : public IEditorModule {
public:
    const char* GetName() const override { return "Roofs"; }

    bool Render(json& root) override {
        bool dirty = false;

        // Ensure the container exists
        if (!root.contains("roofs")) root["roofs"] = json::object();
        json& roofs = root["roofs"];

        // --- Quick Add Buttons ---
        float buttonWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

        if (ImGui::Button("+ Add Roof", ImVec2(buttonWidth, 0))) {
            AddNewPart(roofs, "x_convertible_roof");
            dirty = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("+ Add Boot", ImVec2(buttonWidth, 0))) {
            AddNewPart(roofs, "x_convertible_boot");
            dirty = true;
        }

        ImGui::Separator();
        ImGui::Spacing();

        // --- List Active Components ---
        std::string toDelete = "";

        if (!roofs.empty()) {
            for (auto& [key, val] : roofs.items()) {
                ImGui::PushID(key.c_str());

                // Framed headers for clear separation
                if (ImGui::TreeNodeEx(key.c_str(), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) {

                    dirty |= DrawRoofControls(val);

                    ImGui::Spacing();
                    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.5f));
                    if (ImGui::Button("Remove", ImVec2(-1, 0))) {
                        toDelete = key;
                    }
                    ImGui::PopStyleColor();

                    ImGui::TreePop();
                }

                ImGui::PopID();
                ImGui::Spacing();
            }
        } else {
            ImGui::TextDisabled("No convertible roofs defined.");
        }

        // Safe deletion
        if (!toDelete.empty()) {
            roofs.erase(toDelete);
            dirty = true;
        }

        return dirty;
    }

private:
    void AddNewPart(json& roofs, const std::string& baseName) {
        std::string finalName = baseName;
        int suffix = 1;

        // Auto-incrementing name logic
        while (roofs.contains(finalName + (suffix == 1 ? "" :  std::to_string(suffix)))) {
            suffix++;
        }

        if (suffix > 1) finalName += std::to_string(suffix);

        roofs[finalName] = {
            {"rotation", 90.0f},
            {"speed", 1.0f}
        };
    }

    bool DrawRoofControls(json& val) {
        bool changed = false;

        float fRot = val.value("rotation", 90.0f);
        float fSpd = val.value("speed", 1.0f);

        if (ImGui::SliderFloat("Max Rotation", &fRot, -180.0f, 180.0f, "%.1f deg")) {
            val["rotation"] = fRot;
            changed = true;
        }

        if (ImGui::SliderFloat("Anim Speed", &fSpd, 0.1f, 5.0f, "x%.2f")) {
            val["speed"] = fSpd;
            changed = true;
        }

        return changed;
    }
};