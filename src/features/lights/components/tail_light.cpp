#include "pch.h"
#include "tail_light.h"
#include "utils/util.h"
#include "utils/car.h"
#include "defines.h"

eMaterialType TailLightComponent::GetMatType(CRGBA matCol) {
    if (matCol == VEHCOL_TAILLIGHT_LEFT) return eMaterialType::TailLightLeft;
    if (matCol == VEHCOL_TAILLIGHT_RIGHT) return eMaterialType::TailLightRight;
    return eMaterialType::UnknownMaterial;
}

bool TailLightComponent::TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string_view name, VehLightData& data) {
    if (name == "taillights" || name == "taillights2") {
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.dummyPos = eDummyPos::Rear;
        c.corona.color = {250, 0, 0, 255};
        c.corona.lightingType = eLightingMode::Directional;
        c.shadow.render = name != "taillights2";
        
        c.mirroredX = true;
        c.lightType = eMaterialType::TailLightLeft;
        data.dummies[eMaterialType::TailLightLeft].push_back(new VehicleDummy(c));
        
        c.mirroredX = false;
        c.lightType = eMaterialType::TailLightRight;
        data.dummies[eMaterialType::TailLightRight].push_back(new VehicleDummy(c));
        return true;
    }
    return false;
}

void TailLightComponent::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {
    bool isBike = CModelInfo::IsBikeModel(pControlVeh->m_nModelIndex);
    std::string shdwName = (isBike ? "taillight_bike" : "taillight");

    if (pControlVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pControlVeh->m_nVehicleSubClass == VEHICLE_MTRUCK
        || pControlVeh->m_nVehicleSubClass == VEHICLE_QUAD || pControlVeh->m_nVehicleSubClass == VEHICLE_BIKE
        || pControlVeh->m_nVehicleSubClass == VEHICLE_TRAILER) 
    {
        bool isLeftRearOk = !(Util::IsLightDamaged(pTowedVeh, eLights::LIGHT_REAR_LEFT) || Util::IsPanelDamaged(pTowedVeh, ePanels::WING_REAR_LEFT));
        bool isRightRearOk = !(Util::IsLightDamaged(pTowedVeh, eLights::LIGHT_REAR_RIGHT) || Util::IsPanelDamaged(pTowedVeh, ePanels::WING_REAR_RIGHT));

        bool tailOn = (Util::IsNightTime() || CarUtil::IsLightsForcedOn(pControlVeh)) && !CarUtil::IsLightsForcedOff(pControlVeh);
        if (tailOn) {
            LightManager::RenderLight(pTowedVeh, data, eMaterialType::TailLightLeft, isLeftRearOk, shdwName);
            LightManager::RenderLight(pTowedVeh, data, eMaterialType::TailLightRight, isRightRearOk, shdwName);
        }
    }
}
