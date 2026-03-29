#include "pch.h"
#include "night_light.h"
#include "utils/util.h"

eMaterialType NightLightComponent::GetMatType(CRGBA matCol) {
    if (matCol == VEHCOL_NIGHTLIGHT_1 || matCol == VEHCOL_NIGHTLIGHT_2) return eMaterialType::NightLight;
    return eMaterialType::UnknownMaterial;
}

bool NightLightComponent::TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string_view name, VehLightData& data) {
    if (name.starts_with("light_n")) {
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.lightType = eMaterialType::NightLight;
        c.dummyPos = eDummyPos::Front;
        data.dummies[c.lightType].push_back(new VehicleDummy(c));
        return true;
    }
    return false;
}

void NightLightComponent::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {
    LightManager::RenderLight(pTowedVeh, data, eMaterialType::NightLight, Util::IsNightTime());
}
