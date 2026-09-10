#include "pch.h"
#include "side_light.h"
#include "utils/util.h"
#include "utils/car.h"
#include "utils/render.h"
#include "../damage.h"
#include "defines.h"

void SideLightComponent::RegisterMaterials(std::unordered_map<uint32_t, eMaterialType>& matMap) {
    matMap[VEHCOL_SIDELIGHT_LEFT.ToInt()] = eMaterialType::SideLightLeft;
    matMap[VEHCOL_SIDELIGHT_RIGHT.ToInt()] = eMaterialType::SideLightRight;
}

eMaterialType SideLightComponent::GetMatType(CRGBA matCol) {
    if (matCol == VEHCOL_SIDELIGHT_LEFT) return eMaterialType::SideLightLeft;
    if (matCol == VEHCOL_SIDELIGHT_RIGHT) return eMaterialType::SideLightRight;
    return eMaterialType::UnknownMaterial;
}

bool SideLightComponent::TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string_view name, VehLightData& data) {
    if (auto d = Util::GetCharsAfterPrefix(name, "sidelight_", 1)) {
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        if (d == "L") {
            c.lightType = eMaterialType::SideLightLeft;
            c.dummyPos = eDummyPos::Left;
        } else {
            c.lightType = eMaterialType::SideLightRight;
            c.dummyPos = eDummyPos::Right;
        }
        data.dummies[c.lightType].push_back(VehicleDummy(c));
        return true;
    }
    return false;
}

void SideLightComponent::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {
    auto damage = LightDamageState::Get(pControlVeh, pTowedVeh);
    bool isLeftMiddleOk = damage.isMiddleLeftOk;
    bool isRightMiddleOk = damage.isMiddleRightOk;
    LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::SideLightLeft, true, "indicator", 1.85f, false, isLeftMiddleOk);
    LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::SideLightRight, true, "indicator", 1.85f, false, isRightMiddleOk);
}

void SideLightComponent::ProcessPointLights(CVehicle* pVeh, VehLightData& data) {
    bool isHeadlightsOn = (pVeh->bLightsOn || CarUtil::IsLightsForcedOn(pVeh) || (Util::IsNightTime() && !Util::IsEngineOff(pVeh))) && !CarUtil::IsLightsForcedOff(pVeh);

    if (isHeadlightsOn) {
        for (eMaterialType type : {eMaterialType::SideLightLeft, eMaterialType::SideLightRight}) {
            if (!LightManager::IsDummyAvailable(data, type) || !data.bLightStates[type]) continue;

            bool isLeft = (type == eMaterialType::SideLightLeft);
            ePanels wingEnum = isLeft ? ePanels::WING_FRONT_LEFT : ePanels::WING_FRONT_RIGHT;
            ePanels rearWingEnum = isLeft ? ePanels::WING_REAR_LEFT : ePanels::WING_REAR_RIGHT;
            if (Util::IsPanelDamaged(pVeh, wingEnum) || Util::IsPanelDamaged(pVeh, rearWingEnum)) {
                continue;
            }

            for (auto& e : data.dummies[type]) {
                e->Update();
                RenderUtil::RegisterPointLight(&e->Get(), e->Get().corona.color, 1.5f, true);
            }
        }
    }
}
