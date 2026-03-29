#include "pch.h"
#include "spot_light.h"
#include "features/spotlights.h"

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
        LightManager::RenderLight(pControlVeh, data, eMaterialType::SpotLight, true);
        if (pControlVeh != pTowedVeh) {
            LightManager::RenderLight(pTowedVeh, data, eMaterialType::SpotLight, true);
        }
    }
}
