#include "extras.h"
#include "imgui_internal.h"

#include <map>

// A modified version of ImGui::SliderScalar
bool SliderScalar(const char *label, ImGuiDataType data_type, void *p_data, const void *p_min, const void *p_max,
                  const char *format, ImGuiSliderFlags flags)
{
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext &g = *GImGui;
    const ImGuiStyle &style = g.Style;
    const ImGuiID id = window->GetID(label);
    const float w = ImGui::CalcItemWidth();

    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, style.FramePadding.y * 5.0f));
    const ImRect total_bb(frame_bb.Min,
                          frame_bb.Max +
                              ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

    const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
    ImGui::ItemSize(total_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, id, &frame_bb, temp_input_allowed ? ImGuiItemFlags_Inputable : 0))
        return false;

    // Default format string when passing NULL
    if (format == NULL)
        format = ImGui::DataTypeGetInfo(data_type)->PrintFmt;

    const bool hovered = ImGui::ItemHoverable(frame_bb, id, ImGuiItemFlags_Default_);
    bool temp_input_is_active = temp_input_allowed && ImGui::TempInputIsActive(id);
    if (!temp_input_is_active)
    {
        const bool input_requested_by_tabbing =
            temp_input_allowed && ImGui::IsItemFocused() && (g.NavInputSource == ImGuiInputSource_Keyboard);

        const bool clicked = hovered && ImGui::IsMouseClicked(id, 0);
        const bool make_active = (input_requested_by_tabbing || clicked || g.NavActivateId == id);
        if (make_active && clicked)
            ImGui::SetKeyOwner(ImGuiKey_MouseLeft, id);
        if (make_active && temp_input_allowed)
            if (input_requested_by_tabbing || (clicked && g.IO.KeyCtrl) ||
                (g.NavActivateId == id && (g.NavActivateFlags & ImGuiActivateFlags_PreferInput)))
                temp_input_is_active = true;

        if (make_active && !temp_input_is_active)
        {
            ImGui::SetActiveID(id, window);
            ImGui::SetFocusID(id, window);
            ImGui::FocusWindow(window);
            g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
        }
    }

    if (temp_input_is_active)
    {
        const bool is_clamp_input = (flags & ImGuiSliderFlags_AlwaysClamp) != 0;
        return ImGui::TempInputScalar(frame_bb, id, label, data_type, p_data, format, is_clamp_input ? p_min : NULL,
                                      is_clamp_input ? p_max : NULL);
    }

    // Slider behavior
    ImRect grab_bb;
    const bool value_changed =
        ImGui::SliderBehavior(frame_bb, id, data_type, p_data, p_min, p_max, format, flags, &grab_bb);
    bool active = ImGui::IsItemActive();

    // Draw frame
    float factor_y = ImGui::GetIO().DisplaySize.y / 1920.0f;
    ImRect frame_sz = frame_bb;
    frame_sz.Min.y += 15.0f * factor_y;
    frame_sz.Max.y -= 15.0f * factor_y;

    ImRect temp = frame_sz;
    temp.Max.x = grab_bb.Max.x + 1;
    ImVec4 vCol = ImGui::GetStyle().Colors[ImGuiCol_FrameBgActive];
    vCol.w = 0.50f;

    // Render navigation highlight for gamepad
    ImGui::RenderNavHighlight(grab_bb, id);

    ImGui::RenderFrame(temp.Min, temp.Max, ImGui::ColorConvertFloat4ToU32(vCol), true, 30.0f);
    temp = frame_sz;
    temp.Min.x = grab_bb.Min.x - 1;
    ImGui::RenderFrame(temp.Min, temp.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, 30.0f);

    if (value_changed)
        ImGui::MarkItemEdited(id);

    // Render grab
    if (grab_bb.Max.x > grab_bb.Min.x)
    {
        float radius = (frame_sz.Max.y - frame_sz.Min.y) * 1.3f;
        ImVec2 pos = ImGui::GetWindowPos();
        ImVec2 center = ImVec2(grab_bb.Min.x + (grab_bb.Max.x - grab_bb.Min.x) / 2,
                               grab_bb.Max.y + (grab_bb.Min.y - grab_bb.Max.y) / 2);
        window->DrawList->AddCircleFilled(center, radius, ImGui::GetColorU32(ImGuiCol_FrameBgActive), 30.0f);
    }

    // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
    char value_buf[64];
    const char *value_buf_end =
        value_buf + ImGui::DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);

    if (active)
    {
        bool using_gamepad = g.NavInputSource == ImGuiInputSource_Gamepad;

        // Adjust tooltip position based on input method
        ImVec2 tooltipPos;
        if (using_gamepad)
        {
            tooltipPos = grab_bb.GetCenter();
            tooltipPos.y -= 20.0f; // Adjust this value for gamepad tooltip position
        }
        else
        {
            ImVec2 pos = ImGui::GetMousePos();
            if (pos.x > grab_bb.Min.x && pos.x < grab_bb.Max.x)
            {
                tooltipPos = grab_bb.GetCenter();
                tooltipPos.y -= 20.0f; // Adjust this value for mouse tooltip position
            }
        }

        ImVec2 sz = ImGui::CalcTextSize(value_buf, value_buf_end);
        sz.x += 2 * ImGui::GetStyle().WindowPadding.x;
        sz.y = ImGui::GetTextLineHeight() + 2 * ImGui::GetStyle().WindowPadding.y;

        tooltipPos.x -= sz.x / 2;
        tooltipPos.y -= sz.y / 2;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {2, 2});
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 20);
        vCol.w = 0.8f;
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::ColorConvertFloat4ToU32(vCol));
        ImGui::SetNextWindowPos(tooltipPos);
        ImGuiWindowFlags flags = ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar |
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;
        ImGui::Begin("Tooltip", NULL, flags);
        ImGui::Text(value_buf);
        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
    }

    if (label_size.x > 0.0f)
    {
        ImGui::RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y / 2),
                          label);
    }

    return value_changed;
}

