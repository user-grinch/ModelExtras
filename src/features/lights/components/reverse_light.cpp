#include "pch.h"
#include "reverse_light.h"
#include "utils/util.h"
#include "utils/car.h"
#include "defines.h"

eMaterialType ReverseLightComponent::GetMatType(CRGBA matCol) {
    if (matCol == VEHCOL_REVERSELIGHT_LEFT) return eMaterialType::ReverseLightLeft;
    if (matCol == VEHCOL_REVERSELIGHT_RIGHT) return eMaterialType::ReverseLightRight;
    return eMaterialType::UnknownMaterial;
}

bool ReverseLightComponent::TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string_view name, VehLightData& data) {
    if (name.starts_with("rev") && (STR_FOUND(name, "_l") || STR_FOUND(name, "_r"))) {
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.dummyPos = eDummyPos::Rear;
        c.lightType = STR_FOUND(name, "_l") ? eMaterialType::ReverseLightLeft : eMaterialType::ReverseLightRight;
        c.corona.lightingType = eLightingMode::Directional;
        data.dummies[c.lightType].push_back(new VehicleDummy(c));
        return true;
    }
    return false;
}

void ReverseLightComponent::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {
    bool isBike = CModelInfo::IsBikeModel(pControlVeh->m_nModelIndex);
    std::string shdwName = (isBike ? "taillight_bike" : "taillight");

    if (pControlVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pControlVeh->m_nVehicleSubClass == VEHICLE_MTRUCK
        || pControlVeh->m_nVehicleSubClass == VEHICLE_QUAD || pControlVeh->m_nVehicleSubClass == VEHICLE_BIKE
        || pControlVeh->m_nVehicleSubClass == VEHICLE_TRAILER) 
    {
        bool isLeftRearOk = !(Util::IsLightDamaged(pTowedVeh, eLights::LIGHT_REAR_LEFT) || Util::IsPanelDamaged(pTowedVeh, ePanels::WING_REAR_LEFT));
        bool isRightRearOk = !(Util::IsLightDamaged(pTowedVeh, eLights::LIGHT_REAR_RIGHT) || Util::IsPanelDamaged(pTowedVeh, ePanels::WING_REAR_RIGHT));

        bool reverseOn = !isBike && pControlVeh->m_nCurrentGear == 0 && (Util::GetVehicleSpeed(pControlVeh) >= 0.001f) && pControlVeh->m_pDriver;
        if (reverseOn) {
            LightManager::RenderLight(pTowedVeh, data, eMaterialType::ReverseLightLeft, isLeftRearOk, shdwName);
            LightManager::RenderLight(pTowedVeh, data, eMaterialType::ReverseLightRight, isRightRearOk, shdwName);
        }
    }
}
