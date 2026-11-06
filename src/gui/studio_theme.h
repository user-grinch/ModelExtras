#pragma once
#include <imgui.h>
#include <windows.h>
#include <filesystem>
#include <algorithm>
#include <cmath>
#include <cstdio>

// Presentation only: shared with the DX9 preview harness, independent of game state.
namespace StudioTheme {
struct Preferences {
    ImVec4 accent{0.43f, 0.52f, 0.72f, 1};
    float scale = 1, fontSize = 18;
    float opacity = 1;
    bool hasWindowRect = false;
    ImVec2 windowPos{0, 0};
    ImVec2 windowSize{0, 0};
};
inline ImVec2 lastShellPos;
inline ImVec2 lastShellSize;
inline bool resetWindowRequested = false;
inline void Apply(const Preferences &prefs) {
    auto &style = ImGui::GetStyle();
    style = ImGuiStyle{}; // Rebuild from base sizes so scaling never accumulates.
    ImGui::StyleColorsDark();
    style.WindowPadding = ImVec2(20, 18);
    style.FramePadding = ImVec2(12, 5.0f);
    style.ItemSpacing = ImVec2(12, 7.0f);
    style.ItemInnerSpacing = ImVec2(10, 6);
    style.CellPadding = ImVec2(12, 5);
    style.WindowRounding = 12;
    style.ChildRounding = 8;
    style.FrameRounding = 6;
    style.PopupRounding = 8;
    style.ScrollbarRounding = 9;
    style.GrabRounding = 5;
    style.ScrollbarSize = 10;
    style.GrabMinSize = 14;
    style.WindowBorderSize = style.ChildBorderSize = 1;
    style.FrameBorderSize = 0;
    style.WindowTitleAlign = ImVec2(0, 0.5f);
    auto *c = style.Colors;
    c[ImGuiCol_Text] = ImVec4(.89f, .92f, .95f, 1);
    c[ImGuiCol_TextDisabled] = ImVec4(.49f, .56f, .63f, 1);
    c[ImGuiCol_WindowBg] = ImVec4(.085f, .094f, .105f, 1);
    c[ImGuiCol_ChildBg] = ImVec4(.105f, .116f, .129f, 1);
    c[ImGuiCol_PopupBg] = ImVec4(.085f, .105f, .125f, 1);
    c[ImGuiCol_Border] = ImVec4(.16f, .20f, .24f, 1);
    c[ImGuiCol_FrameBg] = ImVec4(.15f, .166f, .184f, 1);
    c[ImGuiCol_FrameBgHovered] = ImVec4(.16f, .20f, .24f, 1);
    c[ImGuiCol_FrameBgActive] = ImVec4(.18f, .23f, .27f, 1);
    c[ImGuiCol_Button] = ImVec4(.12f, .16f, .19f, 1);
    c[ImGuiCol_ButtonHovered] = ImVec4(.19f, .25f, .29f, 1);
    c[ImGuiCol_ButtonActive] = ImVec4(.23f, .30f, .34f, 1);
    c[ImGuiCol_Header] = ImVec4(.12f, .17f, .20f, 1);
    c[ImGuiCol_HeaderHovered] = ImVec4(.17f, .24f, .27f, 1);
    c[ImGuiCol_HeaderActive] = ImVec4(.20f, .29f, .31f, 1);
    c[ImGuiCol_Separator] = c[ImGuiCol_Border];
    c[ImGuiCol_ScrollbarBg] = c[ImGuiCol_WindowBg];
    c[ImGuiCol_ScrollbarGrab] = ImVec4(.23f, .29f, .34f, 1);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(.32f, .40f, .46f, 1);
    c[ImGuiCol_CheckMark] = c[ImGuiCol_SliderGrab] = c[ImGuiCol_SliderGrabActive] = prefs.accent;
    c[ImGuiCol_SeparatorHovered] = c[ImGuiCol_SeparatorActive] = prefs.accent;
    c[ImGuiCol_TextSelectedBg] = ImVec4(prefs.accent.x, prefs.accent.y, prefs.accent.z, .30f);
    c[ImGuiCol_ResizeGrip] = ImVec4(.22f, .30f, .34f, .4f);
    c[ImGuiCol_ResizeGripHovered] = c[ImGuiCol_ResizeGripActive] = prefs.accent;
    style.ScaleAllSizes(prefs.scale);
    // ScaleAllSizes truncates sizes with ImTrunc. Values with base 1.0f truncate to 0.0f
    // when scale < 1.0f, causing the software cursor and essential 1px borders to vanish.
    style.WindowBorderSize = style.ChildBorderSize = 1;
    style.PopupBorderSize = 1;
    style.SeparatorSize = 1;
    style.InputTextCursorSize = 1;
    style.MouseCursorScale = 1;
    style.FontSizeBase = prefs.fontSize;
    style.FontScaleMain = prefs.scale;
    style.Alpha = 1;
    c[ImGuiCol_WindowBg].w = c[ImGuiCol_ChildBg].w = prefs.opacity;
}
inline void LoadFont() {
    wchar_t directory[MAX_PATH]{};
    if (GetWindowsDirectoryW(directory, MAX_PATH)) {
        const auto path = std::filesystem::path(directory) / "Fonts" / "segoeui.ttf";
        if (std::filesystem::exists(path)) ImGui::GetIO().Fonts->AddFontFromFileTTF(path.string().c_str(), 18);
    }
    if (ImGui::GetIO().Fonts->Fonts.empty()) ImGui::GetIO().Fonts->AddFontDefaultVector();
}
inline void Heading(const char *title, const char *subtitle) {
    ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 1.35f);
    ImGui::TextUnformatted(title);
    ImGui::PopFont();
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGui::TextWrapped("%s", subtitle);
    ImGui::PopStyleColor();
    ImGui::Spacing();
}
inline bool PrimaryButton(const char *label) {
    const auto accent = ImGui::GetStyle().Colors[ImGuiCol_CheckMark];
    ImGui::PushStyleColor(ImGuiCol_Button, accent);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(std::min(1.f, accent.x + .1f), std::min(1.f, accent.y + .1f), std::min(1.f, accent.z + .1f), 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(accent.x * .8f, accent.y * .8f, accent.z * .8f, 1));
    const float brightness = .2126f * accent.x + .7152f * accent.y + .0722f * accent.z;
    ImGui::PushStyleColor(ImGuiCol_Text, brightness > .5f ? ImVec4(.03f, .06f, .07f, 1) : ImVec4(1, 1, 1, 1));
    const bool pressed = ImGui::Button(label);
    ImGui::PopStyleColor(4);
    return pressed;
}
inline void Hint(const char *label) {
    if (!label || !*label || !ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) return;
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 27);
    ImGui::TextUnformatted(label);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}