bool SliderScalarN(const char *label, ImGuiDataType data_type, void *v, int components, const void *v_min,
                   const void *v_max, const char *format, ImGuiSliderFlags flags)
{
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext &g = *GImGui;
    bool value_changed = false;
    ImGui::BeginGroup();
    ImGui::PushID(label);
    ImGui::PushMultiItemsWidths(components, ImGui::CalcItemWidth());
    size_t type_size = ImGui::DataTypeGetInfo(data_type)->Size;
    for (int i = 0; i < components; i++)
    {
        ImGui::PushID(i);
        if (i > 0)
            ImGui::SameLine(0, g.Style.ItemInnerSpacing.x);
        value_changed |= SliderScalar("", data_type, v, v_min, v_max, format, flags);
        ImGui::PopID();
        ImGui::PopItemWidth();
        v = (void *)((char *)v + type_size);
    }
    ImGui::PopID();

    const char *label_end = ImGui::FindRenderedTextEnd(label);
    if (label != label_end)
    {
        ImGui::SameLine(0, g.Style.ItemInnerSpacing.x);
        ImGui::TextEx(label, label_end);
    }

    ImGui::EndGroup();
    return value_changed;
}

bool ImGuiExtras::Filter(const char *label, ImGuiTextFilter &filter, const char *hint)
{
    bool state = filter.Draw(label);
    if (strlen(filter.InputBuf) == 0)
    {
        ImDrawList *drawlist = ImGui::GetWindowDrawList();

        ImVec2 min = ImGui::GetItemRectMin();
        min.x += ImGui::GetStyle().FramePadding.x;
        min.y += ImGui::GetStyle().FramePadding.y;

        drawlist->AddText(min, ImGui::GetColorU32(ImGuiCol_TextDisabled), hint);
    }
    return state;
}

ImVec2 ImGuiExtras::CalcSize(short count, bool spacing)
{
    if (count == 1)
    {
        spacing = false;
    }

    float x = ImGuiExtras::GetWindowContentRegionWidth() - ImGui::GetStyle().ItemSpacing.x * (spacing ? count : 1);
    return ImVec2(x / count, ImGui::GetFrameHeight() * 1.15f);
}

ImVec2 ImGuiExtras::CalcSizeFrame(const char *text)
{
    return ImVec2(ImGui::CalcTextSize(text).x + 2 * ImGui::GetStyle().ItemSpacing.x, ImGui::GetFrameHeight());
}

void ImGuiExtras::TextCentered(const std::string &text)
{
    ImVec2 size = ImGui::CalcTextSize(text.c_str());
    ImGui::NewLine();
    float width = ImGuiExtras::GetWindowContentRegionWidth() - size.x;
    ImGui::SameLine(width / 2);
    ImGui::Text(text.c_str());
}

void ImGuiExtras::Tooltip(const char *text)
{
    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    ImGui::TextDisabled("?");

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text(text);
        ImGui::EndTooltip();
    }
}

bool ImGuiExtras::ListBox(const char *label, VecStr &allItems, std::string &selected)
{
    bool rtn = false;
    if (ImGui::BeginCombo(label, selected.c_str()))
    {
        for (std::string curItem : allItems)
        {
            if (ImGui::MenuItem(curItem.c_str()))
            {
                selected = curItem;
                rtn = true;
            }
        }
        ImGui::EndCombo();
    }

    return rtn;
}

