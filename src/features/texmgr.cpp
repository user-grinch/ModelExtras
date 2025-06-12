#include "pch.h"
#include "defines.h"
#include "texmgr.h"
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

RwTexture *TextureMgr::Get(std::string name, RwUInt8 alpha)
{
    if (Textures.contains(name) && Textures[name].contains(alpha) && Textures[name][alpha])
    {
        return Textures[name][alpha];
    }

    static auto pDict = CFileLoader::LoadTexDictionary(MOD_DATA_PATH("ME_TEXDB.TXD"));
    RwTexture *pTex = RwTexDictionaryFindNamedTexture(pDict, name.c_str());
    if (!pTex) {
        return nullptr;
    }

    Textures[name][alpha] = pTex;
    // int index = CTxdStore::FindTxdSlot("ME_TEXDB");
    // if (index == -1)
    // {
    //     index = CTxdStore::AddTxdSlot("ME_TEXDB");
    //     CTxdStore::LoadTxd(index, MOD_DATA_PATH("ME_TEXDB.TXD"));
    //     CTxdStore::AddRef(index);
    // }
    // CTxdStore::PushCurrentTxd();
    // CTxdStore::SetCurrentTxd(index);

    // Textures[name][alpha] = RwReadTexture(name.c_str());

    if (alpha != 255)
    {
        SetAlpha(Textures[name][alpha], alpha);
    }
    // CTxdStore::PopCurrentTxd();
    return Textures[name][alpha];
}

RwTexture *TextureMgr::FindTextureInDict(RpMaterial *pMat, RwTexDictionary *pDict)
{
    if (!pMat || !pMat->texture) {
        return nullptr;
    }
	const std::string baseName = pMat->texture->name;
	const std::vector<std::string> texNames = {
		baseName + "on",
		baseName + "_on",
	};

	RwTexture *pTex = nullptr;
	for (const auto &name : texNames)
	{
		pTex = TextureMgr::FindInDict(name, pDict);
		if (pTex)
		{
			break;
		}
	}
	return pTex;
}

void TextureMgr::SetAlpha(RwTexture *texture, RwUInt8 alpha)
{
    if (!texture)
        return;

    RwRaster *oldRaster = RwTextureGetRaster(texture);
    if (!oldRaster)
        return;

    int width = RwRasterGetWidth(oldRaster);
    int height = RwRasterGetHeight(oldRaster);

    RwImage *image = RwImageCreate(width, height, 32); // 32-bit = supports RGBA
    RwImageAllocatePixels(image);
    RwImageSetFromRaster(image, oldRaster);

    RwRGBA *pixels = (RwRGBA *)RwImageGetPixels(image);
    for (int y = 1; y < height; ++y)
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

RwTexture *TextureMgr::FindInDict(std::string name, RwTexDictionary *pDict)
{
    if (!pDict)
    {
        return nullptr;
    }
    return RwTexDictionaryFindNamedTexture(pDict, name.c_str());
}