inline void DrawNavigationIcon(ImDrawList *draw, int icon, ImVec2 center, float unit, ImU32 color) {
    if (icon == 0) {
        // Settings: Mechanical gear
        draw->AddCircle(center, 2.4f * unit, color, 0, 1.5f * unit);
        draw->AddCircle(center, 4.8f * unit, color, 0, 1.5f * unit);
        static const ImVec2 gearDirs[6] = {
            { 1.0f, 0.0f },
            { 0.5f, 0.866025f },
            {-0.5f, 0.866025f },
            {-1.0f, 0.0f },
            {-0.5f, -0.866025f },
            { 0.5f, -0.866025f }
        };
        for (const auto &d : gearDirs) {
            draw->AddLine(ImVec2(center.x + 4.6f * unit * d.x, center.y + 4.6f * unit * d.y),
                          ImVec2(center.x + 7.3f * unit * d.x, center.y + 7.3f * unit * d.y),
                          color, 2.6f * unit);
        }
    } else if (icon == 1) {
        // Workshop: Combination wrench
        const ImVec2 h1(center.x - 5.6f * unit, center.y + 5.6f * unit);
        const ImVec2 h2(center.x + 1.2f * unit, center.y - 1.2f * unit);
        draw->AddLine(h1, h2, color, 2.6f * unit);
        draw->AddCircle(h1, 2.2f * unit, color, 0, 1.5f * unit);
        const float fx = 0.7071f, fy = -0.7071f;
        const float px = 0.7071f, py = 0.7071f;
        const ImVec2 j1(h2.x - px * 2.8f * unit, h2.y - py * 2.8f * unit);
        const ImVec2 j2(h2.x + px * 2.8f * unit, h2.y + py * 2.8f * unit);
        draw->AddLine(j1, j2, color, 1.8f * unit);
        const ImVec2 p1(j1.x + fx * 4.2f * unit, j1.y + fy * 4.2f * unit);
        const ImVec2 p2(j2.x + fx * 4.2f * unit, j2.y + fy * 4.2f * unit);
        draw->AddLine(j1, p1, color, 1.8f * unit);
        draw->AddLine(j2, p2, color, 1.8f * unit);
    } else if (icon == 2) {
        // Appearance: Paint palette
        draw->AddCircle(center, 7.2f * unit, color, 0, 1.5f * unit);
        draw->AddCircleFilled(ImVec2(center.x - 2.8f * unit, center.y - 2.4f * unit), 1.1f * unit, color);
        draw->AddCircleFilled(ImVec2(center.x + 0.8f * unit, center.y - 3.2f * unit), 1.1f * unit, color);
        draw->AddCircleFilled(ImVec2(center.x - 3.2f * unit, center.y + 1.6f * unit), 1.1f * unit, color);
        draw->AddCircle(ImVec2(center.x + 2.2f * unit, center.y + 1.8f * unit), 1.6f * unit, color, 0, 1.4f * unit);
    } else if (icon == 3) {
        // About: Info badge
        draw->AddCircle(center, 7.2f * unit, color, 0, 1.5f * unit);
        draw->AddCircleFilled(ImVec2(center.x, center.y - 3.4f * unit), 1.2f * unit, color);
        draw->AddLine(ImVec2(center.x, center.y - 1.0f * unit), ImVec2(center.x, center.y + 3.8f * unit), color, 2.2f * unit);
        draw->AddLine(ImVec2(center.x - 1.6f * unit, center.y - 0.2f * unit), ImVec2(center.x, center.y - 1.0f * unit), color, 1.5f * unit);
        draw->AddLine(ImVec2(center.x - 1.8f * unit, center.y + 3.8f * unit), ImVec2(center.x + 1.8f * unit, center.y + 3.8f * unit), color, 1.5f * unit);
    }
}
inline bool NavigationButton(const char *id, const char *text, bool selected, float width, int icon = -1) {
    const float unit = ImGui::GetStyle().FontScaleMain;
    const auto start = ImGui::GetCursorScreenPos();
    const ImVec2 size(width, 34 * unit);
    const bool pressed = ImGui::InvisibleButton(id, size, ImGuiButtonFlags_EnableNav);
    auto *draw = ImGui::GetWindowDrawList();
    const bool hovered = ImGui::IsItemHovered();
    const bool focused = ImGui::IsItemFocused();
    if (selected || hovered || focused) {
        const auto accent = ImGui::GetStyle().Colors[ImGuiCol_CheckMark];
        const ImVec4 color(selected ? .09f + accent.x * .18f : .18f, selected ? .10f + accent.y * .18f : .21f, selected ? .12f + accent.z * .18f : .28f, 1);
        draw->AddRectFilled(start, ImVec2(start.x + size.x, start.y + size.y), ImGui::GetColorU32(color), 8 * unit);
    }
    const float textHeight = ImGui::CalcTextSize(text).y;
    const float textStartX = icon >= 0 ? start.x + 35 * unit : start.x + 14 * unit;
    draw->AddText(ImVec2(textStartX, start.y + (size.y - textHeight) * .5f), ImGui::GetColorU32(ImGuiCol_Text), text);
    if (icon >= 0) {
        const ImVec2 iconCenter(start.x + 18 * unit, start.y + size.y * .5f);
        const ImU32 iconColor = selected ? ImGui::GetColorU32(ImGuiCol_CheckMark)
                              : (hovered || focused) ? ImGui::GetColorU32(ImGuiCol_Text)
                              : ImGui::GetColorU32(ImGuiCol_TextDisabled);
        DrawNavigationIcon(draw, icon, iconCenter, unit, iconColor);
    }
    return pressed;
}
inline bool CloseButton(float unit) {
    const auto start = ImGui::GetCursorScreenPos();
    const ImVec2 size(30 * unit, 30 * unit);
    const bool pressed = ImGui::InvisibleButton("##close", size, ImGuiButtonFlags_EnableNav);
    if (!ImGui::IsItemVisible()) return pressed;
    auto *draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(start, ImVec2(start.x + size.x, start.y + size.y), ImGui::GetColorU32(ImGui::IsItemHovered() ? ImGuiCol_ButtonHovered : ImGuiCol_Button), 6 * unit);
    const ImVec2 center(start.x + size.x * .5f, start.y + size.y * .5f);
    const float radius = 4 * unit;
    const auto ink = ImGui::GetColorU32(ImGuiCol_Text);
    draw->AddLine(ImVec2(center.x - radius, center.y - radius), ImVec2(center.x + radius, center.y + radius), ink, 1.4f * unit);
    draw->AddLine(ImVec2(center.x - radius, center.y + radius), ImVec2(center.x + radius, center.y - radius), ink, 1.4f * unit);
    return pressed;
}
inline void SameLineFor(const char *label) {
    const auto &style = ImGui::GetStyle();
    const float remaining = ImGui::GetWindowPos().x + ImGui::GetWindowWidth() - style.WindowPadding.x - ImGui::GetItemRectMax().x;
    if (remaining >= ImGui::CalcTextSize(label).x + style.FramePadding.x * 2 + style.ItemSpacing.x) ImGui::SameLine();
}
// Retain ImGui's mouse, keyboard and CTRL-click input behavior; replace only the drawing.
inline bool Slider(const char *id, ImGuiDataType type, void *value, double min, double max, const char *format) {
    int imin = static_cast<int>(min), imax = static_cast<int>(max);
    float fmin = static_cast<float>(min), fmax = static_cast<float>(max);
    double dmin = min, dmax = max;
    const float unit = ImGui::GetStyle().FontScaleMain;
    const float width = ImGui::CalcItemWidth();
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetIO().WantTextInput ? ImGui::GetStyleColorVec4(ImGuiCol_Text) : ImVec4(0,0,0,0));
    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 12 * unit);
    const bool changed = ImGui::SliderScalar(id, type, value, type == ImGuiDataType_S32 ? static_cast<void *>(&imin) : type == ImGuiDataType_Double ? static_cast<void *>(&dmin) : &fmin,
        type == ImGuiDataType_S32 ? static_cast<void *>(&imax) : type == ImGuiDataType_Double ? static_cast<void *>(&dmax) : &fmax, format, ImGuiSliderFlags_AlwaysClamp);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(6);
    if (!ImGui::IsItemVisible() || (ImGui::IsItemActive() && ImGui::GetIO().WantTextInput)) return changed;
    const auto a = ImGui::GetItemRectMin();
    const ImVec2 b(a.x + width, ImGui::GetItemRectMax().y);
    auto *draw = ImGui::GetWindowDrawList();
    const auto bg = ImGui::GetColorU32(ImGuiCol_ChildBg);
    const float left = a.x + 8 * unit, right = b.x - 8 * unit, y = (a.y + b.y) * .5f;
    const double number = type == ImGuiDataType_S32 ? *static_cast<int *>(value) : type == ImGuiDataType_Double ? *static_cast<double *>(value) : *static_cast<float *>(value);
    const float ratio = max > min ? static_cast<float>(std::clamp((number - min) / (max - min), 0.0, 1.0)) : 0;
    const float x = left + ratio * std::max(0.f, right - left);
    draw->AddLine(ImVec2(left, y), ImVec2(right, y), ImGui::GetColorU32(ImGuiCol_Border), 3 * unit);
    draw->AddLine(ImVec2(left, y), ImVec2(x, y), ImGui::GetColorU32(ImGuiCol_CheckMark), 3 * unit);
    draw->AddCircleFilled(ImVec2(x, y), 6 * unit, ImGui::GetColorU32(ImGuiCol_CheckMark));
    char text[48];
    if (type == ImGuiDataType_S32) std::snprintf(text, sizeof(text), format, static_cast<int>(number));
    else std::snprintf(text, sizeof(text), format, number);
    const auto size = ImGui::CalcTextSize(text);
    const ImVec2 center((a.x + b.x - size.x) * .5f, (a.y + b.y - size.y) * .5f);
    draw->AddRectFilled(ImVec2(center.x - 5 * unit, center.y - 1), ImVec2(center.x + size.x + 5 * unit, center.y + size.y + 1), bg, 3 * unit);
    draw->AddText(center, ImGui::GetColorU32(ImGuiCol_Text), text);
    if (ImGui::IsItemFocused()) draw->AddRect(a, b, ImGui::GetColorU32(ImGuiCol_CheckMark), 6 * unit);
    return changed;
}
inline bool Keycap(const char *id, const char *name, bool waiting) {
    const float height = ImGui::GetFrameHeight(), unit = ImGui::GetStyle().FontScaleMain;
    const float width = std::max(height, ImGui::CalcTextSize(name).x + 16 * unit);
    const auto a = ImGui::GetCursorScreenPos();
    const bool pressed = ImGui::InvisibleButton(id, ImVec2(width, height), ImGuiButtonFlags_EnableNav);
    if (!ImGui::IsItemVisible()) return pressed;
    auto *draw = ImGui::GetWindowDrawList();
    const ImVec2 b(a.x + width, a.y + height);
    draw->AddRectFilled(a, b, ImGui::GetColorU32(waiting || ImGui::IsItemHovered() ? ImGuiCol_ButtonHovered : ImGuiCol_FrameBg), 5 * unit);
    draw->AddRect(a, b, ImGui::GetColorU32(waiting || ImGui::IsItemFocused() ? ImGuiCol_CheckMark : ImGuiCol_Border), 5 * unit);
    draw->AddLine(ImVec2(a.x + 5 * unit,b.y - 3 * unit),ImVec2(b.x - 5 * unit,b.y - 3 * unit),ImGui::GetColorU32(ImGuiCol_Border),unit);
    const auto size = ImGui::CalcTextSize(name);
    draw->AddText(ImVec2(a.x + (width-size.x)*.5f,a.y + (height-size.y)*.5f-1*unit),ImGui::GetColorU32(ImGuiCol_Text),name);
    return pressed;
}
inline bool Toggle(const char *label, bool *value) {
    const float height = ImGui::GetFrameHeight() * .72f, width = height * 1.85f;
    const auto start = ImGui::GetCursorScreenPos();
    const bool pressed = ImGui::InvisibleButton(label, ImVec2(width, ImGui::GetFrameHeight()), ImGuiButtonFlags_EnableNav);
    if (pressed) *value = !*value;
    if (!ImGui::IsItemVisible()) return pressed;
    auto *draw = ImGui::GetWindowDrawList();
    const float top = start.y + (ImGui::GetFrameHeight() - height) * .5f;
    const auto color = ImGui::GetStyle().Colors[*value ? ImGuiCol_CheckMark : ImGui::IsItemHovered() ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg];
    draw->AddRectFilled(ImVec2(start.x, top), ImVec2(start.x + width, top + height), ImGui::GetColorU32(color), height * .5f);
    draw->AddCircleFilled(ImVec2(start.x + (*value ? width - height * .5f : height * .5f), top + height * .5f), height * .35f, ImGui::GetColorU32(IM_COL32(235, 243, 248, 255)));
    if (ImGui::IsItemFocused()) draw->AddRect(start, ImVec2(start.x + width, start.y + ImGui::GetFrameHeight()), ImGui::GetColorU32(ImGuiCol_Text), 4);
    return pressed;
}
struct SmoothScroll {
    float target = 0.0f;
    float current = 0.0f;
    float applied = 0.0f;
    bool initialized = false;

