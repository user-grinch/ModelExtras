#pragma once
#include <map>
#include <plugin.h>

class TextureMgr
{
private:
    static inline std::map<std::string, std::map<RwUInt8, RwTexture *>> Textures;

public:
    static RwTexture *FindInDict(std::string name, RwTexDictionary *pDict, bool fallback = false);
    static RwTexture *FindOnTextureInDict(RpMaterial *pMat, RwTexDictionary *pDict, bool fallback = false);
    static RwTexture *LoadFromFile(const char *filename, RwUInt8 alpha = 255);
    static RwTexture *RwReadTexture(const char *name, char *Maskname = NULL);
    static RwTexture *Get(std::string texName, RwUInt8 alpha = 255);
    static void SetAlpha(RwTexture *texture, RwUInt8 alpha = 255);
};