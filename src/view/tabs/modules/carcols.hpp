#pragma once
#include "IEditor.hpp"

class CarcolsModule : public IEditorModule {
public:
    const char* GetName() const override { return "Carcols"; }

    bool Render(json& root) override {
        bool dirty = false;

        if (!root.contains("carcols")) root["carcols"] = json::object();
        json& carcols = root["carcols"];
        if (!carcols.contains("colors")) carcols["colors"] = json::array();
        if (!carcols.contains("variations")) carcols["variations"] = json::array();

        ImGui::Spacing();

        if (ImGui::BeginTabBar("CarcolsTabs", ImGuiTabBarFlags_None)) {

            // --- TAB 1: COLOR PALETTE ---
            if (ImGui::BeginTabItem("Colors")) {
                ImGui::Spacing();

                if (ImGui::Button("+ Add New", ImVec2(-1, 40))) {
                    carcols["colors"].push_back({ {"red", 255}, {"green", 255}, {"blue", 255} });
                    dirty = true;
                }

                ImGui::Separator();
                // Adding a bit of padding for the scrollbar to prevent the "Delete" button clipping
                ImGui::BeginChild("PaletteScroll", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                ImGui::Spacing();

                int colorToDelete = -1;
                for (int i = 0; i < (int)carcols["colors"].size(); ++i) {
                    ImGui::PushID(i);
                    auto& c = carcols["colors"][i];

                    float col[3] = {
                        c.value("red", 255) / 255.0f,
                        c.value("green", 255) / 255.0f,
                        c.value("blue", 255) / 255.0f
                    };

                    // Layout: [ Color Edit (Flex) ] [ Delete (Fixed) ]
                    float avail = ImGui::GetContentRegionAvail().x;
                    ImGui::SetNextItemWidth(avail - 80); // Leave room for the delete button

                    char buf[32]; sprintf(buf, "Color %d", i);
                    if (ImGui::ColorEdit3(buf, col, ImGuiColorEditFlags_NoInputs)) {
                        c["red"] = (int)(col[0] * 255.0f);
                        c["green"] = (int)(col[1] * 255.0f);
                        c["blue"] = (int)(col[2] * 255.0f);
                        dirty = true;
                    }

                    ImGui::SameLine();
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - 120);

                    if (ImGui::Button("Remove", ImVec2(100, 0))) colorToDelete = i;

                    ImGui::PopID();
                    ImGui::Spacing();
                }

                if (colorToDelete != -1) {
                    carcols["colors"].erase(carcols["colors"].begin() + colorToDelete);
                    dirty = true;
                }

                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            // --- TAB 2: VARIATIONS ---
            if (ImGui::BeginTabItem("Variations ")) {
                ImGui::Spacing();

                if (ImGui::Button("+ Add New", ImVec2(-1, 40))) {
                    carcols["variations"].push_back({ {"primary", 0}, {"secondary", 0}, {"tertiary", 0}, {"quaternary", 0} });
                    dirty = true;
                }

                ImGui::Separator();
                ImGui::BeginChild("VariationsScroll", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                ImGui::Spacing();

                int varToDelete = -1;
                for (int i = 0; i < (int)carcols["variations"].size(); ++i) {
                    ImGui::PushID(i + 5000);
                    char varLabel[64]; sprintf(varLabel, "Variation %d", i + 1);

                    // Framed and padded variation cards
                    if (ImGui::TreeNodeEx(varLabel, ImGuiTreeNodeFlags_Framed)) {
                        auto& v = carcols["variations"][i];

                        ImGui::Spacing();
                        dirty |= DrawIndexSelector(v, "Primary",   "primary",   carcols["colors"]);
                        dirty |= DrawIndexSelector(v, "Secondary", "secondary", carcols["colors"]);
                        dirty |= DrawIndexSelector(v, "Tertiary",  "tertiary",  carcols["colors"]);
                        dirty |= DrawIndexSelector(v, "Quaternary","quaternary",carcols["colors"]);
                        ImGui::Spacing();

                        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.5f));
                        if (ImGui::Button("Remove", ImVec2(-1, 40))) varToDelete = i;
                        ImGui::PopStyleColor();

                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                    ImGui::Spacing();
                }

                if (varToDelete != -1) {
                    carcols["variations"].erase(carcols["variations"].begin() + varToDelete);
                    dirty = true;
                }

                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        return dirty;
    }

private:
    bool DrawIndexSelector(json& var, const char* label, const std::string& key, const json& palette) {
        bool changed = false;
        int currentIndex = var.value(key, 0);

        // Auto-fix indices if palette shrunk
        if (currentIndex < 0) currentIndex = 0;
        if (currentIndex >= (int)palette.size() && !palette.empty()) currentIndex = 0;

        // Visual Color Preview
        ImVec4 previewCol = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
        if (!palette.empty() && currentIndex < (int)palette.size()) {
            auto& c = palette[currentIndex];
            previewCol = ImVec4(c.value("red", 0)/255.0f, c.value("green", 0)/255.0f, c.value("blue", 0)/255.0f, 1.0f);
        }

        // Preview Swatch
        ImGui::ColorButton("##swatch", previewCol, ImGuiColorEditFlags_NoTooltip, ImVec2(24, 24));
        ImGui::SameLine();

        // Responsive Dropdown
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
        char comboLabel[32];
        if (palette.empty()) sprintf(comboLabel, "No Colors Found");
        else sprintf(comboLabel, "Color %d", currentIndex);

        if (ImGui::BeginCombo(label, comboLabel)) {
            for (int n = 0; n < (int)palette.size(); n++) {
                const bool isSelected = (currentIndex == n);
                auto& c = palette[n];
                ImVec4 itemCol = ImVec4(c.value("red", 0)/255.0f, c.value("green", 0)/255.0f, c.value("blue", 0)/255.0f, 1.0f);

                ImGui::ColorButton("##p", itemCol, ImGuiColorEditFlags_NoTooltip, ImVec2(16, 16));
                ImGui::SameLine();

                if (ImGui::Selectable((std::string("Color ") + std::to_string(n)).c_str(), isSelected)) {
                    var[key] = n;
                    changed = true;
                }
            }
            ImGui::EndCombo();
        }
        return changed;
    }
};