#include "pch.h"

#include "matmgr.h"

void MaterialMgr::SetColor(RpMaterial *material, RpGeometry *geometry, RwRGBA color, bool noChecks)
{
    auto &matInfo = m_nMapInfoList[material];
    if (noChecks || (material->color.red == 0x3C && material->color.green == 0xFF && material->color.blue == 0x00) ||
        (material->color.red == 0xFF && material->color.green == 0x00 && material->color.blue == 0xAF))
    {
        matInfo.m_bRecolor = true;
        matInfo.m_nColor = color;
        matInfo.m_pGeometry = geometry;
    }
}

void MaterialMgr::SetTexture(RpMaterial *material, RwTexture *texture, bool noChecks)
{
    auto &matInfo = m_nMapInfoList[material];
    if (noChecks || (material->color.red == 0x3C && material->color.green == 0xFF && material->color.blue == 0x00) ||
        (material->color.red == 0xFF && material->color.green == 0x00 && material->color.blue == 0xAF))
    {
        matInfo.m_bRetexture = true;
        matInfo.m_pTexture = texture;
    }
}

void MaterialMgr::ResetColor(RpMaterial *material)
{
    auto &matInfo = m_nMapInfoList[material];
    matInfo.m_bRecolor = false;
    matInfo.m_nColor = {0, 0, 0, 0};
}

void MaterialMgr::ResetTexture(RpMaterial *material)
{
    auto &matInfo = m_nMapInfoList[material];
    matInfo.m_bRetexture = false;
    matInfo.m_pTexture = nullptr;
}
