#pragma once
#include <CVehicle.h>

class CarUtil
{
public:
    static bool IsLightsForcedOff(CVehicle *pVeh);
    static bool IsLightsForcedOn(CVehicle *pVeh);
};