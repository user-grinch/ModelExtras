#include "pch.h"
#include "auxiliary_light.h"
#include "utils/util.h"

eMaterialType AuxiliaryLightComponent::GetMatType(CRGBA matCol) {
    if (matCol == VEHCOL_ALLDAYLIGHT_1 || matCol == VEHCOL_ALLDAYLIGHT_2) return eMaterialType::AllDayLight;
    if (matCol == VEHCOL_DAYLIGHT_1 || matCol == VEHCOL_DAYLIGHT_2) return eMaterialType::DayLight;
    if (matCol == VEHCOL_NIGHTLIGHT_1 || matCol == VEHCOL_NIGHTLIGHT_2) return eMaterialType::NightLight;
    if (matCol == VEHCOL_SIDELIGHT_LEFT) return eMaterialType::SideLightLeft;
    if (matCol == VEHCOL_SIDELIGHT_RIGHT) return eMaterialType::SideLightRight;
    return eMaterialType::UnknownMaterial;
}

bool AuxiliaryLightComponent::TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string& name, VehLightData& data) {
    if (name.starts_with("light_d")) {
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.lightType = eMaterialType::DayLight;
        c.dummyPos = eDummyPos::Front;
        data.dummies[c.lightType].push_back(new VehicleDummy(c));
        return true;
    } else if (name.starts_with("light_n")) {
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.lightType = eMaterialType::NightLight;
        c.dummyPos = eDummyPos::Front;
        data.dummies[c.lightType].push_back(new VehicleDummy(c));
        return true;
    } else if (name.starts_with("light_a")) {
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.lightType = eMaterialType::AllDayLight;
        c.dummyPos = eDummyPos::Front;
        data.dummies[c.lightType].push_back(new VehicleDummy(c));
        return true;
    } else if (auto d = Util::GetCharsAfterPrefix(name, "sidelight_", 1)) {
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.lightType = (d == "L") ? eMaterialType::SideLightLeft : eMaterialType::SideLightRight;
        c.dummyPos = (d == "L") ? eDummyPos::Left : eDummyPos::Right;
        data.dummies[c.lightType].push_back(new VehicleDummy(c));
        return true;
    }
    return false;
}

void AuxiliaryLightComponent::Process(CVehicle* pVeh, VehLightData& data) {}

void AuxiliaryLightComponent::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {
    LightManager::RenderLight(pTowedVeh, data, eMaterialType::DayLight, !Util::IsNightTime());
    LightManager::RenderLight(pTowedVeh, data, eMaterialType::NightLight, Util::IsNightTime());
    LightManager::RenderLight(pTowedVeh, data, eMaterialType::AllDayLight, true);
    LightManager::RenderLight(pTowedVeh, data, eMaterialType::SideLightLeft, true);
    LightManager::RenderLight(pTowedVeh, data, eMaterialType::SideLightRight, true);
}
