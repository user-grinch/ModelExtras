#include "pch.h"
#include "reverse_light.h"
#include "utils/util.h"
#include "utils/car.h"
#include "utils/render.h"
#include "../damage.h"
#include "defines.h"

void ReverseLightComponent::RegisterMaterials(std::unordered_map<uint32_t, eMaterialType>& matMap) {
    matMap[VEHCOL_REVERSELIGHT_LEFT.ToInt()] = eMaterialType::ReverseLightLeft;
    matMap[VEHCOL_REVERSELIGHT_RIGHT.ToInt()] = eMaterialType::ReverseLightRight;
}

eMaterialType ReverseLightComponent::GetMatType(CRGBA matCol) {
    if (matCol == VEHCOL_REVERSELIGHT_LEFT) return eMaterialType::ReverseLightLeft;
    if (matCol == VEHCOL_REVERSELIGHT_RIGHT) return eMaterialType::ReverseLightRight;
    return eMaterialType::UnknownMaterial;
}

bool ReverseLightComponent::TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string_view name, VehLightData& data) {
    if (name.starts_with("revl") || (name.starts_with("rev_") && !name.starts_with("revolution")) || name.starts_with("reverselight")) {
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        bool isLeft = STR_FOUND(name, "_l");
        if (!isLeft && !STR_FOUND(name, "_r")) {
            isLeft = (c.position.x < 0.0f);
        }
        c.dummyPos = eDummyPos::Rear;
        c.lightType = isLeft ? eMaterialType::ReverseLightLeft : eMaterialType::ReverseLightRight;
        c.corona.color = c.shadow.color = {255, 255, 255, static_cast<unsigned char>(LightsConfig::Get().gGlobalCoronaIntensity)};
        c.corona.lightingType = eLightingMode::Directional;
        data.dummies[c.lightType].push_back(new VehicleDummy(c));
        return true;
    }
    return false;
}

void ReverseLightComponent::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {
    bool isBike = CModelInfo::IsBikeModel(pControlVeh->m_nModelIndex);
    std::string shdwName = (isBike ? "taillight_bike" : "taillight");
    float shdwSz = 2.0f;

    if (pControlVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pControlVeh->m_nVehicleSubClass == VEHICLE_MTRUCK
        || pControlVeh->m_nVehicleSubClass == VEHICLE_QUAD || pControlVeh->m_nVehicleSubClass == VEHICLE_BIKE
        || pControlVeh->m_nVehicleSubClass == VEHICLE_TRAILER) 
    {
        bool isRevlightSupportedByModel = LightManager::IsMaterialAvailable(pTowedVeh, {eMaterialType::ReverseLightLeft, eMaterialType::ReverseLightRight});

        bool reverseLightsOn = !isBike && isRevlightSupportedByModel && pControlVeh->m_nCurrentGear == 0 && (Util::GetVehicleSpeed(pControlVeh) >= 0.001f) && pControlVeh->m_pDriver;
        if (reverseLightsOn) {
            auto damage = LightDamageState::Get(pControlVeh, pTowedVeh);
            bool isLeftRearOk = damage.isRearLeftOk;
            bool isRightRearOk = damage.isRearRightOk;

            if (isLeftRearOk) {
                LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::ReverseLightLeft, true, shdwName, shdwSz, false, isLeftRearOk);
            }
            if (isRightRearOk) {
                LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::ReverseLightRight, true, shdwName, shdwSz, false, isRightRearOk);
            }
        }
    }
}

void ReverseLightComponent::ProcessPointLights(CVehicle* pVeh, VehLightData& data) {
    bool isReversing = (pVeh->m_nCurrentGear == 0) && (Util::GetVehicleSpeed(pVeh) >= 0.001f || pVeh->m_fBreakPedal > 0.05f) && (pVeh->m_pDriver != nullptr) && (pVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pVeh->m_nVehicleSubClass == VEHICLE_MTRUCK || pVeh->m_nVehicleSubClass == VEHICLE_QUAD);

    if (isReversing) {
        for (eMaterialType type : {eMaterialType::ReverseLightLeft, eMaterialType::ReverseLightRight}) {
            if (!LightManager::IsDummyAvailable(data, type) || !data.bLightStates[type]) {
                continue;
            }

            bool isLeft = (type == eMaterialType::ReverseLightLeft);
            eLights lightEnum = isLeft ? eLights::LIGHT_REAR_LEFT : eLights::LIGHT_REAR_RIGHT;
            ePanels wingEnum = isLeft ? ePanels::WING_REAR_LEFT : ePanels::WING_REAR_RIGHT;
            if (Util::IsLightDamaged(pVeh, lightEnum) || Util::IsPanelDamaged(pVeh, wingEnum)) {
                continue;
            }

            for (auto e : data.dummies[type]) {
                e->Update();
                RenderUtil::RegisterPointLight(&e->Get(), e->Get().corona.color, 3.2f, true);
            }
        }
    }
}
