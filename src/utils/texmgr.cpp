#include "pch.h"
#include "defines.h"
#include "utils/texmgr.h"
#include <CTxdStore.h>
#include <rwcore.h>
#include <rwplcore.h>
#include <rpworld.h>
#include <RenderWare.h>
#include <CFileLoader.h>

RwTexture *LoadPNGFromFile(const char *filename, RwUInt8 alpha)
{
    RwImage *image = RtPNGImageRead(filename);
    if (!image)
    {
        return nullptr;
    }

    RwInt32 width, height, depth, flags;
    RwImageFindRasterFormat(image, rwRASTERTYPETEXTURE | rwRASTERFORMAT888, &width, &height, &depth, &flags);

    RwRaster *raster = RwRasterCreate(width, height, depth, flags);
    if (!raster)
    {
        RwImageDestroy(image);
        return nullptr;
    }
    if (alpha != 255)
    {
        // Set the alpha value for each pixel
        RwRGBA *pixels = (RwRGBA *)RwImageGetPixels(image);
        for (RwInt32 y = 0; y < height; y++)
        {
            for (RwInt32 x = 0; x < width; x++)
            {
                RwRGBA *pixel = pixels + (y * width + x);
                pixel->red = (pixel->red * alpha) / 255;
                pixel->green = (pixel->green * alpha) / 255;
                pixel->blue = (pixel->blue * alpha) / 255;
                pixel->alpha = alpha;
            }
        }
    }

    RwRasterSetFromImage(raster, image);
    RwImageDestroy(image);
    return RwTextureCreate(raster);
}

RwTexture *TextureMgr::RwReadTexture(const char *name, char *Maskname)
{
    return ((RwTexture * (__cdecl *)(char const *, char const *))0x4C7510)(name, Maskname);
}

RwTexture *TextureMgr::Get(std::string_view name, RwUInt8 alpha)
{
    auto it = Textures.find(name);
    if (it != Textures.end())
    {
        auto itAlpha = it->second.find(alpha);
        if (itAlpha != it->second.end() && itAlpha->second)
        {
            return itAlpha->second;
        }
    }

    static auto pDict = CFileLoader::LoadTexDictionary(MOD_DATA_PATH("ME_TEXDB.TXD"));
    char nameBuf[64];
    size_t copyLen = std::min(name.size(), sizeof(nameBuf) - 1);
    std::memcpy(nameBuf, name.data(), copyLen);
    nameBuf[copyLen] = '\0';

    RwTexture *pTex = RwTexDictionaryFindNamedTexture(pDict, nameBuf);
    if (pTex == nullptr) {
        return nullptr;
    }

    std::string keyStr(name);
    Textures[keyStr][alpha] = pTex;

    if (alpha != 255)
    {
        SetAlpha(Textures[keyStr][alpha], alpha);
    }
    return Textures[keyStr][alpha];
}

RwTexture *TextureMgr::FindOnTextureInDict(RpMaterial *pMat, RwTexDictionary *pDict, bool fallback)
{
    if ((pMat == nullptr) || (pMat->texture == nullptr) || (pMat->texture->name == nullptr)) {
        return nullptr;
    }

    const char *baseName = pMat->texture->name;
    size_t baseLen = std::strlen(baseName);
    if (baseLen == 0 || baseLen >= 58) {
        return nullptr;
    }

    char texBuf[64];
    // Try baseName + "on"
    std::memcpy(texBuf, baseName, baseLen);
    texBuf[baseLen] = 'o';
    texBuf[baseLen + 1] = 'n';
    texBuf[baseLen + 2] = '\0';

    RwTexture *pTex = TextureMgr::FindInDict(std::string_view(texBuf, baseLen + 2), pDict, fallback);
    if (pTex != nullptr) {
        return pTex;
    }

    // Try baseName + "_on"
    texBuf[baseLen] = '_';
    texBuf[baseLen + 1] = 'o';
    texBuf[baseLen + 2] = 'n';
    texBuf[baseLen + 3] = '\0';

    return TextureMgr::FindInDict(std::string_view(texBuf, baseLen + 3), pDict, fallback);
}

void TextureMgr::SetAlpha(RwTexture *texture, RwUInt8 alpha)
{
    if (texture == nullptr) {
        return;
    }

    RwRaster *oldRaster = RwTextureGetRaster(texture);
    if (oldRaster == nullptr) {
        return;
    }

    int width = RwRasterGetWidth(oldRaster);
    int height = RwRasterGetHeight(oldRaster);

    RwImage *image = RwImageCreate(width, height, 32); // 32-bit = supports RGBA
    RwImageAllocatePixels(image);
    RwImageSetFromRaster(image, oldRaster);

    RwRGBA *pixels = (RwRGBA *)RwImageGetPixels(image);
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            RwRGBA *pixel = &pixels[y * width + x];
            pixel->red = (pixel->red * alpha) / 255;
            pixel->green = (pixel->green * alpha) / 255;
            pixel->blue = (pixel->blue * alpha) / 255;
            pixel->alpha = alpha;
        }
    }

    RwRasterDestroy(oldRaster);
    RwRaster *newRaster = RwRasterCreate(width, height, 32, rwRASTERTYPETEXTURE | rwRASTERFORMAT8888);
    RwRasterSetFromImage(newRaster, image);
    texture->raster = newRaster;
    RwImageDestroy(image);
}

// Priority
// 1. Vehicle's txd 
// 2. ModelExtras txd
// 3. vehicle.txd (Supports vehfuncs additional txds)
RwTexture *TextureMgr::FindInDict(std::string_view name, RwTexDictionary *pDict, bool fallback)
{
    if (name.empty()) return nullptr;

    char nameBuf[64];
    size_t copyLen = std::min(name.size(), sizeof(nameBuf) - 1);
    std::memcpy(nameBuf, name.data(), copyLen);
    nameBuf[copyLen] = '\0';

    RwTexture *pTex = nullptr;

    if (pDict)
    {
        pTex = RwTexDictionaryFindNamedTexture(pDict, nameBuf);
    }

    if (fallback) {
        if (!pTex) {
            LOG_VERBOSE("TextureMgr: Unable to find '{}' in the vehicle's TXD file. Searching in the ModelExtras TXD file instead.", name);
            pTex = TextureMgr::Get(name);
        }

        if (!pTex) {
            LOG_VERBOSE("TextureMgr: Unable to find '{}' in the ModelExtras TXD file. Searching in the vehicle TXD file instead.", name);
            pTex = RwTexDictionaryFindNamedTexture(CVehicleModelInfo::ms_pVehicleTxd, nameBuf);
        }

        if (!pTex) {
            LOG_VERBOSE("TextureMgr: Unable to find '{}' in the vehicle TXD file. Using the default white texture", name);
            pTex = CVehicleModelInfo::ms_pWhiteTexture;
        }
    }

    return pTex;
}