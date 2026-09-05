#pragma once
#include <CVehicle.h>
#include <CAutomobile.h>
#include <CBike.h>
#include <RenderWare.h>
#include <optional>
#include "enums/dummypos.h"

class CarUtil
{
public:
    static bool IsLightsForcedOff(CVehicle *pVeh);
    static bool IsLightsForcedOn(CVehicle *pVeh);
    static bool AreHeadlightsPopUpOpen(CVehicle *pVeh);

    static bool IsEngineOff(CVehicle *pVeh);
    static bool IsDoorDamaged(CVehicle *pVeh, eDoors door);
    static bool IsLightDamaged(CVehicle *pVeh, eLights light);
    static bool IsPanelDamaged(CVehicle *pVeh, ePanels panel);
    static bool IsFrameDamaged(CVehicle *pVeh, RwFrame *frame);
    static CVector UpdateRelativeToBoundingBox(CVehicle *pVeh, eDummyPos dummyPos, CVector center, CVector up, CVector right);

    static float GetVehiclePitch(CVehicle *pVeh);
    static bool IsVehicleDoingWheelie(CVehicle *pVeh);
    static float GetVehicleSpeed(CVehicle *pVeh);
    static float GetVehicleSpeedRealistic(CVehicle *vehicle);
};