    void Reset(float value = 0.0f) {
        target = current = applied = value;
        initialized = true;
    }

    void Update() {
        const float actual = ImGui::GetScrollY();
        const float maxScroll = ImGui::GetScrollMaxY();
        if (!initialized) {
            Reset(actual);
            return;
        }
        if (std::abs(actual - applied) >= 1.0f) {
            target = current = applied = actual;
        }
        const bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
        const float wheel = ImGui::GetIO().MouseWheel;
        if (hovered && wheel != 0.0f && !ImGui::GetIO().KeyCtrl && maxScroll > 0.0f) {
            const float step = std::trunc(std::min(5.0f * ImGui::GetFontSize(), ImGui::GetWindowHeight() * 0.67f));
            target -= wheel * step;
        }
        target = std::clamp(target, 0.0f, maxScroll);
        if (std::abs(current - target) > 0.1f) {
            const float dt = std::clamp(ImGui::GetIO().DeltaTime, 0.0f, 0.1f);
            current += (target - current) * (1.0f - std::exp(-18.0f * dt));
            if (std::abs(target - current) < 0.5f) current = target;
            current = std::clamp(current, 0.0f, maxScroll);
            applied = std::round(current);
            ImGui::SetScrollY(applied);
        } else {
            current = target;
            applied = actual;
        }
    }
};

