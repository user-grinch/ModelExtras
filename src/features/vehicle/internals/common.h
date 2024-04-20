#pragma once
#include <plugin.h>
using uchar = unsigned char;

class Common {
private:
    static inline std::map<std::string, RwTexture*> Textures;

public:
	static RwTexture* GetTexture(std::string texName);
    static void RegisterCorona(CVehicle* pVeh, CVector pos, uchar red, uchar green, uchar blue, uchar alpha, int id, float size, float dummyAngle, bool skipChecks = false);
    static void RegisterCoronaWithAngle(CVehicle* pVeh, CVector posn, uchar red, uchar green, uchar blue, uchar alpha, int id, float cameraAngle, float angle, float radius, float size);
    static void RegisterShadow(CVehicle* pVeh, CVector pos, uchar red, uchar green, uchar blue, float angle, float dummyAngle);
};