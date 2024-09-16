#pragma once
#include "materials.h"
#include "game_sa/CGeneral.h"
#include "game_sa/RenderWare.h"

enum class eDummyPos { 
    // remove later
    Front,
    Rear,
    Left,
    Right,
    
    MiddleLeft, 
    MiddleRight, 
    FrontLeft, 
    RearLeft, 
    FrontRight, 
    RearRight, 
    None
};

class VehicleDummy {
private:
    static int ReadHex(char a, char b);
    bool hasParent = false;

public:
    RwFrame* Frame;
    RwRGBA Color = { 255, 255, 255, 128 };
    CVector Position, ShdwPosition;
    eDummyPos Type;
    float Size;
    float Angle;
    float CurrentAngle = 0.0f;

    VehicleDummy(RwFrame* frame, std::string name, bool parent, eDummyPos type = eDummyPos::None, RwRGBA color = { 255, 255, 255, 128 });

    void ResetAngle() {
        if (CurrentAngle != 0.0f) {
            ReduceAngle(CurrentAngle);
        }
    };

    void AddAngle(float angle) {
        if (angle != 0.0f) {
            RwFrameRotate(Frame, (RwV3d*)0x008D2E18, angle, rwCOMBINEPRECONCAT);
            CurrentAngle += angle;
        }
    };

    void ReduceAngle(float angle) {
        if (angle != 0.0f) {
            RwFrameRotate(Frame, (RwV3d*)0x008D2E18, -angle, rwCOMBINEPRECONCAT);
            CurrentAngle -= angle;
        }
    };

    void SetAngle(float angle) {
        ResetAngle();
        AddAngle(angle);
    }
};
