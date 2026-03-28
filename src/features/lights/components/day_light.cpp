#include "pch.h"
#include "day_light.h"
#include "utils/util.h"

eMaterialType DayLightComponent::GetMatType(CRGBA matCol) {
    if (matCol == VEHCOL_DAYLIGHT_1 || matCol == VEHCOL_DAYLIGHT_2) return eMaterialType::DayLight;
    return eMaterialType::UnknownMaterial;
}

bool DayLightComponent::TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string& name, VehLightData& data) {
    if (name.starts_with("light_d")) {
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.lightType = eMaterialType::DayLight;
        c.dummyPos = eDummyPos::Front;
        data.dummies[c.lightType].push_back(new VehicleDummy(c));
        return true;
    }
    return false;
}

void DayLightComponent::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {
    LightManager::RenderLight(pTowedVeh, data, eMaterialType::DayLight, !Util::IsNightTime());
}
