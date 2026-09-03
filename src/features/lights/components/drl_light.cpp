#include "pch.h"
#include "drl_light.h"
#include "utils/util.h"
#include "utils/render.h"
#include "defines.h"

void DRLLightComponent::RegisterMaterials(std::unordered_map<uint32_t, eMaterialType>& matMap) {
    matMap[VEHCOL_ALLDAYLIGHT_1.ToInt()] = eMaterialType::AllDayLight;
    matMap[VEHCOL_ALLDAYLIGHT_2.ToInt()] = eMaterialType::AllDayLight;
    matMap[VEHCOL_DAYLIGHT_1.ToInt()] = eMaterialType::DayLight;
    matMap[VEHCOL_DAYLIGHT_2.ToInt()] = eMaterialType::DayLight;
    matMap[VEHCOL_NIGHTLIGHT_1.ToInt()] = eMaterialType::NightLight;
    matMap[VEHCOL_NIGHTLIGHT_2.ToInt()] = eMaterialType::NightLight;
}

eMaterialType DRLLightComponent::GetMatType(CRGBA matCol) {
    if (matCol == VEHCOL_ALLDAYLIGHT_1 || matCol == VEHCOL_ALLDAYLIGHT_2) return eMaterialType::AllDayLight;
    if (matCol == VEHCOL_DAYLIGHT_1 || matCol == VEHCOL_DAYLIGHT_2) return eMaterialType::DayLight;
    if (matCol == VEHCOL_NIGHTLIGHT_1 || matCol == VEHCOL_NIGHTLIGHT_2) return eMaterialType::NightLight;
    return eMaterialType::UnknownMaterial;
}

bool DRLLightComponent::TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string_view name, VehLightData& data) {
    eMaterialType type = eMaterialType::UnknownMaterial;
    if (name.starts_with("light_a")) {
        type = eMaterialType::AllDayLight;
    } else if (name.starts_with("light_d")) {
        type = eMaterialType::DayLight;
    } else if (name.starts_with("light_n")) {
        type = eMaterialType::NightLight;
    } else {
        return false;
    }

    DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
    c.lightType = type;
    c.dummyPos = eDummyPos::Front;
    c.shadow.size = 1.85f;
    c.shadow.color = {220, 220, 220, static_cast<unsigned char>(LightsConfig::Get().gGlobalShadowIntensity)};
    data.dummies[type].push_back(new VehicleDummy(c));
    return true;
}

void DRLLightComponent::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {
    bool isDriverOrForced = pControlVeh->m_pDriver != nullptr || CarUtil::IsLightsForcedOn(pControlVeh);
    if (isDriverOrForced || Util::IsNightTime()) {
        LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::AllDayLight, true, "indicator", 1.85f);
    }
    if (!Util::IsNightTime() && isDriverOrForced) {
        LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::DayLight, true, "indicator", 1.85f);
    }
    if (Util::IsNightTime()) {
        LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::NightLight, true, "indicator", 1.85f);
    }
}


void DRLLightComponent::ProcessPointLights(CVehicle* pVeh, VehLightData& data) {
    auto renderDRLPointLight = [&](eMaterialType type) {
        if (!LightManager::IsDummyAvailable(data, type) || !data.bLightStates[type]) return;
        for (auto* dummy : data.dummies[type]) {
            dummy->Update();
            RenderUtil::RegisterPointLight(&dummy->Get(), dummy->Get().corona.color, 0.85f, true);
        }
    };

    renderDRLPointLight(eMaterialType::AllDayLight);
    if (!Util::IsNightTime()) {
        renderDRLPointLight(eMaterialType::DayLight);
    }
    if (Util::IsNightTime()) {
        renderDRLPointLight(eMaterialType::NightLight);
    }
}
