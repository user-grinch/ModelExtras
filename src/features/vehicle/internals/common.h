#pragma once
#include <plugin.h>
using uchar = unsigned char;

class Common {
public:
    static void RegisterCorona(CVehicle* pVeh, CVector pos, uchar red, uchar green, uchar blue, uchar alpha, int id,  float size);
    static void RegisterCoronaWithAngle(CVehicle* pVeh, CVector posn, uchar red, uchar green, uchar blue, uchar alpha, int id, float cameraAngle, float angle, float radius, float size);
    static void RegisterShadow(CVehicle* pVeh, CVector pos, uchar red, uchar green, uchar blue, float angle, float currentAngle);
};