inline SmoothScroll contentScroll;

inline void BeginShell(int &page, bool &visible, const char *version, const Preferences &prefs = {}) {
    const float unit = ImGui::GetStyle().FontScaleMain;
    const auto display = ImGui::GetIO().DisplaySize;
    static ImVec2 previousDisplay;
    static bool windowPlaced = false;

    const bool displayChanged = previousDisplay.x > 0 && (previousDisplay.x != display.x || previousDisplay.y != display.y);
    previousDisplay = display;

    const ImVec2 defaultSize(std::min(1080.f * unit, display.x - 24), std::min(640.f * unit, display.y - 24));
    const ImVec2 defaultPos(display.x * .5f, display.y * .5f);

    if (resetWindowRequested) {
        resetWindowRequested = false;
        ImGui::SetNextWindowSize(defaultSize, ImGuiCond_Always);
        ImGui::SetNextWindowPos(defaultPos, ImGuiCond_Always, ImVec2(.5f, .5f));
    } else if (!windowPlaced) {
        windowPlaced = true;
        if (prefs.hasWindowRect) {
            const float minW = std::min(720.f * unit, display.x - 16);
            const float minH = std::min(440.f * unit, display.y - 16);
            const float maxW = std::max(1.f, display.x - 16);
            const float maxH = std::max(1.f, display.y - 16);
            const float sizeW = std::clamp(prefs.windowSize.x, minW, maxW);
            const float sizeH = std::clamp(prefs.windowSize.y, minH, maxH);
            const float posX = std::clamp(prefs.windowPos.x, 0.0f, std::max(0.0f, display.x - sizeW));
            const float posY = std::clamp(prefs.windowPos.y, 0.0f, std::max(0.0f, display.y - sizeH));
            ImGui::SetNextWindowSize(ImVec2(sizeW, sizeH), ImGuiCond_Always);
            ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always);
        } else {
            ImGui::SetNextWindowSize(defaultSize, ImGuiCond_Always);
            ImGui::SetNextWindowPos(defaultPos, ImGuiCond_Always, ImVec2(.5f, .5f));
        }
    } else if (displayChanged) {
        const float minW = std::min(720.f * unit, display.x - 16);
        const float minH = std::min(440.f * unit, display.y - 16);
        const float maxW = std::max(1.f, display.x - 16);
        const float maxH = std::max(1.f, display.y - 16);
        const float sizeW = std::clamp(lastShellSize.x > 0 ? lastShellSize.x : defaultSize.x, minW, maxW);
        const float sizeH = std::clamp(lastShellSize.y > 0 ? lastShellSize.y : defaultSize.y, minH, maxH);
        const float posX = std::clamp(lastShellPos.x, 0.0f, std::max(0.0f, display.x - sizeW));
        const float posY = std::clamp(lastShellPos.y, 0.0f, std::max(0.0f, display.y - sizeH));
        ImGui::SetNextWindowSize(ImVec2(sizeW, sizeH), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always);
    }

    ImGui::SetNextWindowSizeConstraints(ImVec2(std::min(720.f * unit, display.x - 16), std::min(440.f * unit, display.y - 16)), ImVec2(std::max(1.f, display.x - 16), std::max(1.f, display.y - 16)));
    ImGui::Begin("ModelExtras Studio###MEStudio", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    lastShellPos = ImGui::GetWindowPos();
    lastShellSize = ImGui::GetWindowSize();
    const auto start = ImGui::GetCursorScreenPos();
    auto *draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(start, ImVec2(start.x + 38 * unit, start.y + 38 * unit), ImGui::GetColorU32(IM_COL32(48, 53, 72, 255)), 8 * unit);
    const auto ink = ImGui::GetColorU32(IM_COL32(168, 182, 221, 255));
    // Compact vehicle emblem: vector artwork remains sharp at every UI scale.
    draw->AddRect(ImVec2(start.x + 7 * unit, start.y + 17 * unit), ImVec2(start.x + 31 * unit, start.y + 27 * unit), ink, 3 * unit, 1.7f * unit);
    draw->AddLine(ImVec2(start.x + 11 * unit, start.y + 17 * unit), ImVec2(start.x + 15 * unit, start.y + 10 * unit), ink, 1.7f * unit);
    draw->AddLine(ImVec2(start.x + 15 * unit, start.y + 10 * unit), ImVec2(start.x + 25 * unit, start.y + 10 * unit), ink, 1.7f * unit);
    draw->AddLine(ImVec2(start.x + 25 * unit, start.y + 10 * unit), ImVec2(start.x + 29 * unit, start.y + 17 * unit), ink, 1.7f * unit);
    draw->AddCircleFilled(ImVec2(start.x + 12 * unit, start.y + 28 * unit), 3 * unit, ink);
    draw->AddCircleFilled(ImVec2(start.x + 27 * unit, start.y + 28 * unit), 3 * unit, ink);
    ImGui::SetCursorScreenPos(ImVec2(start.x + 52 * unit, start.y + 5 * unit));
    ImGui::PushFont(nullptr, 22);
    ImGui::Text("ModelExtras v%s", version);
    ImGui::PopFont();
    ImGui::SetCursorScreenPos(ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x - 30 * unit, start.y + 3 * unit));
    if (CloseButton(unit)) visible = false;
    ImGui::SetCursorScreenPos(ImVec2(start.x, start.y + 47 * unit));
    ImGui::Separator();
    const float body = std::max(100.f, ImGui::GetContentRegionAvail().y);
    ImGui::BeginChild("navigation", ImVec2(160 * unit, body), ImGuiChildFlags_Borders);
    const char *names[] = {"Settings", "Workshop", "Appearance", "About"};
    for (int i = 0; i < 4; ++i) {
        ImGui::PushID(i);
        if (NavigationButton("##page", names[i], page == i, ImGui::GetContentRegionAvail().x, i)) page = i;
        ImGui::PopID();
    }
    ImGui::EndChild();
    ImGui::SameLine();
    static int previousPage = -1;
    if (page != previousPage) {
        ImGui::SetNextWindowScroll(ImVec2(0, 0));
        contentScroll.Reset(0.0f);
    }
    previousPage = page;
    ImGui::BeginChild("content", ImVec2(0, body), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PushItemWidth(std::max(130.f * unit, ImGui::GetContentRegionAvail().x * .55f));
}
inline void EndShell(const char *status) {
    ImGui::PopItemWidth();
    contentScroll.Update();
    ImGui::EndChild();
    lastShellPos = ImGui::GetWindowPos();
    lastShellSize = ImGui::GetWindowSize();
    ImGui::End();
}
}
