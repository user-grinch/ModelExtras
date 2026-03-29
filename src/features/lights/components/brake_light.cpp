#include "pch.h"
#include "brake_light.h"
#include "utils/util.h"
#include "utils/car.h"
#include "defines.h"

eMaterialType BrakeLightComponent::GetMatType(CRGBA matCol) {
    if (matCol == VEHCOL_BRAKELIGHT_LEFT) return eMaterialType::BrakeLightLeft;
    if (matCol == VEHCOL_BRAKELIGHT_RIGHT) return eMaterialType::BrakeLightRight;
    return eMaterialType::UnknownMaterial;
}

bool BrakeLightComponent::TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string_view name, VehLightData& data) {
    if (name.starts_with("breakl") && (STR_FOUND(name, "_l") || STR_FOUND(name, "_r"))) {
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.dummyPos = eDummyPos::Rear;
        c.lightType = STR_FOUND(name, "_l") ? eMaterialType::BrakeLightLeft : eMaterialType::BrakeLightRight;
        c.corona.color = {240, 0, 0, 255};
        c.corona.lightingType = eLightingMode::Directional;
        data.dummies[c.lightType].push_back(new VehicleDummy(c));
        return true;
    }
    return false;
}

void BrakeLightComponent::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {
    bool isBike = CModelInfo::IsBikeModel(pControlVeh->m_nModelIndex);
    std::string shdwName = (isBike ? "taillight_bike" : "taillight");

    if (pControlVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pControlVeh->m_nVehicleSubClass == VEHICLE_MTRUCK
        || pControlVeh->m_nVehicleSubClass == VEHICLE_QUAD || pControlVeh->m_nVehicleSubClass == VEHICLE_BIKE
        || pControlVeh->m_nVehicleSubClass == VEHICLE_TRAILER) 
    {
        bool isLeftRearOk = !(Util::IsLightDamaged(pTowedVeh, eLights::LIGHT_REAR_LEFT) || Util::IsPanelDamaged(pTowedVeh, ePanels::WING_REAR_LEFT));
        bool isRightRearOk = !(Util::IsLightDamaged(pTowedVeh, eLights::LIGHT_REAR_RIGHT) || Util::IsPanelDamaged(pTowedVeh, ePanels::WING_REAR_RIGHT));

        bool brakeOn = pControlVeh->m_fBreakPedal && pControlVeh->m_pDriver;
        if (brakeOn) {
            LightManager::RenderLight(pTowedVeh, data, eMaterialType::BrakeLightLeft, isLeftRearOk, shdwName);
            LightManager::RenderLight(pTowedVeh, data, eMaterialType::BrakeLightRight, isRightRearOk, shdwName);
        }
    }
}
