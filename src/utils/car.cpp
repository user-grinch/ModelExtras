#include "pch.h"
#include "car.h"
#include "enums/lightoverride.h"

bool CarUtil::IsLightsForcedOn(CVehicle *pVeh)
{
    return pVeh->m_nOverrideLights == eLightOverride::ForceLightsOn;
}

bool CarUtil::IsLightsForcedOff(CVehicle *pVeh)
{
    return CVehicle::ms_forceVehicleLightsOff || pVeh->m_nOverrideLights == eLightOverride::ForceLightsOff;
}