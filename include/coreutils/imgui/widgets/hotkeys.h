#pragma once
#include <imgui.h>

#include "db/dbmgr.hpp"

#include <string>

struct HotKeyCodes
{
    ImGuiKey key1, key2;

    std::string toString()
    {
        return std::format("{}.{}", static_cast<int>(key1), static_cast<int>(key2));
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(HotKeyCodes, key1, key2);

class Hotkey
{
  private:
    static inline std::string current;
    std::string hotkeyName;

    bool wPressed = false;
    HotKeyCodes usedCombo, defCombo;
    TableRegistry<HotKeyCodes> *config = nullptr;

    double lastUpdate;

  public:
    Hotkey(const std::string &name, TableRegistry<HotKeyCodes> *config, ImGuiKey key1 = ImGuiKey_None,
           ImGuiKey key2 = ImGuiKey_None);

    void Draw(const char *label);

    std::string GetKeyCombo();
    bool Pressed(bool noDelay = false);
};
