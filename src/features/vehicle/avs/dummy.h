#pragma once
#include "materials.h"
#include "game_sa/CGeneral.h"
#include "RenderWare.h"
#include "enums/parenttype.h"
#include "enums/dummypos.h"
#include "enums/lighttype.h"

class VehicleDummy
{
public:
    RwFrame *Frame;
    CRGBA Color = {255, 255, 255, 100};
    CVector Position;
    float coronaSize = 0.35f;
    float Angle = 0.0f;
    float CurrentAngle = 0.0f;

    size_t DummyIdx = 0;
    eDummyPos DummyType = eDummyPos::None;
    eLightType LightType = eLightType::NonDirectional;
    eParentType PartType = eParentType::Unknown;

    CVector ShdwPosition;
    CVector2D shdwOffSet = {0.0f, 0.0f};
    CVector2D shdowSize = {1.0f, 1.0f};

    size_t strobeLightOn = false;
    size_t strobeLightTimer = 0;
    size_t strobeDelay = 1000;

    VehicleDummy(CVehicle *pVeh, RwFrame *frame, std::string name, eDummyPos type = eDummyPos::None, CRGBA color = {255, 255, 255, 128}, size_t dummyIdx = 0);

    void ResetAngle()
    {
        if (CurrentAngle != 0.0f)
        {
            ReduceAngle(CurrentAngle);
        }
    };

    void AddAngle(float angle)
    {
        if (angle != 0.0f)
        {
            RwFrameRotate(Frame, (RwV3d *)0x008D2E18, angle, rwCOMBINEPRECONCAT);
            CurrentAngle += angle;
        }
    };

    void ReduceAngle(float angle)
    {
        if (angle != 0.0f)
        {
            RwFrameRotate(Frame, (RwV3d *)0x008D2E18, -angle, rwCOMBINEPRECONCAT);
            CurrentAngle -= angle;
        }
    };

    void SetAngle(float angle)
    {
        ResetAngle();
        AddAngle(angle);
    }

    void Update(CVehicle *pVeh);
};
