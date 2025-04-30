#include "pch.h"
#include "texmgr.h"
#include <CTxdStore.h>
#include <rwcore.h>
#include <rwplcore.h>
#include <rpworld.h>
#include <RenderWare.h>

RwTexture *LoadPNGFromFile(const char *filename, RwUInt8 alpha)
{
    RwImage *image = RtPNGImageRead(filename);
    if (!image)
    {
        return nullptr;
    }

    RwInt32 width, height, depth, flags;
    RwImageFindRasterFormat(image, 4, &width, &height, &depth, &flags);

    RwRaster *raster = RwRasterCreate(width, height, depth, flags);
    if (!raster)
    {
        RwImageDestroy(image);
        return nullptr;
    }

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

    RwRasterSetFromImage(raster, image);
    RwImageDestroy(image);
    return RwTextureCreate(raster);
}

RwTexture *TextureMgr::Get(std::string path, RwUInt8 alpha)
{
    std::string fullPath = MOD_DATA_PATH("textures/") + path;
    if (Textures.contains(path) && Textures[path].contains(alpha) && Textures[path][alpha])
    {
        return Textures[path][alpha];
    }

    if (path.ends_with(".dds"))
    {
        std::string name_without_ext = fullPath.substr(0, fullPath.find_last_of('.'));
        Textures[path][alpha] = RwD3D9DDSTextureRead(name_without_ext.c_str(), NULL);
    }
    else if (path.ends_with(".png"))
    {
        Textures[path][alpha] = LoadPNGFromFile(fullPath.c_str(), alpha);
    }
    else
    {
        return nullptr;
    }

    return Textures[path][alpha];
};

RwTexture *TextureMgr::FindInDict(RpMaterial *pMat, RwTexDictionary *pDict)
{
    const std::string baseName = pMat->texture->name;

    // texture glitch fix
    const std::vector<std::string> texNames = {
        // baseName,
        baseName + "on",
        baseName + "_on",
        // "sirenlighton",
        // "sirenlight_on",
        // "vehiclelightson128"
    };

    RwTexture *pTex = nullptr;
    for (const auto &name : texNames)
    {
        pTex = RwTexDictionaryFindNamedTexture(pDict, name.c_str());
        if (pTex)
        {
            break;
        }
    }
    return pTex;
}