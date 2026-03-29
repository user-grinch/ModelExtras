#include "pch.h"
#include "side_light.h"
#include "utils/util.h"

eMaterialType SideLightComponent::GetMatType(CRGBA matCol) {
    if (matCol == VEHCOL_SIDELIGHT_LEFT) return eMaterialType::SideLightLeft;
    if (matCol == VEHCOL_SIDELIGHT_RIGHT) return eMaterialType::SideLightRight;
    return eMaterialType::UnknownMaterial;
}

bool SideLightComponent::TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string_view name, VehLightData& data) {
    if (auto d = Util::GetCharsAfterPrefix(name, "sidelight_", 1)) {
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.lightType = (d == "L") ? eMaterialType::SideLightLeft : eMaterialType::SideLightRight;
        c.dummyPos = (d == "L") ? eDummyPos::Left : eDummyPos::Right;
        data.dummies[c.lightType].push_back(new VehicleDummy(c));
        return true;
    }
    return false;
}

void SideLightComponent::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {
    LightManager::RenderLight(pTowedVeh, data, eMaterialType::SideLightLeft, true);
    LightManager::RenderLight(pTowedVeh, data, eMaterialType::SideLightRight, true);
}
