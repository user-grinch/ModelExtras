#pragma once
#include "materials.h"
#include "game_sa/CGeneral.h"
#include "game_sa/RenderWare.h"

enum class eDummyPos { 
    Front, 
    Left, 
    Rear, 
    Right,
    None
};

class VehicleDummy {
private:
    static int ReadHex(char a, char b);
    bool hasParent = false;

public:
    RwFrame* Frame;
    RwRGBA Color = { 255, 255, 255, 128 };
    CVector Position;
    eDummyPos Type;
    float Size;
    float Angle;
    float CurrentAngle = 0.0f;

    VehicleDummy(RwFrame* frame, std::string name, bool parent, eDummyPos type = eDummyPos::None, RwRGBA color = { 255, 255, 255, 128 });
};
