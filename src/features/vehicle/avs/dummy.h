#pragma once
#include "materials.h"
#include "game_sa/CGeneral.h"
#include "RenderWare.h"

enum class eDummyPos {
    None,
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
};

// This enum order needs to be same as ePanels
enum class eDetachPart {
    FrontLeftWing,
    FrontRightWing,
    RearLeftWing,
    RearRightWing,
    WindScreen,
    FrontBumper,
    RearBumper,
    Unknown,
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
    eDetachPart PartType = eDetachPart::Unknown;
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

    void Update() {
        Position = { Frame->modelling.pos.x, Frame->modelling.pos.y, Frame->modelling.pos.z };
        ShdwPosition = Position;
    }
};
