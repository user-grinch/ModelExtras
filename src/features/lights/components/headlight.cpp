#include "pch.h"
#include "headlight.h"
#include "utils/util.h"
#include "utils/car.h"
#include "utils/audiomgr.h"
#include <CWeather.h>

eMaterialType HeadlightComponent::GetMatType(CRGBA matCol) {
    if (matCol == VEHCOL_HEADLIGHT_LEFT) return eMaterialType::HeadLightLeft;
    if (matCol == VEHCOL_HEADLIGHT_RIGHT) return eMaterialType::HeadLightRight;
    return eMaterialType::UnknownMaterial;
}

bool HeadlightComponent::TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string_view name, VehLightData& data) {
    if (name == "headlights" || name == "headlights2") {
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.dummyPos = eDummyPos::Front;
        c.lightType = eMaterialType::HeadLightLeft;
        c.corona.lightingType = eLightingMode::Directional;
        c.shadow.render = name != "headlights2";
        
        c.mirroredX = true;
        data.dummies[eMaterialType::HeadLightLeft].push_back(new VehicleDummy(c));
        
        c.mirroredX = false;
        c.lightType = eMaterialType::HeadLightRight;
        data.dummies[eMaterialType::HeadLightRight].push_back(new VehicleDummy(c));
        return true;
    }
    return false;
}

void HeadlightComponent::Process(CVehicle* pVeh, VehLightData& data) {
    if (pVeh->m_pDriver == FindPlayerPed()) {
        static size_t prev = 0;
        static uint32_t longLightKey = gConfig.ReadInteger("KEYS", "LongLightKey", VK_G);

        if (KeyPressed(longLightKey) && (pVeh->bLightsOn || CarUtil::IsLightsForcedOn(pVeh))) {
            size_t now = CTimer::m_snTimeInMilliseconds;
            if (now - prev > 500) {
                data.bLongLightsOn = !data.bLongLightsOn;
                prev = now;
                AudioMgr::PlaySwitchSound(pVeh);
            }
        }
    } else if (pVeh->m_nVehicleSubClass != VEHICLE_BMX && pVeh->m_nVehicleSubClass != VEHICLE_BOAT && pVeh->m_nVehicleSubClass != VEHICLE_TRAILER && !Util::IsEngineOff(pVeh)) {
        if (DistanceBetweenPoints(pVeh->GetPosition(), TheCamera.GetPosition()) < 150.0f || pVeh->GetIsOnScreen()) {
            bool isLeftFrontOk = !Util::IsLightDamaged(pVeh, eLights::LIGHT_FRONT_LEFT);
            bool isRightFrontOk = !Util::IsLightDamaged(pVeh, eLights::LIGHT_FRONT_RIGHT);
            // Render Headlights directly from Process logic for AI/Parked if visible to camera.
            // This replicates the old ProcessScriptsEvent headlight render for non-player vehicles.
            if (pVeh->bLightsOn || CarUtil::IsLightsForcedOn(pVeh) || Util::IsNightTime()) {
                std::string texName = data.bLongLightsOn ? "headlight_long" : "headlight_short";
                LightManager::RenderLight(pVeh, data, eMaterialType::HeadLightLeft, isLeftFrontOk, texName);
                LightManager::RenderLight(pVeh, data, eMaterialType::HeadLightRight, isRightFrontOk, texName);
            }
        }
    }
}

void HeadlightComponent::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {
    if (CarUtil::IsLightsForcedOff(pControlVeh)) return;

    if (pControlVeh->m_pDriver == FindPlayerPed() && (pControlVeh->bLightsOn || CarUtil::IsLightsForcedOn(pControlVeh) || Util::IsNightTime())) {
        bool leftOn = pControlVeh->m_renderLights.m_bLeftFront && !Util::IsLightDamaged(pControlVeh, eLights::LIGHT_FRONT_LEFT);
        bool rightOn = pControlVeh->m_renderLights.m_bRightFront && !Util::IsLightDamaged(pControlVeh, eLights::LIGHT_FRONT_RIGHT);
        
        std::string texName = data.bLongLightsOn ? "headlight_long" : "headlight_short";

        LightManager::RenderLight(pControlVeh, data, eMaterialType::HeadLightLeft, leftOn, texName);
        LightManager::RenderLight(pControlVeh, data, eMaterialType::HeadLightRight, rightOn, texName);

        if (pControlVeh != pTowedVeh) {
             LightManager::RenderLight(pTowedVeh, data, eMaterialType::HeadLightLeft, leftOn, texName);
             LightManager::RenderLight(pTowedVeh, data, eMaterialType::HeadLightRight, rightOn, texName);
        }
    }
}

