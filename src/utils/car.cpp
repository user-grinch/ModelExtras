#include "pch.h"
#include "car.h"
#include "mathutil.h"
#include "frame.h"
#include "enums/lightoverride.h"
#include <CModelInfo.h>
#include <CVehicleModelInfo.h>
#include <CColModel.h>
#include <CBike.h>
#include <CAutomobile.h>
#include <cmath>
#include <algorithm>

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
        if (!pAuto->m_aCarNodes[CAR_MISC_A])
        {
            return true;
        }
        return pAuto->m_renderLights.m_bLeftFront || pAuto->m_renderLights.m_bRightFront || pAuto->m_fPropRotate >= 0.68f;
    }
    return true;
}

bool CarUtil::IsEngineOff(CVehicle *pVeh) {
    return !pVeh->bEngineOn || pVeh->bEngineBroken;
}

float CarUtil::GetVehicleSpeed(CVehicle *pVeh)
{
    return pVeh->m_vecMoveSpeed.Magnitude2D() * 50.0f;
}

float CarUtil::GetVehicleSpeedRealistic(CVehicle *vehicle)
{
    float wheelSpeed = 0.0;
    CVehicleModelInfo *vehicleModelInfo = (CVehicleModelInfo *)CModelInfo::GetModelInfo(vehicle->m_nModelIndex);
    if (vehicle->m_nVehicleSubClass == VEHICLE_BIKE || vehicle->m_nVehicleSubClass == VEHICLE_BMX)
    {
        CBike *bike = (CBike *)vehicle;
        wheelSpeed = ((bike->m_aWheelAngularVelocity[0] * vehicleModelInfo->m_fWheelSizeFront) +
                      (bike->m_aWheelAngularVelocity[1] * vehicleModelInfo->m_fWheelSizeRear)) /
                     2.0f;
    }
    else if (vehicle->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || vehicle->m_nVehicleSubClass == VEHICLE_MTRUCK || vehicle->m_nVehicleSubClass == VEHICLE_QUAD)
    {
        CAutomobile *automobile = (CAutomobile *)vehicle;
        wheelSpeed = ((automobile->m_fWheelSpeed[0] + automobile->m_fWheelSpeed[1] * vehicleModelInfo->m_fWheelSizeFront) +
                      (automobile->m_fWheelSpeed[2] + automobile->m_fWheelSpeed[3] * vehicleModelInfo->m_fWheelSizeRear)) /
                     4.0f;
    }
    else
    {
        return (CarUtil::GetVehicleSpeed(vehicle)) * 3.6f;
    }
    wheelSpeed /= 2.45f;
    wheelSpeed *= -186.0f;

    return wheelSpeed;
}

bool CarUtil::IsLightDamaged(CVehicle *pVeh, eLights light) {
    if (!pVeh || pVeh->m_nVehicleSubClass != VEHICLE_AUTOMOBILE) {
        return false;
    }
    CAutomobile *pAutoMobile = static_cast<CAutomobile*>(pVeh);
    if (!pAutoMobile) {
        return false;
    }

    return pAutoMobile->m_damageManager.GetLightStatus(light);
}

bool CarUtil::IsDoorDamaged(CVehicle *pVeh, eDoors door) {
    if (!pVeh || pVeh->m_nVehicleSubClass != VEHICLE_AUTOMOBILE) {
        return false;
    }
    CAutomobile *pAutoMobile = static_cast<CAutomobile*>(pVeh);
    if (!pAutoMobile) {
        return false;
    }

    return pAutoMobile->m_damageManager.GetDoorStatus(door);
}

bool CarUtil::IsPanelDamaged(CVehicle *pVeh, ePanels panel) {
    if (!pVeh || pVeh->m_nVehicleSubClass != VEHICLE_AUTOMOBILE) {
        return false;
    }
    CAutomobile *pAutoMobile = static_cast<CAutomobile*>(pVeh);
    if (!pAutoMobile) {
        return false;
    }

    return pAutoMobile->m_damageManager.GetPanelStatus(panel);
}

