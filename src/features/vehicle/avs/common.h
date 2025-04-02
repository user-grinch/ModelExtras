#pragma once
#include <plugin.h>
using uchar = unsigned char;

class Common
{
private:
    static inline std::map<std::string, RwTexture *> Textures;

public:
    static float NormalizeAngle(float angle);
    static RwTexture *GetTexture(std::string texName);
    static void RegisterCorona(CVehicle *pVeh, int coronaID, CVector pos, uchar red, uchar green, uchar blue, uchar alpha, float size);
    static void RegisterCoronaWithAngle(CVehicle *pVeh, int coronaID, CVector posn, uchar red, uchar green, uchar blue, uchar alpha, float angle, float radius, float size);
    static void RegisterShadow(CVehicle *pVeh, CVector position, CRGBA col, float angle, float currentAngle, const std::string &shadwTexName, CVector2D shdwSz = {1.0f, 1.0f}, CVector2D shdwOffset = {0.0f, 0.0f}, RwTexture *pTexture = nullptr);
};