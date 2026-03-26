#pragma once
#include "IEditor.hpp"

class ExhaustsModule : public IEditorModule {
public:
    const char* GetName() const override { return "Exhausts"; }

    bool Render(json& root) override {
        bool dirty = false;
        if (!root.contains("exhausts")) root["exhausts"] = json::object();
        json& exhausts = root["exhausts"];

        // Top Action Bar
        if (ImGui::Button("+ Add Exhaust", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            AddNewExhaust(exhausts);
            dirty = true;
        }

        ImGui::Separator();
        ImGui::Spacing();

        std::string keyToDelete = "";

        // Iterate through exhausts
        if (exhausts.empty()) {
            ImGui::TextDisabled("No custom exhausts defined.");
        } else {
            for (auto& [key, val] : exhausts.items()) {
                ImGui::PushID(key.c_str());

                // Framed headers make the list easier to read
                if (ImGui::TreeNodeEx(key.c_str(), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowOverlap)) {

                    dirty |= DrawExhaustEditor(val);

                    ImGui::Spacing();
                    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
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

        if (!keyToDelete.empty()) {
            exhausts.erase(keyToDelete);
            dirty = true;
        }

        return dirty;
    }

private:
    void AddNewExhaust(json& exhausts) {
        std::string name = "x_exhaust";
        int suffix = 1;
        while (exhausts.contains(name + std::to_string(suffix))) suffix++;

        name += std::to_string(suffix);
        exhausts[name] = {{"lifetime", 0.5f}, {"speed", 1.0f}, {"size", 1.0f}};
    }

    bool DrawExhaustEditor(json& v) {
        bool changed = false;

        // Use local floats for the widgets
        float fL = v.value("lifetime", 0.5f);
        float fS = v.value("speed", 1.0f);
        float fSz = v.value("size", 1.0f);

        // Sliders are better for visual range
        // Usage: SliderFloat("Label", &value, min, max, "format")
        if (ImGui::SliderFloat("Lifetime", &fL, 0.0f, 2.0f, "%.2f s"))  { v["lifetime"] = fL; changed = true; }
        if (ImGui::SliderFloat("Speed",    &fS, 0.0f, 5.0f, "%.1f x"))  { v["speed"] = fS;    changed = true; }
        if (ImGui::SliderFloat("Size",     &fSz, 0.1f, 10.0f, "%.1f"))  { v["size"] = fSz;   changed = true; }

        return changed;
    }
};