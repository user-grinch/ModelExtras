#include "pch.h"
#include "spot_light.h"
#include "features/spotlights.h"
#include "defines.h"

void SpotLightComponent::RegisterMaterials(std::unordered_map<uint32_t, eMaterialType>& matMap) {
    matMap[VEHCOL_SPOTLIGHT.ToInt()] = eMaterialType::SpotLight;
}

eMaterialType SpotLightComponent::GetMatType(CRGBA matCol) {
    if (matCol == VEHCOL_SPOTLIGHT) return eMaterialType::SpotLight;
    return eMaterialType::UnknownMaterial;
}

bool SpotLightComponent::TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string_view name, VehLightData& data) {
    if (name.starts_with("spotlight_light")) {
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.lightType = eMaterialType::SpotLight;
        data.dummies[c.lightType].push_back(new VehicleDummy(c));
        return true;
    }
    return false;
}

void SpotLightComponent::Process(CVehicle* pVeh, VehLightData& data) {}

void SpotLightComponent::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {
    if (SpotLights::IsEnabled(pControlVeh)) {
        LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::SpotLight, false, "", 1.0f, false, true, true);
    }
}