bool ImGuiExtras::ListBox(const char *label, VecStr &allItems, int &selected)
{
    bool rtn = false;
    if (ImGui::BeginCombo(label, allItems[selected].c_str()))
    {
        for (size_t index = 0; index < allItems.size(); ++index)
        {
            if (ImGui::MenuItem(allItems[index].c_str()))
            {
                selected = index;
                rtn = true;
            }
        }
        ImGui::EndCombo();
    }

    return rtn;
}

bool ImGuiExtras::Checkbox(const char *label, bool *v, const char *hint, bool is_disabled)
{
    ImGuiWindow *window = ImGui::GetCurrentWindow();

    if (window->SkipItems)
        return false;

    ImGuiID id = ImGui::GetID(label);
    ImGuiContext &g = *GImGui;
    ImGuiStyle &style = g.Style;
    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

    const float square_sz = g.FontSize + 4.0f;
    const ImVec2 pos = window->DC.CursorPos;

    const ImRect total_bb(
        pos, pos + ImVec2(square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f),
                          label_size.y + style.FramePadding.y * 2.0f));
    ImGui::ItemSize(total_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, id))
    {
        return false;
    }

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
    if (pressed)
    {
        *v = !(*v);
        ImGui::MarkItemEdited(id);
    }

    const ImRect check_bb(pos, pos + ImVec2(square_sz, square_sz));
    ImGui::RenderNavHighlight(total_bb, id);
    ImGui::RenderFrame(check_bb.Min, check_bb.Max,
                       ImGui::GetColorU32((held && hovered) ? ImGuiCol_FrameBg
                                          : hovered         ? ImGuiCol_FrameBgHovered
                                                            : ImGuiCol_FrameBg),
                       true, 30.0f);

    const float pad = ImMax(1.0f, IM_FLOOR(square_sz / 6.0f));
    if (*v)
        ImGui::RenderCheckMark(window->DrawList, check_bb.Min + ImVec2(pad, pad),
                               ImGui::GetColorU32(ImGui::GetStyleColorVec4(ImGuiCol_CheckMark)),
                               square_sz - pad * 2.0f);

    ImVec2 label_pos = ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y);
    if (label_size.x > 0.0f)
    {
        ImGui::RenderText(label_pos, label);
    }

    // draw hint
    if (hint != nullptr && hint != "")
    {
        ImGui::SameLine(0, style.ItemInnerSpacing.x);
        ImGui::InvisibleButton("?", ImGui::CalcTextSize("?", nullptr, true));
        ImVec2 min = ImGui::GetItemRectMin();
        window->DrawList->AddText(ImVec2(min.x, min.y + style.ItemInnerSpacing.y / 2),
                                  ImGui::GetColorU32(ImGuiCol_TextDisabled), "?");

        if (ImGui::IsItemHovered() && !is_disabled)
        {
            ImGui::BeginTooltip();
            ImGui::Text(hint);
            ImGui::Spacing();
            ImGui::EndTooltip();
        }
    }
    return pressed;
}

bool ImGuiExtras::IsLeftClick()
{
    return ImGui::IsMouseReleased(ImGuiMouseButton_Left) || ImGui::IsKeyPressed(ImGuiKey_GamepadFaceDown);
}

bool ImGuiExtras::IsRightClick()
{
    return ImGui::IsMouseReleased(ImGuiMouseButton_Right) || ImGui::IsKeyPressed(ImGuiKey_GamepadFaceLeft);
}

bool ImGuiExtras::BeginPopupContextWindow(const char *str_id, ImGuiPopupFlags popup_flags)
{
    ImGuiContext &g = *GImGui;
    ImGuiWindow *window = g.CurrentWindow;
    if (!str_id)
        str_id = "window_context";
    ImGuiID id = window->GetID(str_id);
    int mouse_button = (popup_flags & ImGuiPopupFlags_MouseButtonMask_);
    if (IsRightClick() && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
        if (!(popup_flags & ImGuiPopupFlags_NoOpenOverItems) || !ImGui::IsAnyItemHovered())
            ImGui::OpenPopupEx(id, popup_flags);
    return ImGui::BeginPopupEx(id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar |
                                       ImGuiWindowFlags_NoSavedSettings);
}

bool ImGuiExtras::ColorButton(const char *desc_id, const ImVec4 &col, bool def_rounding, ImGuiColorEditFlags flags,
                              const ImVec2 &size_arg)
{
    std::string label = desc_id;
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
    {
        return false;
    }

    ImGuiContext &g = *GImGui;
    const ImGuiID id = window->GetID(desc_id);
    const float default_size = ImGui::GetFrameHeight();
    const ImVec2 size(size_arg.x == 0.0f ? default_size : size_arg.x, size_arg.y == 0.0f ? default_size : size_arg.y);
    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
    ImGui::ItemSize(bb, (size.y >= default_size) ? g.Style.FramePadding.y : 0.0f);
    if (!ImGui::ItemAdd(bb, id))
    {
        return false;
    }

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    window->DrawList->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32({col.x, col.y, col.z, col.w}),
                                    def_rounding ? g.Style.FrameRounding : 30.0f);

    ImGui::RenderNavHighlight(bb, id);

    return pressed;
}

