#include "pch.h"
#include "brake_light.h"
#include "utils/util.h"
#include "utils/car.h"
#include "../damage.h"
#include "defines.h"

void BrakeLightComponent::RegisterMaterials(std::unordered_map<uint32_t, eMaterialType>& matMap) {
    matMap[VEHCOL_BRAKELIGHT_LEFT.ToInt()] = eMaterialType::BrakeLightLeft;
    matMap[VEHCOL_BRAKELIGHT_RIGHT.ToInt()] = eMaterialType::BrakeLightRight;
}

eMaterialType BrakeLightComponent::GetMatType(CRGBA matCol) {
    if (matCol == VEHCOL_BRAKELIGHT_LEFT) return eMaterialType::BrakeLightLeft;
    if (matCol == VEHCOL_BRAKELIGHT_RIGHT) return eMaterialType::BrakeLightRight;
    return eMaterialType::UnknownMaterial;
}

bool BrakeLightComponent::TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string_view name, VehLightData& data) {
    if ((name.starts_with("breakl") || name.starts_with("brakel") || name.starts_with("brake_") || name.starts_with("break_")) &&
        !STR_FOUND(name, "disc") && !STR_FOUND(name, "caliper")) {
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.dummyPos = eDummyPos::Rear;
        bool hasLeft = name.ends_with("_l") || STR_FOUND(name, "_l_") || STR_FOUND(name, "left");
        bool hasRight = name.ends_with("_r") || STR_FOUND(name, "_r_") || STR_FOUND(name, "right");
        if (hasLeft && !hasRight) {
            c.lightType = eMaterialType::BrakeLightLeft;
        } else if (hasRight && !hasLeft) {
            c.lightType = eMaterialType::BrakeLightRight;
        } else {
            // Side-agnostic or center brake light (e.g. breakl, breakl_m, breaklight, breaklight_m)
            c.lightType = (c.position.x < -0.05f) ? eMaterialType::BrakeLightLeft : eMaterialType::BrakeLightRight;
        }
        c.corona.size = LightsConfig::Get().gfTailLightCoronaSize;
        c.corona.color = {240, 0, 0, static_cast<unsigned char>(LightsConfig::Get().gTailLightCoronaIntensity)};
        c.shadow.color = {240, 0, 0, static_cast<unsigned char>(LightsConfig::Get().gTailLightShadowIntensity)};
        c.shadow.size = LightsConfig::Get().gfTailLightShadowSizeMul;
        c.corona.lightingType = eLightingMode::Directional;
        data.dummies[c.lightType].push_back(VehicleDummy(c));
        return true;
    }
    return false;
}

void BrakeLightComponent::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {
    bool isBike = CModelInfo::IsBikeModel(pControlVeh->m_nModelIndex);
    std::string shdwName = (isBike ? "taillight_bike" : "taillight");
    float shdwSz = 2.0f;

    if (pControlVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pControlVeh->m_nVehicleSubClass == VEHICLE_MTRUCK
        || pControlVeh->m_nVehicleSubClass == VEHICLE_QUAD || pControlVeh->m_nVehicleSubClass == VEHICLE_BIKE
        || pControlVeh->m_nVehicleSubClass == VEHICLE_TRAILER) 
    {
        bool brakeOn = LightManager::IsBraking(pControlVeh);
        if (brakeOn) {
            auto damage = LightDamageState::Get(pControlVeh);
            bool isLeftRearOk = damage.isRearLeftOk;
            bool isRightRearOk = damage.isRearRightOk;

            auto RenderBrakeForVeh = [&](CVehicle* pVeh, VehLightData& vData, bool leftOk, bool rightOk) {
                bool sttInstalled = LightManager::IsMaterialAvailable(pVeh, {eMaterialType::STTLightLeft, eMaterialType::STTLightRight});
                if (sttInstalled) return;

                if (LightManager::IsMaterialAvailable(pVeh, {eMaterialType::BrakeLightLeft, eMaterialType::BrakeLightRight}) || LightManager::IsDummyAvailable(vData, {eMaterialType::BrakeLightLeft, eMaterialType::BrakeLightRight})) {
                    LightManager::RenderLight(pVeh, vData, eMaterialType::BrakeLightLeft, true, shdwName, shdwSz, false, leftOk);
                    LightManager::RenderLight(pVeh, vData, eMaterialType::BrakeLightRight, true, shdwName, shdwSz, false, rightOk);
                } else if (LightManager::IsMaterialAvailable(pVeh, {eMaterialType::TailLightLeft, eMaterialType::TailLightRight}) || LightManager::IsDummyAvailable(vData, {eMaterialType::TailLightLeft, eMaterialType::TailLightRight})) {
                    LightManager::RenderLight(pVeh, vData, eMaterialType::TailLightLeft, true, shdwName, shdwSz, true, leftOk);
                    LightManager::RenderLight(pVeh, vData, eMaterialType::TailLightRight, true, shdwName, shdwSz, true, rightOk);
                }
            };

            RenderBrakeForVeh(pControlVeh, data, isLeftRearOk, isRightRearOk);
            if (pControlVeh != pTowedVeh) {
                auto towedDamage = LightDamageState::Get(pTowedVeh);
                RenderBrakeForVeh(pTowedVeh, LightManager::m_VehData.Get(pTowedVeh), towedDamage.isRearLeftOk, towedDamage.isRearRightOk);
            }
        }
    }
}
