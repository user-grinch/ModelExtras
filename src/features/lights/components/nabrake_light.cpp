#include "pch.h"
#include "nabrake_light.h"
#include "utils/util.h"
#include "utils/car.h"
#include "defines.h"

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
        c.corona.color = {240, 0, 0, 255};
        c.corona.lightingType = eLightingMode::Directional;
        data.dummies[c.lightType].push_back(new VehicleDummy(c));
        return true;
    }
    return false;
}

void NABrakeLightComponent::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {
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
            LightManager::RenderLight(pTowedVeh, data, eMaterialType::NABrakeLightLeft, isLeftRearOk, shdwName);
            LightManager::RenderLight(pTowedVeh, data, eMaterialType::NABrakeLightRight, isRightRearOk, shdwName);
        }
    }
}
