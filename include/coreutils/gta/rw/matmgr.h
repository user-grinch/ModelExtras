#pragma once
#include "plugin.h"

struct MaterialMgr
{
    struct Info
    {
        bool m_bRecolor = false;
        bool m_bRetexture = false;
        RwRGBA m_nColor{0, 0, 0, 0};
        RwTexture *m_pTexture = nullptr;
        RpGeometry *m_pGeometry = nullptr;
        RwRGBA m_nOriginalColor{0, 0, 0, 0};
        RwTexture *m_pOriginalTexture = nullptr;
        RwInt32 m_nOriginalGeometryFlags = 0;
    };

    std::array<uchar, 4> m_nCarColors;
    std::string m_nTextureName;
    std::unordered_map<RpMaterial *, Info> m_nMapInfoList;

    void ResetColor(RpMaterial *pMat);
    void ResetTexture(RpMaterial *pMat);
    void SetColor(RpMaterial *pMat, RpGeometry *pGeo, RwRGBA color, bool noChecks = false);
    void SetTexture(RpMaterial *pMat, RwTexture *pTex, bool noChecks = false);
};
