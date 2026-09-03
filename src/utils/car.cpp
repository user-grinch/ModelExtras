#include "pch.h"
#include "car.h"
#include "enums/lightoverride.h"
#include "utils/util.h"

bool CarUtil::IsLightsForcedOn(CVehicle *pVeh)
{
    return pVeh->m_nOverrideLights == eLightOverride::ForceLightsOn;
}

bool CarUtil::IsLightsForcedOff(CVehicle *pVeh)
{
    return CVehicle::ms_forceVehicleLightsOff || pVeh->m_nOverrideLights == eLightOverride::ForceLightsOff;
}

bool CarUtil::AreHeadlightsPopUpOpen(CVehicle *pVeh)
{
    if (pVeh && pVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE)
    {
        CAutomobile *pAuto = static_cast<CAutomobile *>(pVeh);

        // Only automobiles with a moving misc_a node can have native pop-up headlights
        if (!pAuto->m_aCarNodes[CAR_MISC_A])
        {
            return true;
        }

        // For vehicles with pop-up headlights, GTA SA's CAutomobile::PreRender holds
        // m_renderLights.m_bLeftFront and m_bRightFront false while the headlight covers are opening,
        // and sets them true once fully deployed. Utility vehicles (forklifts, dozers, packers)
        // have their m_renderLights set immediately when headlights are active.
        return pAuto->m_renderLights.m_bLeftFront || pAuto->m_renderLights.m_bRightFront || pAuto->m_fPropRotate >= 0.68f;
    }
    return true;
}

bool CarUtil::CanHaveHeadlights(CVehicle *pVeh)
{
    if (!pVeh || pVeh->m_fHealth <= 0.0f)
    {
        return false;
    }

    // Only road motor vehicles can have headlights (automobiles, motorcycles, quads, monster trucks).
    // Excludes bicycles (BMX), boats, planes, helicopters, trailers, trains, etc.
    return pVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE ||
           pVeh->m_nVehicleSubClass == VEHICLE_BIKE ||
           pVeh->m_nVehicleSubClass == VEHICLE_QUAD ||
           pVeh->m_nVehicleSubClass == VEHICLE_MTRUCK;
}

bool CarUtil::AreHeadlightsActive(CVehicle *pVeh)
{
    if (!CanHaveHeadlights(pVeh))
    {
        return false;
    }

    if (IsLightsForcedOff(pVeh))
    {
        return false;
    }

    if (IsLightsForcedOn(pVeh))
    {
        return true;
    }

    if (Util::IsNightTime())
    {
        return true;
    }

    // During daytime, vehicles require an active driver to have headlights on (unless forced on above).
    // This prevents unoccupied parked cars from displaying daytime ghost headlights.
    return pVeh->bLightsOn && pVeh->m_pDriver != nullptr;
}