bool CarUtil::IsFrameDamaged(CVehicle *pVeh, RwFrame *frame) {
    if (!pVeh || !frame || pVeh->m_nVehicleSubClass != VEHICLE_AUTOMOBILE) {
        return false;
    }

    RwFrame *current = frame;
    while (current) {
        const char *name = GetFrameNodeName(current);
        if (name) {
            std::string_view sName(name);
            if (sName.starts_with("bump_front") || sName.starts_with("bump_f")) {
                return CarUtil::IsPanelDamaged(pVeh, ePanels::BUMP_FRONT);
            }
            if (sName.starts_with("bump_rear") || sName.starts_with("bump_r")) {
                return CarUtil::IsPanelDamaged(pVeh, ePanels::BUMP_REAR);
            }
            if (sName.starts_with("wing_lf")) {
                return CarUtil::IsPanelDamaged(pVeh, ePanels::WING_FRONT_LEFT);
            }
            if (sName.starts_with("wing_rf")) {
                return CarUtil::IsPanelDamaged(pVeh, ePanels::WING_FRONT_RIGHT);
            }
            if (sName.starts_with("wing_lr")) {
                return CarUtil::IsPanelDamaged(pVeh, ePanels::WING_REAR_LEFT);
            }
            if (sName.starts_with("wing_rr")) {
                return CarUtil::IsPanelDamaged(pVeh, ePanels::WING_REAR_RIGHT);
            }
            if (sName.starts_with("bonnet")) {
                return CarUtil::IsDoorDamaged(pVeh, eDoors::BONNET);
            }
            if (sName.starts_with("boot")) {
                return CarUtil::IsDoorDamaged(pVeh, eDoors::BOOT);
            }
            if (sName.starts_with("windscreen")) {
                return CarUtil::IsPanelDamaged(pVeh, ePanels::WINDSCREEN);
            }
            if (sName.starts_with("door_lf")) {
                return CarUtil::IsDoorDamaged(pVeh, eDoors::DOOR_FRONT_LEFT);
            }
            if (sName.starts_with("door_rf")) {
                return CarUtil::IsDoorDamaged(pVeh, eDoors::DOOR_FRONT_RIGHT);
            }
            if (sName.starts_with("door_lr")) {
                return CarUtil::IsDoorDamaged(pVeh, eDoors::DOOR_REAR_LEFT);
            }
            if (sName.starts_with("door_rr")) {
                return CarUtil::IsDoorDamaged(pVeh, eDoors::DOOR_REAR_RIGHT);
            }
            if (sName.starts_with("chassis") || sName == "Root") {
                break;
            }
        }
        current = RwFrameGetParent(current);
    }
    return false;
}

float CarUtil::GetVehiclePitch(CVehicle *pVeh) {
    if (!pVeh || !pVeh->m_matrix) {
        return 0.0f;
    }

    CVector forward = pVeh->m_matrix->at;
    forward.Normalize();

    float pitchRad = asinf(forward.y);
    return pitchRad * 57.2957795f;
}

bool CarUtil::IsVehicleDoingWheelie(CVehicle *pVeh) {
    return pVeh && pVeh->m_nVehicleSubClass == VEHICLE_BIKE && CarUtil::GetVehiclePitch(pVeh) > 45.0f;
}

static float mx1, my1, mz1;
static float mx2, my2, mz2;

CVector CarUtil::UpdateRelativeToBoundingBox(CVehicle *pVeh, eDummyPos dummyPos, CVector shdwPos, CVector up, CVector right) {
    CVehicleModelInfo* pInfo = (CVehicleModelInfo*)CModelInfo::GetModelInfo(pVeh->m_nModelIndex);
    CVector min = pInfo->m_pColModel->m_boundBox.m_vecMin + CVector(mx1, my1, mz1);
    CVector max = pInfo->m_pColModel->m_boundBox.m_vecMax + CVector(mx2, my2, mz2);

    shdwPos += (min + max) * 0.5f;
    CVector corner1 = shdwPos + up + right;
    CVector corner2 = shdwPos + up - right;
    CVector corner3 = shdwPos - up + right;
    CVector corner4 = shdwPos - up - right;

    CVector minVec;
    minVec.x = std::min({ corner1.x, corner2.x, corner3.x, corner4.x });
    minVec.y = std::min({ corner1.y, corner2.y, corner3.y, corner4.y });
    minVec.z = std::min({ corner1.z, corner2.z, corner3.z, corner4.z });

    CVector maxVec;
    maxVec.x = std::max({ corner1.x, corner2.x, corner3.x, corner4.x });
    maxVec.y = std::max({ corner1.y, corner2.y, corner3.y, corner4.y });
    maxVec.z = std::max({ corner1.z, corner2.z, corner3.z, corner4.z });

    if (dummyPos == eDummyPos::Front) {
        minVec.y = std::max(minVec.y, max.y);
    }
    else if (dummyPos == eDummyPos::Rear) {
        maxVec.y = std::min(maxVec.y, min.y);
    } 

    shdwPos = (maxVec + minVec) * 0.5f;
    shdwPos -= (min + max) * 0.5f;
    return shdwPos;
}
