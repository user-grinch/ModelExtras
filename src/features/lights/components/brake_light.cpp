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
    if ((name.starts_with("breakl") || name.starts_with("brakel")) && (STR_FOUND(name, "_l") || STR_FOUND(name, "_r"))) {
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.dummyPos = eDummyPos::Rear;
        c.lightType = STR_FOUND(name, "_l") ? eMaterialType::BrakeLightLeft : eMaterialType::BrakeLightRight;
        c.corona.color = c.shadow.color = {240, 0, 0, static_cast<unsigned char>(LightsConfig::Get().gGlobalCoronaIntensity)};
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
        bool brakeOn = pControlVeh->m_fBreakPedal && pControlVeh->m_pDriver;
        if (brakeOn) {
            auto damage = LightDamageState::Get(pControlVeh, pTowedVeh);
            bool isLeftRearOk = damage.isRearLeftOk;
            bool isRightRearOk = damage.isRearRightOk;

            bool sttInstalled = LightManager::IsMaterialAvailable(pTowedVeh, {eMaterialType::STTLightLeft, eMaterialType::STTLightRight});
            if (!sttInstalled) {
                if (LightManager::IsMaterialAvailable(pTowedVeh, {eMaterialType::BrakeLightLeft, eMaterialType::BrakeLightRight}) || LightManager::IsDummyAvailable(data, {eMaterialType::BrakeLightLeft, eMaterialType::BrakeLightRight})) {
                    if (isLeftRearOk) {
                        LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::BrakeLightLeft, true, shdwName, shdwSz, false, isLeftRearOk);
                    }
                    if (isRightRearOk) {
                        LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::BrakeLightRight, true, shdwName, shdwSz, false, isRightRearOk);
                    }
                } else if (LightManager::IsMaterialAvailable(pTowedVeh, {eMaterialType::TailLightLeft, eMaterialType::TailLightRight}) || LightManager::IsDummyAvailable(data, {eMaterialType::TailLightLeft, eMaterialType::TailLightRight})) {
                    if (isLeftRearOk) {
                        LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::TailLightLeft, true, shdwName, shdwSz, true, isLeftRearOk);
                    }
                    if (isRightRearOk) {
                        LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::TailLightRight, true, shdwName, shdwSz, true, isRightRearOk);
                    }
                }
            }
        }
    }
}
