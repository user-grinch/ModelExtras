#include "pch.h"
#include "strobe_light.h"
#include "utils/modelinfomgr.h"
#include "utils/util.h"
#include "utils/render.h"

void StrobeLightComponent::RegisterMaterials(std::unordered_map<uint32_t, eMaterialType>& matMap) {
    matMap[VEHCOL_STROBELIGHT.ToInt()] = eMaterialType::StrobeLight;
}

eMaterialType StrobeLightComponent::GetMatType(CRGBA matCol) {
    if (matCol == VEHCOL_STROBELIGHT) return eMaterialType::StrobeLight;
    return eMaterialType::UnknownMaterial;
}

bool StrobeLightComponent::TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string_view name, VehLightData& data) {
    if (auto d = Util::GetDigitsAfter(name, "strobe_light")) {
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.lightType = eMaterialType::StrobeLight;
        c.dummyPos = eDummyPos::Front;
        c.dummyIdx = d.value();
        data.dummies[c.lightType].push_back(new VehicleDummy(c));
        return true;
    }
    return false;
}

void StrobeLightComponent::Process(CVehicle* pVeh, VehLightData& data) {}

void StrobeLightComponent::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {
    LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::StrobeLight, true);
}

void StrobeLightComponent::ProcessPointLights(CVehicle* pVeh, VehLightData& data) {
    if (LightManager::IsDummyAvailable(data, eMaterialType::StrobeLight) && data.bLightStates[eMaterialType::StrobeLight]) {
        for (auto* dummy : data.dummies[eMaterialType::StrobeLight]) {
            dummy->Update();
            const auto& c = dummy->GetRef();
            if (c.strobe.enabled) {
                RenderUtil::RegisterPointLight(&dummy->Get(), c.corona.color, 6.0f, true);
            }
        }
    }
}
