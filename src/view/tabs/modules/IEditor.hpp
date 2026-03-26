#pragma once
#include "imgui.h"
#include "imgui_stdlib.h"
#include "nlohmann\json.hpp"

using json = nlohmann::json;

namespace UI
{
    // Safe string getter
    static std::string GetString(const json &j, const char *key, const char *defaultVal = "")
    {
        return j.value(key, defaultVal);
    }

    // Helper for RGBA JSON objects
    static bool DrawJsonColor(const char *label, json &jNode)
    {
        float col[4] = {
            jNode.value("red", 0) / 255.0f,
            jNode.value("green", 0) / 255.0f,
            jNode.value("blue", 0) / 255.0f,
            jNode.value("alpha", 255) / 255.0f};

        if (ImGui::ColorEdit4(label, col, ImGuiColorEditFlags_AlphaPreviewHalf))
        {
            jNode["red"] = static_cast<int>(col[0] * 255.0f);
            jNode["green"] = static_cast<int>(col[1] * 255.0f);
            jNode["blue"] = static_cast<int>(col[2] * 255.0f);
            jNode["alpha"] = static_cast<int>(col[3] * 255.0f);
            return true;
        }
        return false;
    }

    // Helper for Enums/Combos
    static bool DrawJsonCombo(const char *label, json &jNode, const char *key, const std::vector<std::string> &options)
    {
        std::string current = jNode.value(key, options.empty() ? "" : options[0]);
        bool changed = false;

        if (ImGui::BeginCombo(label, current.c_str()))
        {
            for (const auto &opt : options)
            {
                bool isSelected = (current == opt);
                if (ImGui::Selectable(opt.c_str(), isSelected))
                {
                    jNode[key] = opt;
                    changed = true;
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        return changed;
    }
}

class IEditorModule
{
public:
    virtual ~IEditorModule() = default;
    virtual const char *GetName() const = 0;
    virtual bool Render(nlohmann::json &root) = 0;
};
