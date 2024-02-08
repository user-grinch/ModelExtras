#include "pch.h"
#include "turnlights.h"
#include <CCoronas.h>
#include <CGeneral.h>

#define TURN_ON_OFF_DELAY 500
#define MAX_RADIUS 200.0f

TurnLightsFeature TurnLights;

static CVector2D GetCarPathLinkPosition(CCarPathLinkAddress &address) {
    if (address.m_nAreaId != -1 && address.m_nCarPathLinkId != -1 && ThePaths.m_pPathNodes[address.m_nAreaId]) {
        return CVector2D(static_cast<float>(ThePaths.m_pNaviNodes[address.m_nAreaId][address.m_nCarPathLinkId].m_vecPosn.x) / 8.0f,
            static_cast<float>(ThePaths.m_pNaviNodes[address.m_nAreaId][address.m_nCarPathLinkId].m_vecPosn.y) / 8.0f);
    }
    return CVector2D(0.0f, 0.0f);
}

static void DrawTurnlight(CVehicle *vehicle, unsigned int dummyId, bool leftSide) {
    CVector posn =
        reinterpret_cast<CVehicleModelInfo *>(CModelInfo::ms_modelInfoPtrs[vehicle->m_nModelIndex])->m_pVehicleStruct->m_avDummyPos[dummyId];
    if (posn.x == 0.0f) posn.x = 0.15f;
    if (leftSide) posn.x *= -1.0f;
    CCoronas::RegisterCorona(reinterpret_cast<unsigned int>(vehicle) + 50 + dummyId + (leftSide ? 0 : 2), vehicle, 255, 128, 0, 255, posn,
        0.3f, 150.0f, CORONATYPE_SHINYSTAR, eCoronaFlareType::FLARETYPE_NONE, false, false, 0, 0.0f, false, 0.5f, 0, 50.0f, false, true);
}

static void DrawVehicleTurnlights(CVehicle *vehicle, eLightsStatus lightsStatus) {
    if (lightsStatus == eLightsStatus::Both || lightsStatus == eLightsStatus::Right) {
        DrawTurnlight(vehicle, 0, false);
        DrawTurnlight(vehicle, 1, false);
    }
    if (lightsStatus == eLightsStatus::Both || lightsStatus == eLightsStatus::Left) {
        DrawTurnlight(vehicle, 0, true);
        DrawTurnlight(vehicle, 1, true);
    }
}

static float GetZAngleForPoint(CVector2D const &point) {
    float angle = CGeneral::GetATanOfXY(point.x, point.y) * 57.295776f - 90.0f;
    while (angle < 0.0f) angle += 360.0f;
    return angle;
}

void TurnLightsFeature::Initialize() {
    Events::vehicleRenderEvent.before += [this](CVehicle *vehicle) {
        VehData &data = vehData.Get(vehicle);
        if ((vehicle->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || vehicle->m_nVehicleSubClass == VEHICLE_BIKE) &&
            (vehicle->GetVehicleAppearance() == VEHICLE_APPEARANCE_AUTOMOBILE || vehicle->GetVehicleAppearance() == VEHICLE_APPEARANCE_BIKE) &&
            vehicle->m_nVehicleFlags.bEngineOn && vehicle->m_fHealth > 0 && !vehicle->m_nVehicleFlags.bIsDrowning && !vehicle->m_pAttachedTo )
        {
            eLightsStatus &lightsStatus = data.lightsStatus;
            if (vehicle->m_pDriver) {
                CPed *playa = FindPlayerPed();
                if (playa && playa->m_pVehicle == vehicle && playa->m_nPedFlags.bInVehicle) {
                    if (KeyPressed(90)) // Z
                        lightsStatus = eLightsStatus::Left;
                    else if (KeyPressed(88)) // X
                        lightsStatus = eLightsStatus::Both;
                    else if (KeyPressed(67)) // C
                        lightsStatus = eLightsStatus::Right;
                    else if (KeyPressed(VK_SHIFT))
                        lightsStatus = eLightsStatus::Off;
                }
                else {
                    lightsStatus = eLightsStatus::Off;

                    CVector2D prevPoint = GetCarPathLinkPosition(vehicle->m_autoPilot.m_nPreviousPathNodeInfo);
                    CVector2D currPoint = GetCarPathLinkPosition(vehicle->m_autoPilot.m_nCurrentPathNodeInfo);
                    CVector2D nextPoint = GetCarPathLinkPosition(vehicle->m_autoPilot.m_nNextPathNodeInfo);

                    float angle = GetZAngleForPoint(nextPoint - currPoint) - GetZAngleForPoint(currPoint - prevPoint);
                    while (angle < 0.0f) angle += 360.0f;
                    while (angle > 360.0f) angle -= 360.0f;

                    if (angle >= 30.0f && angle < 180.0f)
                        lightsStatus = eLightsStatus::Left;
                    else if (angle <= 330.0f && angle > 180.0f)
                        lightsStatus = eLightsStatus::Right;

                    if (lightsStatus == eLightsStatus::Off) {
                        if (vehicle->m_autoPilot.m_nCurrentLane == 0 && vehicle->m_autoPilot.m_nNextLane == 1)
                            lightsStatus = eLightsStatus::Right;
                        else if (vehicle->m_autoPilot.m_nCurrentLane == 1 && vehicle->m_autoPilot.m_nNextLane == 0)
                            lightsStatus = eLightsStatus::Left;
                    }
                }
            }
            if (CTimer::m_snTimeInMilliseconds % (TURN_ON_OFF_DELAY * 2) < TURN_ON_OFF_DELAY) {
                if (DistanceBetweenPoints(TheCamera.m_vecGameCamPos, vehicle->GetPosition()) < MAX_RADIUS) {
                    DrawVehicleTurnlights(vehicle, lightsStatus);
                    if (vehicle->m_pTractor)
                        DrawVehicleTurnlights(vehicle->m_pTractor, lightsStatus);
                }
            }
        }
    };
}