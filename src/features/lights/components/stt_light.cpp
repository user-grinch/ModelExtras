#include "pch.h"
#include "stt_light.h"
#include "utils/util.h"
#include "utils/car.h"
#include "defines.h"

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
        c.corona.color = {240, 0, 0, 255};
        c.corona.lightingType = eLightingMode::Directional;
        data.dummies[c.lightType].push_back(new VehicleDummy(c));
        return true;
    }
    return false;
}

void STTLightComponent::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {
    bool isBike = CModelInfo::IsBikeModel(pControlVeh->m_nModelIndex);
    std::string shdwName = (isBike ? "taillight_bike" : "taillight");

    if (pControlVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pControlVeh->m_nVehicleSubClass == VEHICLE_MTRUCK
        || pControlVeh->m_nVehicleSubClass == VEHICLE_QUAD || pControlVeh->m_nVehicleSubClass == VEHICLE_BIKE
        || pControlVeh->m_nVehicleSubClass == VEHICLE_TRAILER) 
    {
        bool isLeftRearOk = !(Util::IsLightDamaged(pTowedVeh, eLights::LIGHT_REAR_LEFT) || Util::IsPanelDamaged(pTowedVeh, ePanels::WING_REAR_LEFT));
        bool isRightRearOk = !(Util::IsLightDamaged(pTowedVeh, eLights::LIGHT_REAR_RIGHT) || Util::IsPanelDamaged(pTowedVeh, ePanels::WING_REAR_RIGHT));

        bool brakeOn = pControlVeh->m_fBreakPedal && pControlVeh->m_pDriver;
        bool tailOn = (Util::IsNightTime() || CarUtil::IsLightsForcedOn(pControlVeh)) && !CarUtil::IsLightsForcedOff(pControlVeh);
        
        if (brakeOn) {
            LightManager::RenderLight(pTowedVeh, data, eMaterialType::STTLightLeft, isLeftRearOk, shdwName);
            LightManager::RenderLight(pTowedVeh, data, eMaterialType::STTLightRight, isRightRearOk, shdwName);
        }

        if (tailOn) {
            LightManager::RenderLight(pTowedVeh, data, eMaterialType::STTLightLeft, isLeftRearOk, shdwName);
            LightManager::RenderLight(pTowedVeh, data, eMaterialType::STTLightRight, isRightRearOk, shdwName);
        }
    }
}
