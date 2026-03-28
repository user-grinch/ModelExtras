#include "pch.h"
#include "allday_light.h"
#include "utils/util.h"

eMaterialType AllDayLightComponent::GetMatType(CRGBA matCol) {
    if (matCol == VEHCOL_ALLDAYLIGHT_1 || matCol == VEHCOL_ALLDAYLIGHT_2) return eMaterialType::AllDayLight;
    return eMaterialType::UnknownMaterial;
}

bool AllDayLightComponent::TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string& name, VehLightData& data) {
    if (name.starts_with("light_a")) {
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.lightType = eMaterialType::AllDayLight;
        c.dummyPos = eDummyPos::Front;
        data.dummies[c.lightType].push_back(new VehicleDummy(c));
        return true;
    }
    return false;
}

void AllDayLightComponent::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {
    LightManager::RenderLight(pTowedVeh, data, eMaterialType::AllDayLight, true);
}
