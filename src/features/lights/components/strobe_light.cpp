#include "pch.h"
#include "strobe_light.h"
#include "utils/modelinfomgr.h"
#include "utils/util.h"

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

void StrobeLightComponent::Process(CVehicle* pVeh, VehLightData& data) {
    if (data.dummies.count(eMaterialType::StrobeLight) == 0) return;

    size_t timer = CTimer::m_snTimeInMilliseconds;
    for (auto* dummy : data.dummies[eMaterialType::StrobeLight]) {
        auto& c = const_cast<DummyConfig&>(dummy->GetRef());
        if (timer - c.strobe.timer > c.strobe.delay) {
            c.strobe.enabled = !c.strobe.enabled;
            c.strobe.timer = timer;
        }
    }
}

void StrobeLightComponent::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {
    if (data.dummies.count(eMaterialType::StrobeLight) == 0) return;

    for (auto* dummy : data.dummies[eMaterialType::StrobeLight]) {
        const auto& c = dummy->GetRef();
        if (c.strobe.enabled) {
            ModelInfoMgr::EnableStrobeMaterial(pTowedVeh, c.dummyIdx);
            // Strobes usually don't have shadows/coronas in the same way, but we can call RenderLight if needed
            LightManager::RenderLight(pTowedVeh, data, eMaterialType::StrobeLight, true);
        }
    }
}
