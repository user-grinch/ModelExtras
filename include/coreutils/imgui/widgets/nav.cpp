#include "imgui/fonts/fontmgr.h"
#include "imgui/fonts/fonts.h"
#include "nav.h"

#include <algorithm>
#include <iomanip>
#include <map>

bool ImGuiNav::BeginPage(const char *icon, const char *name, bool active)
{
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext &g = *GImGui;
    ImGuiStyle &style = g.Style;
    ImGuiID id = window->GetID(icon);
    ImFont *iconFont = FontMgr::Get("icon");
    ImVec2 iconSz = iconFont->CalcTextSizeA(iconFont->LegacySize, FLT_MAX, 0.0f, icon);
    ImVec2 windowPos = window->Pos;
    ImVec2 pos = window->DC.CursorPos;

    ImRect rect(ImVec2(windowPos.x + style.ItemSpacing.x, pos.y),
                ImVec2(windowPos.x + style.ItemSpacing.x * 2.0f + iconSz.x, pos.y + iconSz.y + style.ItemSpacing.y));
    ImGui::ItemSize(ImVec4(rect.Min.x, rect.Min.y, rect.Max.x, rect.Max.y + 5), style.FramePadding.y);
    if (!ImGui::ItemAdd(rect, id))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(rect, id, &hovered, &held, NULL);
    ImVec4 textCol = style.Colors[active ? ImGuiCol_TitleBgActive : (hovered ? ImGuiCol_TextDisabled : ImGuiCol_Text)];
    textCol.w = 0.7f;
    ImVec4 iconCol = style.Colors[active ? ImGuiCol_TitleBgActive : hovered ? ImGuiCol_TextDisabled : ImGuiCol_Text];
    iconCol.w = 0.7f;

    window->DrawList->AddText(iconFont, iconFont->LegacySize, (rect.Min + rect.Max - iconSz) / 2, (ImColor)iconCol,
                              icon);

    if (hovered)
    {
        auto drawlist = ImGui::GetForegroundDrawList();

        ImFont *textFont = FontMgr::Get("title");
        float titleSz = textFont->LegacySize / 1.2f;
        ImVec2 sz = textFont->CalcTextSizeA(titleSz, FLT_MAX, 0.0f, name);

        ImVec2 min = {rect.Max.x + style.ItemSpacing.x * 4.0f,
                      rect.Min.y + style.ItemSpacing.y + (iconSz.y - sz.y) / 2};
        ImVec2 max = min + style.ItemSpacing + sz + ImVec2(0, 5);
        ImColor bgCol = style.Colors[ImGuiCol_FrameBg];
        drawlist->AddRectFilled(min, max + style.ItemSpacing, bgCol, style.WindowRounding, ImDrawFlags_RoundCornersAll);
        drawlist->AddText(textFont, titleSz, min, (ImColor)textCol, name);
    }
    return pressed;
}

std::string ImGuiNav::GetSelectedTab(ViewPtr page)
{
    for (auto &e : m_TabStore)
    {
        if (e.m_pPage == page)
        {
            return e.m_TabNames[e.m_nSelectedTab];
        }
    }
    return "";
}

bool ImGuiNav::IsTabSelected(ViewPtr page, std::string &&tabName)
{
    for (auto &e : m_TabStore)
    {
        if (e.m_pPage == page)
        {
            if (std::find(e.m_TabNames.begin(), e.m_TabNames.end(), tabName) == e.m_TabNames.end())
            {
                e.m_TabNames.push_back(tabName);
            }
            return e.m_TabNames[e.m_nSelectedTab] == tabName;
        }
    }

    // new entry
    m_TabStore.push_back({page, {tabName}, 0});
    return false;
}

bool ImGuiNav::BeginTab(const char *name, bool active)
{
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext &g = *GImGui;
    const ImGuiStyle &style = g.Style;
    const ImGuiID id = window->GetID(name);
    ImFont *pTitle = FontMgr::Get("title");
    float titleSz = pTitle->LegacySize / 1.2f;
    ImFont *pIcon = FontMgr::Get("icon");
    ImVec2 iconSz = pIcon->CalcTextSizeA(pIcon->LegacySize, FLT_MAX, 0.0f, ICON_HOME);
    ImVec2 textSz = pTitle->CalcTextSizeA(titleSz, FLT_MAX, 0.0f, "Custom Skins");
    ImVec2 pos = window->DC.CursorPos;

    const ImRect rect(pos, ImVec2(pos.x + textSz.x, pos.y + iconSz.y + style.ItemSpacing.y));
    ImGui::ItemSize(ImVec4(rect.Min.x, rect.Min.y, rect.Max.x, rect.Max.y + 3), style.FramePadding.y);
    if (!ImGui::ItemAdd(rect, id))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(rect, id, &hovered, &held, NULL);

    ImVec4 col = style.Colors[active ? ImGuiCol_TitleBgActive : hovered ? ImGuiCol_TextDisabled : ImGuiCol_Text];
    window->DrawList->AddText(pTitle, titleSz,
                              ImVec2(rect.Min.x + style.ItemSpacing.x + style.ItemInnerSpacing.x,
                                     (rect.Min.y + rect.Max.y) / 2 - textSz.y / 2),
                              ImColor(col.x, col.y, col.z, 0.7f), name);
    return pressed;
}

void ImGuiNav::ResetPage(ViewPtr page)
{
    auto e =
        std::find_if(m_TabStore.begin(), m_TabStore.end(), [page](ImGuiNav::TabInfo &f) { return f.m_pPage == page; });
    e->m_nSelectedTab = 0;
    e->m_TabNames.clear();
}

void ImGuiNav::ResetAll()
{
    m_TabStore.clear();
}
