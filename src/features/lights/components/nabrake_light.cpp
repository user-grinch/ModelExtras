#include "pch.h"
#include "nabrake_light.h"
#include "utils/util.h"
#include "utils/car.h"
#include "../damage.h"
#include "defines.h"

void NABrakeLightComponent::RegisterMaterials(std::unordered_map<uint32_t, eMaterialType>& matMap) {
    matMap[VEHCOL_NABRAKE_LEFT.ToInt()] = eMaterialType::NABrakeLightLeft;
    matMap[VEHCOL_NABRAKE_RIGHT.ToInt()] = eMaterialType::NABrakeLightRight;
}

eMaterialType NABrakeLightComponent::GetMatType(CRGBA matCol) {
    if (matCol == VEHCOL_NABRAKE_LEFT) return eMaterialType::NABrakeLightLeft;
    if (matCol == VEHCOL_NABRAKE_RIGHT) return eMaterialType::NABrakeLightRight;
    return eMaterialType::UnknownMaterial;
}

bool NABrakeLightComponent::TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string_view name, VehLightData& data) {
    if (auto d = Util::GetCharsAfterPrefix(name, "nabrakelight_", 1)) {
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.lightType = (d == "L") ? eMaterialType::NABrakeLightLeft : eMaterialType::NABrakeLightRight;
        c.dummyPos = eDummyPos::Rear;
        c.corona.color = c.shadow.color = {240, 0, 0, static_cast<unsigned char>(LightsConfig::Get().gGlobalCoronaIntensity)};
        c.corona.lightingType = eLightingMode::Directional;
        data.dummies[c.lightType].push_back(VehicleDummy(c));
        return true;
    }
    return false;
}

void NABrakeLightComponent::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {
    bool isBike = CModelInfo::IsBikeModel(pControlVeh->m_nModelIndex);
    std::string shdwName = (isBike ? "taillight_bike" : "taillight");
    float shdwSz = 2.0f;

    if (pControlVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pControlVeh->m_nVehicleSubClass == VEHICLE_MTRUCK
        || pControlVeh->m_nVehicleSubClass == VEHICLE_QUAD || pControlVeh->m_nVehicleSubClass == VEHICLE_BIKE
        || pControlVeh->m_nVehicleSubClass == VEHICLE_TRAILER) 
    {
        auto damage = LightDamageState::Get(pControlVeh, pTowedVeh);
        bool isLeftRearOk = damage.isRearLeftOk;
        bool isRightRearOk = damage.isRearRightOk;

        bool brakeOn = pControlVeh->m_fBreakPedal && pControlVeh->m_pDriver;
        if (brakeOn && data.nIndicatorState != eIndicatorState::BothOn) {
            if (data.nIndicatorState != eIndicatorState::LeftOn && isLeftRearOk) {
                LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::NABrakeLightLeft, true, shdwName, shdwSz, false, isLeftRearOk);
            }
            if (data.nIndicatorState != eIndicatorState::RightOn && isRightRearOk) {
                LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::NABrakeLightRight, true, shdwName, shdwSz, false, isRightRearOk);
            }
        }
    }
}
