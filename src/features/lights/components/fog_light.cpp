#include "pch.h"
#include "fog_light.h"
#include "utils/audiomgr.h"
#include "utils/render.h"
#include "defines.h"

void FogLightComponent::RegisterMaterials(std::unordered_map<uint32_t, eMaterialType>& matMap) {
    matMap[VEHCOL_FOGLIGHT_LEFT.ToInt()] = eMaterialType::FogLightLeft;
    matMap[VEHCOL_FOGLIGHT_RIGHT.ToInt()] = eMaterialType::FogLightRight;
}

eMaterialType FogLightComponent::GetMatType(CRGBA matCol) {
    if (matCol == VEHCOL_FOGLIGHT_LEFT) return eMaterialType::FogLightLeft;
    if (matCol == VEHCOL_FOGLIGHT_RIGHT) return eMaterialType::FogLightRight;
    return eMaterialType::UnknownMaterial;
}

bool FogLightComponent::TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string_view name, VehLightData& data) {
    if ((name.starts_with("fogl") || name.starts_with("fog_")) && (STR_FOUND(name, "_l") || STR_FOUND(name, "_r"))) {
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.dummyPos = eDummyPos::Front;
        bool isLeft = STR_FOUND(name, "_l") || !STR_FOUND(name, "_r");
        c.lightType = isLeft ? eMaterialType::FogLightLeft : eMaterialType::FogLightRight;
        c.shadow.render = false;
        c.corona.color = c.shadow.color = {255, 255, 255, static_cast<unsigned char>(LightsConfig::Get().gGlobalCoronaIntensity)};
        c.corona.lightingType = eLightingMode::NonDirectional;
        data.dummies[c.lightType].push_back(new VehicleDummy(c));
        return true;
    }
    return false;
}

void FogLightComponent::Process(CVehicle* pVeh, VehLightData& data) {
    if (pVeh->IsDriver(FindPlayerPed()) && !Util::IsEngineOff(pVeh)) {
        static size_t prev = 0;
        bool isHeadlightsActive = (pVeh->bLightsOn || CarUtil::IsLightsForcedOn(pVeh) || Util::IsNightTime()) && !CarUtil::IsLightsForcedOff(pVeh);
        bool canToggleFogLight = !LightsConfig::Get().bFoglightTiedToHeadlight || isHeadlightsActive;

        if (Util::IsKeyPressed(LightsConfig::Get().nFogLightKey) && LightManager::IsMaterialAvailable(pVeh, {eMaterialType::FogLightLeft, eMaterialType::FogLightRight}) && canToggleFogLight) {
            size_t now = CTimer::m_snTimeInMilliseconds;
            if (now - prev > 500.0f) {
                data.bFogLightsOn = !data.bFogLightsOn;
                prev = now;
                AudioMgr::PlaySwitchSound(pVeh);
            }
        }
    }
}

void FogLightComponent::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {
    bool isHeadlightsActive = (pControlVeh->bLightsOn || CarUtil::IsLightsForcedOn(pControlVeh) || Util::IsNightTime()) && !CarUtil::IsLightsForcedOff(pControlVeh);
    bool shouldRenderFog = !LightsConfig::Get().bFoglightTiedToHeadlight || isHeadlightsActive;

    if (!data.bFogLightsOn || !shouldRenderFog) return;
    bool isFogOk = !Util::IsPanelDamaged(pControlVeh, ePanels::BUMP_FRONT);
    LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::FogLightLeft, true, "foglight", 3.0f, false, isFogOk);
    LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::FogLightRight, true, "foglight", 3.0f, false, isFogOk);
}

void FogLightComponent::ProcessPointLights(CVehicle* pVeh, VehLightData& data) {
    bool isHeadlightsOn = (pVeh->bLightsOn || CarUtil::IsLightsForcedOn(pVeh) || (Util::IsNightTime() && !Util::IsEngineOff(pVeh)) || (pVeh->m_nVehicleSubClass == VEHICLE_BIKE && !Util::IsEngineOff(pVeh))) && !CarUtil::IsLightsForcedOff(pVeh);
    bool shouldRenderFog = !LightsConfig::Get().bFoglightTiedToHeadlight || isHeadlightsOn;

    if (data.bFogLightsOn && shouldRenderFog) {
        for (eMaterialType type : {eMaterialType::FogLightLeft, eMaterialType::FogLightRight}) {
            if (!LightManager::IsDummyAvailable(data, type) || !data.bLightStates[type]) continue;

            bool isLeft = (type == eMaterialType::FogLightLeft);
            eLights lightEnum = isLeft ? eLights::LIGHT_FRONT_LEFT : eLights::LIGHT_FRONT_RIGHT;
            ePanels wingEnum = isLeft ? ePanels::WING_FRONT_LEFT : ePanels::WING_FRONT_RIGHT;
            if (Util::IsLightDamaged(pVeh, lightEnum) || Util::IsPanelDamaged(pVeh, wingEnum) || Util::IsPanelDamaged(pVeh, ePanels::BUMP_FRONT)) {
                continue;
            }

            for (auto e : data.dummies[type]) {
                e->Update();
                RenderUtil::RegisterPointLight(&e->Get(), e->Get().corona.color, 8.5f, true);
            }
        }
    }
}
