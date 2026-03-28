#include "pch.h"
#include "fog_light.h"
#include "utils/audiomgr.h"
#include "defines.h"

eMaterialType FogLightComponent::GetMatType(CRGBA matCol) {
    if (matCol == VEHCOL_FOGLIGHT_LEFT) return eMaterialType::FogLightLeft;
    if (matCol == VEHCOL_FOGLIGHT_RIGHT) return eMaterialType::FogLightRight;
    return eMaterialType::UnknownMaterial;
}

bool FogLightComponent::TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string& name, VehLightData& data) {
    if (name.starts_with("fogl") && (STR_FOUND(name, "_l") || STR_FOUND(name, "_r"))) {
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.dummyPos = eDummyPos::Front;
        c.lightType = STR_FOUND(name, "_l") ? eMaterialType::FogLightLeft : eMaterialType::FogLightRight;
        c.shadow.render = false;
        c.corona.lightingType = eLightingMode::Directional;
        data.dummies[c.lightType].push_back(new VehicleDummy(c));
        return true;
    }
    return false;
}

void FogLightComponent::Process(CVehicle* pVeh, VehLightData& data) {
    if (pVeh->m_pDriver == FindPlayerPed()) {
        static size_t prev = 0;
        static uint32_t fogLightKey = gConfig.ReadInteger("KEYS", "FogLightKey", VK_J);

        if (KeyPressed(fogLightKey)) {
            size_t now = CTimer::m_snTimeInMilliseconds;
            if (now - prev > 500) {
                data.bFogLightsOn = !data.bFogLightsOn;
                prev = now;
                AudioMgr::PlaySwitchSound(pVeh);
            }
        }
    }
}

void FogLightComponent::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {
    if (!data.bFogLightsOn) return;
    LightManager::RenderLight(pTowedVeh, data, eMaterialType::FogLightLeft, true, "foglight");
    LightManager::RenderLight(pTowedVeh, data, eMaterialType::FogLightRight, true, "foglight");
}
