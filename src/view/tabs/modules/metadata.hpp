#pragma once
#include "IEditor.hpp"
#include <vector>

class MetadataModule : public IEditorModule {
    // --- Configuration: Edit these easily later ---
    struct VersionEntry {
        const char* label;
        int value;
    };

    static constexpr VersionEntry kVersionList[] = {
        { "v1.7", 10700 },
        { "v2.0", 20000 },
        { "v2.1", 20100 },
    };

public:
    const char* GetName() const override { return "Metadata"; }

    bool Render(json& root) override {
        bool dirty = false;

        // Initialize metadata if missing
        if (!root.contains("metadata")) root["metadata"] = json::object();
        json& meta = root["metadata"];

        ImGui::Spacing();
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);

        // 1. Text Fields
        dirty |= DrawTextField(meta, "Author", "author");
        dirty |= DrawTextField(meta, "Description", "desc");
        dirty |= DrawTextField(meta, "Creation Date", "creationtime");

        // 2. Version Dropdown
        dirty |= DrawVersionCombo(meta);

        ImGui::PopItemWidth();
        return dirty;
    }

private:
    // Helper to keep the Render function clean
    bool DrawTextField(json& meta, const char* label, const std::string& key) {
        std::string value = UI::GetString(meta, key.c_str());
        if (ImGui::InputText(label, &value)) {
            meta[key] = value;
            return true;
        }
        return false;
    }

    bool DrawVersionCombo(json& meta) {
        bool dirty = false;
        int currentVer = meta.value("minver", 10700);

        // Find current label
        const char* preview = "Unknown Version";
        for (const auto& entry : kVersionList) {
            if (entry.value == currentVer) {
                preview = entry.label;
                break;
            }
        }

        if (ImGui::BeginCombo("Min ME version", preview)) {
            for (const auto& entry : kVersionList) {
                bool isSelected = (currentVer == entry.value);
                if (ImGui::Selectable(entry.label, isSelected)) {
                    meta["minver"] = entry.value;
                    dirty = true;
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        return dirty;
    }
};