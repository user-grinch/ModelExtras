#pragma once
#include <CVehicle.h>

class CarUtil
{
public:
    static bool IsLightsForcedOff(CVehicle *pVeh);
    static bool IsLightsForcedOn(CVehicle *pVeh);
    static bool AreHeadlightsPopUpOpen(CVehicle *pVeh);
    static bool CanHaveHeadlights(CVehicle *pVeh);
    static bool AreHeadlightsActive(CVehicle *pVeh);
};
