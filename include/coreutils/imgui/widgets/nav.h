#pragma once
#include "imgui.h"
#include "imgui_internal.h"

#include <map>
#include <string>
#include <vector>

using ViewPtr = void *;

/*
    Navigation Class for Trainer
    Handles Pages, Tabs & Animations
*/
class ImGuiNav
{
  private:
    /*
        TabInfo structure
        Contains info about menu tabs
    */
    struct TabInfo
    {
        ViewPtr m_pPage = nullptr;
        std::vector<std::string> m_TabNames;
        size_t m_nSelectedTab = 0;
    };

    // Contains animation data for page & tabs
    struct AnimData
    {
        float containerAlpha, indicatorAlpha, textAlpha;
    };

    struct DrawAnimData
    {
        float alpha;
        float padding;
    };

  public:
    static inline std::vector<TabInfo> m_TabStore;

    // Inits a new page & draws it
    static bool BeginPage(const char *icon, const char *name, bool active);

    // Clears a page from store
    static void ResetPage(ViewPtr page);
    static void ResetAll();

    // Inits a new page-tab & draws it
    static bool BeginTab(const char *name, bool active);

    // Returns the name of the currently selected tab
    static std::string GetSelectedTab(ViewPtr page);

    // Returns true when specified tab is selected
    static bool IsTabSelected(ViewPtr page, std::string &&tabName);
};
