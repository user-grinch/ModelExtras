#pragma once
#include "imgui.h"
#include <string>
#include <vector>

class FontMgr
{
  private:
    struct FontInfo
    {
        ImFont *pFont;
        float fMul, fBaseSize;
        std::string sID;
        const char *pData;
        bool bIconFont;

        FontInfo(ImFont *f, std::string i, const char *d, float mul, bool icon = false)
            : pFont(f), fMul(mul), sID(std::move(i)), pData(d), bIconFont(icon)
        {
        }
    };

    static inline std::vector<FontInfo> vecLoadedFonts;

    static const ImWchar *GetGlyphRangesInternal(bool isIcon);

    static float GetScaleFactor(float w = -1, float h = -1);

  public:
    FontMgr() = delete;
    FontMgr(const FontMgr &) = delete;

    static ImFont *Get(const char *fontID);
    static ImFont *LoadFont(const char *fontID, const char *data, float fontMul = 1.0f, bool isIcon = false);
    static void RescaleFonts(float w, float h);
    static void UnloadAll();
};