bool ImGuiExtras::ColorPicker(const char *label, float col[4], ImGuiColorEditFlags flags)
{
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
    {
        return false;
    }

    ImGuiContext &g = *GImGui;
    const ImGuiStyle &style = g.Style;
    const char *label_display_end = ImGui::FindRenderedTextEnd(label);
    g.NextItemData.ClearFlags();

    ImGui::BeginGroup();
    ImGui::PushID(label);
    const bool set_current_color_edit_id = (g.ColorEditCurrentID == 0);
    if (set_current_color_edit_id)
        g.ColorEditCurrentID = window->IDStack.back();

    ImGuiWindow *picker_active_window = NULL;
    float alpha = style.Colors[ImGuiCol_FrameBg].w;
    const ImVec4 col_v4(col[0], col[1], col[2], alpha > 0.7f ? 0.7f : alpha);
    auto sz = ImGui::GetFrameHeight() - 4.0f;
    if (ColorButton("##ColorButton", col_v4, false, NULL, {sz, sz}))
    {
        g.ColorPickerRef = col_v4;
        ImGui::OpenPopup("picker");
        ImGui::SetNextWindowPos(g.LastItemData.Rect.GetBL() + ImVec2(0.0f, style.ItemSpacing.y));
    }

    ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
    ImGui::BeginGroup();
    ImGui::Dummy({0.0f, 1.0f});
    ImGui::TextEx(label, ImGui::FindRenderedTextEnd(label));
    ImGui::EndGroup();

    bool pressed = false;
    if (ImGui::BeginPopup("picker"))
    {
        if (g.CurrentWindow->BeginCount == 1)
        {
            picker_active_window = g.CurrentWindow;
            ImGuiColorEditFlags picker_flags = flags | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_Uint8 |
                                               ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoLabel |
                                               ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar |
                                               ImGuiColorEditFlags_NoSmallPreview;

            ImGui::SetNextItemWidth(ImGui::GetFrameHeight() * 12.0f); // Use 256 + bar sizes?
            pressed = ImGui::ColorPicker4("##picker", col, picker_flags, &g.ColorPickerRef.x);
        }
        ImGui::EndPopup();
    }

    if (set_current_color_edit_id)
    {
        g.ColorEditCurrentID = 0;
    }

    ImGui::PopID();
    ImGui::EndGroup();

    // When picker is being actively used, use its active id so IsItemActive() will function on ColorEdit4().
    if (picker_active_window && g.ActiveId != 0 && g.ActiveIdWindow == picker_active_window)
    {
        g.LastItemData.ID = g.ActiveId;
    }

    if (pressed && g.LastItemData.ID != 0) // In case of ID collision, the second EndGroup() won't catch g.ActiveId
        ImGui::MarkItemEdited(g.LastItemData.ID);

    return pressed;
}

void ImGuiExtras::TooltipInlined(const char *expandedText, const char *minimizedText)
{
    static float animProgress = 0.0f;
    static float animSpeed = 2.0f;

    ImGuiStyle &style = ImGui::GetStyle();
    float deltaTime = ImGui::GetIO().DeltaTime;

    ImVec2 expandedSize = ImGui::CalcTextSize(expandedText);
    ImVec2 iconSize = ImGui::CalcTextSize(minimizedText);
    iconSize.x += style.ItemInnerSpacing.x * 8.0f;
    expandedSize.x += style.ItemInnerSpacing.x * 4.0f;

    float currentWidth = iconSize.x + animProgress * (expandedSize.x - iconSize.x);
    float currentHeight = iconSize.y + style.ItemInnerSpacing.y * 4.0f;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, currentHeight * 0.5f);
    ImVec4 col = style.Colors[ImGuiCol_Button];
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, col);
    ImGui::Button(animProgress > 0.0f ? expandedText : minimizedText, ImVec2(currentWidth, currentHeight));
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();

    if (ImGui::IsItemHovered())
    {
        animProgress = std::min(1.0f, animProgress + animSpeed * deltaTime);
    }
    else
    {
        animProgress = std::max(0.0f, animProgress - animSpeed * deltaTime);
    }
}

float ImGuiExtras::GetWindowContentRegionWidth()
{
    return ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
}
