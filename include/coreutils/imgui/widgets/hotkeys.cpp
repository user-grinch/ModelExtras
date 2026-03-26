#include "hotkeys.h"
#include "imgui_internal.h"

inline bool IsKeyDown(ImGuiKey key)
{
    return key == ImGuiKey_None || ImGui::IsKeyDown(key);
}

Hotkey::Hotkey(const std::string &name, TableRegistry<HotKeyCodes> *config, ImGuiKey key1, ImGuiKey key2)
{
    // default values
    usedCombo.key1 = defCombo.key1 = key1;
    usedCombo.key2 = defCombo.key2 = key2;

    if (!config || !config->load())
    {
        return;
    }

    if (!config->getTable(name).empty())
    {
        usedCombo = config->getTable(name).front(); // Should be one entry anyway
    }

    if (key2 == ImGuiKey_None)
    {
        usedCombo.key2 = usedCombo.key1;
    }

    this->config = config;
    hotkeyName = name;
}

void Hotkey::Draw(const char *label)
{
    bool active = (current == label);
    bool pressed = false;
    static bool wasPressed = false;
    ImGuiStyle &style = ImGui::GetStyle();

    // Check for pressed keys
    if (active)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_FrameBgActive]);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, style.Colors[ImGuiCol_FrameBgActive]);

        for (ImGuiKey key = ImGuiKey_Tab; key <= ImGuiKey_GamepadRStickDown;
             key = static_cast<ImGuiKey>(static_cast<int>(key) + 1))
        {
            if (IsKeyDown(key))
            {
                usedCombo.key1 = key;
                pressed = true;
                break;
            }
        }

        if (usedCombo.key1 != ImGuiKey_None)
        {
            for (ImGuiKey key = static_cast<ImGuiKey>(static_cast<int>(usedCombo.key1) + 1);
                 key <= ImGuiKey_GamepadRStickDown; key = static_cast<ImGuiKey>(static_cast<int>(key) + 1))
            {
                if (IsKeyDown(key))
                {
                    usedCombo.key2 = key;
                    pressed = true;
                    break;
                }
            }
        }

        if (usedCombo.key2 == ImGuiKey_None)
        {
            usedCombo.key2 = usedCombo.key1;
        }
    }

    ImGui::Text(label);
    ImVec2 resetButtonSize = ImGui::CalcTextSize("ResetReset");
    float button1Width = ImGui::GetContentRegionAvail().x - resetButtonSize.x - ImGui::GetStyle().ItemSpacing.x * 3.0f;
    float height = ImGui::GetFrameHeight() * 1.25f;
    if (ImGui::Button(std::format("{}##{}", GetKeyCombo(), label).c_str(), ImVec2(button1Width, height)) && !active)
    {
        current = label;
    }

    // Show a tooltip for active state above the hotkey widget
    if (active)
    {
        if (wasPressed && !IsKeyDown(usedCombo.key1) && !IsKeyDown(usedCombo.key2))
        {
            current = "";
            wasPressed = pressed = false;

            if (config)
            {
                config->clearTable(hotkeyName);
                config->updateByID(hotkeyName, usedCombo);
                config->save();
            }

            lastUpdate = ImGui::GetTime();
        }

        ImVec2 windowPos = ImGui::GetCurrentWindow()->Pos;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        pos.y = pos.y > windowPos.y ? pos.y : windowPos.y;
        pos.y -= ImGui::GetTextLineHeightWithSpacing() + height + style.WindowPadding.y * 2;

        ImGui::SetNextWindowPos(pos);
        if (!pressed && current == label)
        {
            usedCombo = {ImGuiKey_None, ImGuiKey_None};
        }
        ImGui::SetTooltip(pressed ? "Release the keys to set as hotkey" : "Press a new key combination");
    }

    if (active)
    {
        ImGui::PopStyleColor(2);
        wasPressed = pressed;
    }
    ImGui::SameLine();
    if (ImGui::Button(std::format("Reset##{}", label).c_str(), ImVec2(resetButtonSize.x, height)))
    {
        current = "";
        usedCombo = defCombo;
        config->clearTable(hotkeyName);
        config->updateByID(hotkeyName, usedCombo);
        config->save();
    }
    ImGui::Dummy(ImVec2(0, 10));
}

bool Hotkey::Pressed(bool noDelay)
{
    if (ImGui::GetTime() - lastUpdate < 2.0)
        return false;

    if (noDelay)
    {
        return IsKeyDown(usedCombo.key1) && IsKeyDown(usedCombo.key2);
    }
    else
    {
        if (IsKeyDown(usedCombo.key1) && IsKeyDown(usedCombo.key2))
        {
            wPressed = true;
        }
        else
        {
            if (wPressed)
            {
                wPressed = false;
                return current == "";
            }
        }
    }
    return false;
}

std::string Hotkey::GetKeyCombo()
{
    if (usedCombo.key1 == usedCombo.key2)
    {
        return ImGui::GetKeyName(usedCombo.key1);
    }
    else
    {
        return std::format("{} + {}", ImGui::GetKeyName(usedCombo.key1), ImGui::GetKeyName(usedCombo.key2));
    }
}
