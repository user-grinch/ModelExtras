#include "pch.h"
#include "stt_light.h"
#include "utils/util.h"
#include "utils/car.h"
#include "../damage.h"
#include "defines.h"

void STTLightComponent::RegisterMaterials(std::unordered_map<uint32_t, eMaterialType>& matMap) {
    matMap[VEHCOL_STTLIGHT_LEFT.ToInt()] = eMaterialType::STTLightLeft;
    matMap[VEHCOL_STTLIGHT_RIGHT.ToInt()] = eMaterialType::STTLightRight;
}

eMaterialType STTLightComponent::GetMatType(CRGBA matCol) {
    if (matCol == VEHCOL_STTLIGHT_LEFT) return eMaterialType::STTLightLeft;
    if (matCol == VEHCOL_STTLIGHT_RIGHT) return eMaterialType::STTLightRight;
    return eMaterialType::UnknownMaterial;
}

bool STTLightComponent::TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string_view name, VehLightData& data) {
    if (auto d = Util::GetCharsAfterPrefix(name, "sttlight_", 1)) {
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.lightType = (d == "L") ? eMaterialType::STTLightLeft : eMaterialType::STTLightRight;
        c.dummyPos = eDummyPos::Rear;
        c.corona.color = c.shadow.color = {240, 0, 0, static_cast<unsigned char>(LightsConfig::Get().gGlobalCoronaIntensity)};
        c.corona.lightingType = eLightingMode::Directional;
        data.dummies[c.lightType].push_back(new VehicleDummy(c));
        return true;
    }
    return false;
}

void STTLightComponent::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {
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
        bool indicatorOn = data.bUsingGlobalIndicators && data.nIndicatorState != eIndicatorState::Off;
        bool tailOn = (Util::IsNightTime() || pControlVeh->bLightsOn || CarUtil::IsLightsForcedOn(pControlVeh) || indicatorOn) && !CarUtil::IsLightsForcedOff(pControlVeh);
        
        if (brakeOn || tailOn) {
            if (isLeftRearOk) {
                LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::STTLightLeft, true, shdwName, shdwSz, false, isLeftRearOk);
            }
            if (isRightRearOk) {
                LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::STTLightRight, true, shdwName, shdwSz, false, isRightRearOk);
            }
        }
    }
}
