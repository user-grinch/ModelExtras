#include "pch.h"
#include "util.h"
#include <CMenuManager.h>
#include <cstring>
#include <cctype>

bool Util::IsWindowFocused() {
    HWND hGameWnd = *(HWND*)0xC97C1C;
    if (hGameWnd && GetForegroundWindow() != hGameWnd) {
        return false;
    }
    if (FrontEndMenuManager.m_bMenuActive) {
        return false;
    }
    return true;
}

CRGBA Util::GetMaterialColor(RpMaterial *pMat) {
    if (!pMat) return CRGBA(0, 0, 0, 0);
    RwRGBA *pColor = RpMaterialGetColor(pMat);
    return CRGBA(pColor->red, pColor->green, pColor->blue, 255);
}

bool Util::IsAntiPatternLightMaterial(RpMaterial *pMat)
{
    if (!pMat || !pMat->texture || !pMat->texture->name) {
        return false;
    }

    char lowerName[32];
    size_t len = strnlen(pMat->texture->name, sizeof(lowerName) - 1);
    for (size_t i = 0; i < len; ++i) {
        lowerName[i] = static_cast<char>(std::tolower(pMat->texture->name[i]));
    }
    lowerName[len] = '\0';
    std::string_view texName(lowerName, len);

    return texName == "vehicleenvmap128" ||
           texName == "vehicleenvmap" ||
           texName == "x_cube" ||
           texName == "cubemap" ||
           texName == "reflection" ||
           texName.starts_with("x_cube_") ||
           texName.starts_with("vehicleenvmap");
}
