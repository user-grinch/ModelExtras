#pragma once
#include <plugin.h>
using uchar = unsigned char;

class Common {
private:
    static inline std::map<std::string, RwTexture*> Textures;

public:
	static RwTexture* GetTexture(std::string texName);
    static void RegisterCorona(CVehicle* pVeh, CVector pos, uchar red, uchar green, uchar blue, uchar alpha, int id, float size);
    static void RegisterCoronaWithAngle(CVehicle* pVeh, CVector posn, uchar red, uchar green, uchar blue, uchar alpha, int id, float angle, float radius, float size);
    static void RegisterShadow(CVehicle* pVeh, CVector position, unsigned char red, unsigned char green, unsigned char blue, unsigned int alpha, float angle, float currentAngle, const std::string& shadwTexName, float shdwSz = 1.0f, float shdwOffset = 0.0f, RwTexture *pTexture = nullptr);
};