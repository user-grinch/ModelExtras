#include "pch.h"
#include "rear_lights.h"
#include "utils/util.h"
#include "utils/car.h"
#include "defines.h"

eMaterialType RearLightsComponent::GetMatType(CRGBA matCol) {
    if (matCol == VEHCOL_TAILLIGHT_LEFT) return eMaterialType::TailLightLeft;
    if (matCol == VEHCOL_TAILLIGHT_RIGHT) return eMaterialType::TailLightRight;
    if (matCol == VEHCOL_REVERSELIGHT_LEFT) return eMaterialType::ReverseLightLeft;
    if (matCol == VEHCOL_REVERSELIGHT_RIGHT) return eMaterialType::ReverseLightRight;
    if (matCol == VEHCOL_BRAKELIGHT_LEFT) return eMaterialType::BrakeLightLeft;
    if (matCol == VEHCOL_BRAKELIGHT_RIGHT) return eMaterialType::BrakeLightRight;
    if (matCol == VEHCOL_STTLIGHT_LEFT) return eMaterialType::STTLightLeft;
    if (matCol == VEHCOL_STTLIGHT_RIGHT) return eMaterialType::STTLightRight;
    if (matCol == VEHCOL_NABRAKE_LEFT) return eMaterialType::NABrakeLightLeft;
    if (matCol == VEHCOL_NABRAKE_RIGHT) return eMaterialType::NABrakeLightRight;
    return eMaterialType::UnknownMaterial;
}

bool RearLightsComponent::TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string& name, VehLightData& data) {
    if (name.starts_with("rev") && (STR_FOUND(name, "_l") || STR_FOUND(name, "_r"))) {
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.dummyPos = eDummyPos::Rear;
        c.lightType = STR_FOUND(name, "_l") ? eMaterialType::ReverseLightLeft : eMaterialType::ReverseLightRight;
        c.corona.lightingType = eLightingMode::Directional;
        data.dummies[c.lightType].push_back(new VehicleDummy(c));
        return true;
    } else if (name.starts_with("breakl") && (STR_FOUND(name, "_l") || STR_FOUND(name, "_r"))) {
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.dummyPos = eDummyPos::Rear;
        c.lightType = STR_FOUND(name, "_l") ? eMaterialType::BrakeLightLeft : eMaterialType::BrakeLightRight;
        c.corona.color = {240, 0, 0, 255};
        c.corona.lightingType = eLightingMode::Directional;
        data.dummies[c.lightType].push_back(new VehicleDummy(c));
        return true;
    } else if (auto d = Util::GetCharsAfterPrefix(name, "sttlight_", 1)) {
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.lightType = (d == "L") ? eMaterialType::STTLightLeft : eMaterialType::STTLightRight;
        c.dummyPos = eDummyPos::Rear;
        c.corona.color = {240, 0, 0, 255};
        c.corona.lightingType = eLightingMode::Directional;
        data.dummies[c.lightType].push_back(new VehicleDummy(c));
        return true;
    } else if (auto d = Util::GetCharsAfterPrefix(name, "nabrakelight_", 1)) {
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.lightType = (d == "L") ? eMaterialType::NABrakeLightLeft : eMaterialType::NABrakeLightRight;
        c.dummyPos = eDummyPos::Rear;
        c.corona.color = {240, 0, 0, 255};
        c.corona.lightingType = eLightingMode::Directional;
        data.dummies[c.lightType].push_back(new VehicleDummy(c));
        return true;
    } else if (name == "taillights" || name == "taillights2") {
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

void RearLightsComponent::Process(CVehicle* pVeh, VehLightData& data) {}

void RearLightsComponent::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {
    bool isBike = CModelInfo::IsBikeModel(pControlVeh->m_nModelIndex);
    std::string shdwName = (isBike ? "taillight_bike" : "taillight");

    if (pControlVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pControlVeh->m_nVehicleSubClass == VEHICLE_MTRUCK
        || pControlVeh->m_nVehicleSubClass == VEHICLE_QUAD || pControlVeh->m_nVehicleSubClass == VEHICLE_BIKE
        || pControlVeh->m_nVehicleSubClass == VEHICLE_TRAILER) 
    {
        bool isLeftRearOk = !(Util::IsLightDamaged(pTowedVeh, eLights::LIGHT_REAR_LEFT) || Util::IsPanelDamaged(pTowedVeh, ePanels::WING_REAR_LEFT));
        bool isRightRearOk = !(Util::IsLightDamaged(pTowedVeh, eLights::LIGHT_REAR_RIGHT) || Util::IsPanelDamaged(pTowedVeh, ePanels::WING_REAR_RIGHT));

        // 1. Reverse Lights
        bool reverseOn = !isBike && pControlVeh->m_nCurrentGear == 0 && (Util::GetVehicleSpeed(pControlVeh) >= 0.001f) && pControlVeh->m_pDriver;
        if (reverseOn) {
            LightManager::RenderLight(pTowedVeh, data, eMaterialType::ReverseLightLeft, isLeftRearOk, shdwName);
            LightManager::RenderLight(pTowedVeh, data, eMaterialType::ReverseLightRight, isRightRearOk, shdwName);
        }

        // 2. Brake Lights (Standard, STT, and NA Brake)
        bool brakeOn = pControlVeh->m_fBreakPedal && pControlVeh->m_pDriver;
        if (brakeOn) {
            LightManager::RenderLight(pTowedVeh, data, eMaterialType::BrakeLightLeft, isLeftRearOk, shdwName);
            LightManager::RenderLight(pTowedVeh, data, eMaterialType::BrakeLightRight, isRightRearOk, shdwName);
            
            LightManager::RenderLight(pTowedVeh, data, eMaterialType::NABrakeLightLeft, isLeftRearOk, shdwName);
            LightManager::RenderLight(pTowedVeh, data, eMaterialType::NABrakeLightRight, isRightRearOk, shdwName);

            LightManager::RenderLight(pTowedVeh, data, eMaterialType::STTLightLeft, isLeftRearOk, shdwName);
            LightManager::RenderLight(pTowedVeh, data, eMaterialType::STTLightRight, isRightRearOk, shdwName);
        }

        // 3. Tail Lights (Standard and STT)
        bool tailOn = (Util::IsNightTime() || CarUtil::IsLightsForcedOn(pControlVeh)) && !CarUtil::IsLightsForcedOff(pControlVeh);
        if (tailOn) {
            LightManager::RenderLight(pTowedVeh, data, eMaterialType::TailLightLeft, isLeftRearOk, shdwName);
            LightManager::RenderLight(pTowedVeh, data, eMaterialType::TailLightRight, isRightRearOk, shdwName);

            LightManager::RenderLight(pTowedVeh, data, eMaterialType::STTLightLeft, isLeftRearOk, shdwName);
            LightManager::RenderLight(pTowedVeh, data, eMaterialType::STTLightRight, isRightRearOk, shdwName);
        }
    }
}

