#pragma once
#include "imgui.h"

#include <string>
#include <vector>

class ImGuiExtras
{
  private:
    using VecStr = const std::vector<std::string>;

  public:
    // Custom context menu with gamepad support
    static bool BeginPopupContextWindow(const char *str_id = NULL, ImGuiPopupFlags popup_flags = 1);

    // Calculates button size based on window width & spacing flags
    static ImVec2 CalcSize(short count = 1, bool spacing = true);

    // Calculates button size based on frame size
    static ImVec2 CalcSizeFrame(const char *text);

    // Regular checkbox with hint support
    static bool Checkbox(const char *label, bool *v, const char *hint = "", bool is_disabled = false);

    static bool ColorButton(const char *desc_id, const ImVec4 &col, bool def_rounding = true,
                            ImGuiColorEditFlags flags = NULL, const ImVec2 &size_arg = {0, 0});
    static bool ColorPicker(const char *label, float col[4], ImGuiColorEditFlags flags = NULL);

    // ImGui::TextFilter with hint support
    static bool Filter(const char *label, ImGuiTextFilter &filter, const char *hint);

    // IsLeft/Right click with gamepad support
    static bool IsLeftClick();
    static bool IsRightClick();

    // Draws a dropdown listbox
    static bool ListBox(const char *label, VecStr &allItems, int &selected);
    static bool ListBox(const char *label, VecStr &allItems, std::string &selected);

    // Text aligned to the center of the window
    static void TextCentered(const std::string &text);

    // Displays a popup with helpful text
    static void Tooltip(const char *text);
    // Displays sliding inlined tooltip
    static void TooltipInlined(const char *expandedText, const char *minimizedText = "Info");

    static float GetWindowContentRegionWidth();
};
