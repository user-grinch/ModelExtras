#pragma once
#include "pch.h"
#include "defines.h"
#include "modules/light.hpp"
#include "modules/metadata.hpp"
#include "modules/sirens.hpp"
#include "modules/spoiler.hpp"
#include "view/common.hpp"

#pragma once
#include <imgui.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <memory>

#include "modules/door.hpp"
#include "modules/exhaust.hpp"
#include "modules/IEditor.hpp"
#include "modules/roof.hpp"
#include "modules/carcols.hpp"

class ModelExtrasEditor {
private:
    int currentModelId = -1;
    std::string basePath = "./ModelExtras/data/";
    json rootData;
    bool isDirty = false;
    char statusMessage[512] = "Ready";
    ImVec4 statusColor = ImVec4(1, 1, 1, 0.5f);

    std::vector<std::unique_ptr<IEditorModule>> modules;

    void SetStatus(const char* msg, ImVec4 color = ImVec4(1, 1, 1, 0.5f)) {
        strncpy(statusMessage, msg, sizeof(statusMessage));
        statusColor = color;
    }

public:
    ModelExtrasEditor() {
        // Core Modules
        modules.push_back(std::make_unique<MetadataModule>());
        modules.push_back(std::make_unique<ExhaustsModule>());
        modules.push_back(std::make_unique<SpoilerModule>());
        modules.push_back(std::make_unique<RoofModule>());

        modules.push_back(std::make_unique<DoorsModule>());
        modules.push_back(std::make_unique<CarcolsModule>());

        modules.push_back(std::make_unique<LightsModule>());
        modules.push_back(std::make_unique<SirensModule>());

    }

    void SetModel(int id) {
        if (currentModelId != id) {
            // Optional: Prompt to save if isDirty
            currentModelId = id;
            Load();
        }
    }

    void Load() {
        if (!std::filesystem::exists(basePath))
            std::filesystem::create_directories(basePath);

        std::string fullPath = basePath + std::to_string(currentModelId) + ".jsonc";
        std::ifstream file(fullPath);

        if (file.is_open()) {
            try {
                rootData = json::parse(file, nullptr, true, true);
                SetStatus("Configuration loaded successfully.");
            } catch (const std::exception& e) {
                SetStatus("Parse Error! Check Raw View.", ImVec4(1, 0.4f, 0.4f, 1));
                rootData = json::object();
            }
        } else {
            SetStatus("Template created for new ID.");
            rootData = json::object();
            rootData["metadata"] = {{"author", "Default"}, {"version", "1.0"}};
        }
        isDirty = false;
    }

    void Save() {
        if (currentModelId < 0) return;
        std::string fullPath = basePath + std::to_string(currentModelId) + ".jsonc";
        std::ofstream file(fullPath);
        if (file.is_open()) {
            file << rootData.dump(4);
            SetStatus("File saved to disk.", ImVec4(0.4f, 1.0f, 0.4f, 1));
            isDirty = false;
        } else {
            SetStatus("Write Error: Check folder permissions!", ImVec4(1, 0, 0, 1));
        }
    }

    void DrawUI() {
        // 1. Top Toolbar
        DrawToolbar();

        ImGui::Separator();

        // 2. Main Content Area (Tabs)
        if (ImGui::BeginTabBar("ModulesBar", ImGuiTabBarFlags_None)) {
            for (auto& mod : modules) {
                // If a module has changes, we could add an asterisk to the tab name
                std::string tabName = mod->GetName();
                if (ImGui::BeginTabItem(tabName.c_str())) {

                    ImGui::BeginChild("ModuleContent", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));
                    if (mod->Render(rootData)) {
                        isDirty = true;
                    }
                    ImGui::EndChild();

                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }

        // 3. Bottom Status Bar
        DrawStatusBar();
    }

private:
    void DrawToolbar() {
        ImGui::BeginGroup();
        ImGui::TextDisabled("VEHICLE ID:"); ImGui::SameLine();
        ImGui::Text("%d", currentModelId);
        ImGui::EndGroup();

        ImGui::SameLine(ImGui::GetWindowWidth() - 180);

        if (isDirty) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
            if (ImGui::Button("SAVE CHANGES")) Save();
            ImGui::PopStyleColor();
        } else {
            ImGui::BeginDisabled();
            ImGui::Button("SAVE CHANGES");
            ImGui::EndDisabled();
        }

        ImGui::SameLine();
        if (ImGui::Button("RELOAD")) Load();
    }

    void DrawStatusBar() {
        ImGui::Separator();

        // Show dirty flag
        if (isDirty) {
            ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), " UNSAVED CHANGES ");
        } else {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), " SYNCED ");
        }

        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        // Show status message with its own color
        ImGui::TextColored(statusColor, "%s", statusMessage);
    }
};

void Tab_Editor() {
    if (ImGui::BeginTabItem("Editor")) {
        CVehicle* pVeh = FindPlayerVehicle();
        if (pVeh) {
            static ModelExtrasEditor editor;
            static int lastModelIndex = -1;
            int curModelIndex = pVeh->m_nModelIndex;
            if (curModelIndex != lastModelIndex) {
                editor.SetModel(curModelIndex);
                editor.Load();
                lastModelIndex = curModelIndex;
            }
            editor.DrawUI();
        } else {
            DrawVehicleRequiredMessage();
        }
        ImGui::EndTabItem();
    }
}