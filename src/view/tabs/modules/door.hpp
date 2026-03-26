#pragma once
#include "IEditor.hpp"

class DoorsModule : public IEditorModule {
    struct DoorType { const char* label; const char* key; };

public:
    const char* GetName() const override { return "Doors"; }

    bool Render(json& root) override {
        bool dirty = false;
        if (!root.contains("doors")) root["doors"] = json::object();
        json& doors = root["doors"];

        // --- Quick Add Section ---
        float btnWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

        auto DrawAddButton = [&](const char* label, const char* key) {
            if (ImGui::Button(label, ImVec2(btnWidth, 0))) {
                AddDoorNode(doors, key);
                dirty = true;
            }
        };

        if (ImGui::TreeNodeEx("Add Rotating Doors", ImGuiTreeNodeFlags_DefaultOpen)) {
            DrawAddButton("Front Left", "x_rd_lf"); ImGui::SameLine(); DrawAddButton("Front Right", "x_rd_rf");
            DrawAddButton("Rear Left",  "x_rd_lr"); ImGui::SameLine(); DrawAddButton("Rear Right",  "x_rd_rr");
            DrawAddButton("Bonnet", "x_rd_bonnet"); ImGui::SameLine(); DrawAddButton("Boot", "x_rd_boot");
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Add Sliding Doors")) {
            DrawAddButton("Slide Front Left", "x_sd_lf"); ImGui::SameLine(); DrawAddButton("Slide Front Right", "x_sd_rf");
            DrawAddButton("Slide Rear Left",  "x_sd_lr"); ImGui::SameLine(); DrawAddButton("Slide Rear Right",  "x_sd_rr");
            ImGui::TreePop();
        }

        ImGui::Separator();
        ImGui::Spacing();

        // --- List Active Doors ---
        std::string toDelete = "";
        if (!doors.empty()) {
            for (auto& [key, val] : doors.items()) {
                ImGui::PushID(key.c_str());

                if (ImGui::TreeNodeEx(key.c_str(), ImGuiTreeNodeFlags_Framed)) {
                    dirty |= DrawDoorControls(val);

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
            ImGui::TextDisabled("No custom doors defined.");
        }

        if (!toDelete.empty()) {
            doors.erase(toDelete);
            dirty = true;
        }

        return dirty;
    }

private:
    void AddDoorNode(json& doors, const std::string& base) {
        std::string finalName = base;
        int suffix = 1;
        // Check for base name first, then append suffixes
        while (doors.contains(finalName + (suffix == 1 ? "" : std::to_string(suffix)))) {
            suffix++;
        }
        if (suffix > 1) finalName += std::to_string(suffix);

        doors[finalName] = { {"mul", 1.0f}, {"popout", 0.0f} };
    }

    bool DrawDoorControls(json& val) {
        bool changed = false;
        float fMul = val.value("mul", 1.0f);
        float fPop = val.value("popout", 0.0f);

        if (ImGui::SliderFloat("Open Multiplier", &fMul, 0.0f, 2.0f, "%.2f x")) {
            val["mul"] = fMul;
            changed = true;
        }

        if (ImGui::SliderFloat("Pop-out/Slide", &fPop, -1.0f, 1.0f, "%.2f m")) {
            val["popout"] = fPop;
            changed = true;
        }

        return changed;
    }
};