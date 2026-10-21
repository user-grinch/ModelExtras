#include "pch.h"
#include "headlight.h"
#include "utils/util.h"
#include "utils/car.h"
#include "utils/audiomgr.h"
#include "utils/render.h"
#include "../damage.h"
#include "defines.h"
#include <CWeather.h>

extern bool gbProperShadersDetected;

void HeadlightComponent::RegisterMaterials(std::unordered_map<uint32_t, eMaterialType>& matMap) {
    matMap[VEHCOL_HEADLIGHT_LEFT.ToInt()] = eMaterialType::HeadLightLeft;
    matMap[VEHCOL_HEADLIGHT_RIGHT.ToInt()] = eMaterialType::HeadLightRight;
    matMap[VEHCOL_HIGHBEAM_LEFT.ToInt()] = eMaterialType::HighBeamLeft;
    matMap[VEHCOL_HIGHBEAM_RIGHT.ToInt()] = eMaterialType::HighBeamRight;
}

eMaterialType HeadlightComponent::GetMatType(CRGBA matCol) {
    if (matCol == VEHCOL_HEADLIGHT_LEFT) return eMaterialType::HeadLightLeft;
    if (matCol == VEHCOL_HEADLIGHT_RIGHT) return eMaterialType::HeadLightRight;
    if (matCol == VEHCOL_HIGHBEAM_LEFT) return eMaterialType::HighBeamLeft;
    if (matCol == VEHCOL_HIGHBEAM_RIGHT) return eMaterialType::HighBeamRight;
    return eMaterialType::UnknownMaterial;
}

static bool CanVehicleHaveHeadlights(CVehicle* pVeh) {
    if (!pVeh) return false;
    int model = pVeh->m_nModelIndex;
    if (CModelInfo::IsBmxModel(model) || CModelInfo::IsBoatModel(model) || CModelInfo::IsTrailerModel(model) || CModelInfo::IsHeliModel(model) || CModelInfo::IsPlaneModel(model)) {
        return false;
    }
    if (pVeh->m_nVehicleSubClass == VEHICLE_BMX || pVeh->m_nVehicleSubClass == VEHICLE_BOAT || pVeh->m_nVehicleSubClass == VEHICLE_TRAILER || pVeh->m_nVehicleSubClass == VEHICLE_HELI || pVeh->m_nVehicleSubClass == VEHICLE_PLANE) {
        return false;
    }
    return true;
}

bool HeadlightComponent::AreHeadlightsOpen(CVehicle* pVeh, const VehLightData& data) {
    if (!pVeh || pVeh->m_nVehicleSubClass != VEHICLE_AUTOMOBILE) {
        return true;
    }

    CAutomobile* pAuto = static_cast<CAutomobile*>(pVeh);
    bool isZR350 = (pVeh->m_nModelIndex == MODEL_ZR350 && pAuto->m_aCarNodes[CAR_MISC_A] != nullptr);
    if (isZR350) {
        return pAuto->m_fPropRotate >= 0.65f;
    }

    if (data.bHasVehFuncsPopUp) {
        if (data.nHeadlightsTurnedOnTime == 0 || (CTimer::m_snTimeInMilliseconds - data.nHeadlightsTurnedOnTime < 800)) {
            return false;
        }
        return true;
    }

    return true;
}

bool HeadlightComponent::TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string_view name, VehLightData& data) {
    if (!CanVehicleHaveHeadlights(pVeh)) return false;
    if (name == "headlights" || name == "headlights2") {
        if (pFrame && !rwLinkListEmpty(&pFrame->objectList)) {
            return false;
        }
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.dummyPos = eDummyPos::Front;
        c.corona.size = LightsConfig::Get().gfHeadLightCoronaSize;
        c.corona.color = {250, 250, 250, static_cast<unsigned char>(LightsConfig::Get().gHeadLightCoronaIntensity)};
        c.shadow.color = {250, 250, 250, static_cast<unsigned char>(LightsConfig::Get().gHeadLightShadowIntensity)};
        c.shadow.size = LightsConfig::Get().gfHeadLightShadowSize;
        c.corona.lightingType = eLightingMode::Directional;
        c.shadow.render = name != "headlights2";
        
        bool isBike = (pVeh->m_nVehicleSubClass == VEHICLE_BIKE);

        if (isBike) {
            c.mirroredX = false;
            c.lightType = eMaterialType::HeadLightLeft;
            data.dummies[c.lightType].push_back(VehicleDummy(c));
        } else {
            bool dummyIsLeft = (c.frame->modelling.pos.x < 0.0f);
            c.mirroredX = !dummyIsLeft;
            c.lightType = eMaterialType::HeadLightLeft;
            data.dummies[eMaterialType::HeadLightLeft].push_back(VehicleDummy(c));

            c.mirroredX = dummyIsLeft;
            c.lightType = eMaterialType::HeadLightRight;
            data.dummies[eMaterialType::HeadLightRight].push_back(VehicleDummy(c));
        }
        return true;
    }

    if (name == "headlights_l" || name == "headlights_r") {
        if (pFrame && !rwLinkListEmpty(&pFrame->objectList)) {
            return false;
        }
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.dummyPos = eDummyPos::Front;
        bool isLeft = (name == "headlights_l");
        c.lightType = isLeft ? eMaterialType::HeadLightLeft : eMaterialType::HeadLightRight;
        c.corona.size = LightsConfig::Get().gfHeadLightCoronaSize;
        c.corona.color = {250, 250, 250, static_cast<unsigned char>(LightsConfig::Get().gHeadLightCoronaIntensity)};
        c.shadow.color = {250, 250, 250, static_cast<unsigned char>(LightsConfig::Get().gHeadLightShadowIntensity)};
        c.shadow.size = LightsConfig::Get().gfHeadLightShadowSize;
        c.corona.lightingType = eLightingMode::Directional;
        c.shadow.render = true;
        c.mirroredX = false;
        
        data.dummies[c.lightType].push_back(VehicleDummy(c));
        return true;
    }

    if (name == "highbeam_l" || name == "highbeam_r" || name == "highbeams_l" || name == "highbeams_r") {
        if (pFrame && !rwLinkListEmpty(&pFrame->objectList)) {
            return false;
        }
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.dummyPos = eDummyPos::Front;
        bool isLeft = (name == "highbeam_l" || name == "highbeams_l");
        c.lightType = isLeft ? eMaterialType::HighBeamLeft : eMaterialType::HighBeamRight;
        c.corona.size = LightsConfig::Get().gfHeadLightCoronaSize;
        c.corona.color = {250, 250, 250, static_cast<unsigned char>(LightsConfig::Get().gHeadLightCoronaIntensity)};
        c.shadow.color = {250, 250, 250, static_cast<unsigned char>(LightsConfig::Get().gHeadLightShadowIntensity)};
        c.shadow.size = LightsConfig::Get().gfHeadLightShadowSize;
        c.corona.lightingType = eLightingMode::Directional;
        c.shadow.render = false;
        c.mirroredX = false;
        
        data.dummies[c.lightType].push_back(VehicleDummy(c));
        return true;
    }

    if (name == "highbeam" || name == "highbeams") {
        if (pFrame && !rwLinkListEmpty(&pFrame->objectList)) {
            return false;
        }
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.dummyPos = eDummyPos::Front;
        c.corona.size = LightsConfig::Get().gfHeadLightCoronaSize;
        c.corona.color = {250, 250, 250, static_cast<unsigned char>(LightsConfig::Get().gHeadLightCoronaIntensity)};
        c.shadow.color = {250, 250, 250, static_cast<unsigned char>(LightsConfig::Get().gHeadLightShadowIntensity)};
        c.shadow.size = LightsConfig::Get().gfHeadLightShadowSize;
        c.corona.lightingType = eLightingMode::Directional;
        c.shadow.render = false;
        
        bool isBike = (pVeh->m_nVehicleSubClass == VEHICLE_BIKE);

        if (isBike) {
            c.mirroredX = false;
            c.lightType = eMaterialType::HighBeamLeft;
            data.dummies[c.lightType].push_back(VehicleDummy(c));
        } else {
            bool dummyIsLeft = (c.frame->modelling.pos.x < 0.0f);
            c.mirroredX = !dummyIsLeft;
            c.lightType = eMaterialType::HighBeamLeft;
            data.dummies[eMaterialType::HighBeamLeft].push_back(VehicleDummy(c));

            c.mirroredX = dummyIsLeft;
            c.lightType = eMaterialType::HighBeamRight;
            data.dummies[eMaterialType::HighBeamRight].push_back(VehicleDummy(c));
        }
        return true;
    }

    std::string lowerName;
    lowerName.reserve(name.size());
    for (char ch : name) {
        lowerName.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }

    if (lowerName.find("f_pop") != std::string::npos || lowerName.find("popup") != std::string::npos) {
        data.bHasVehFuncsPopUp = true;
        return true;
    }
    return false;
}

void HeadlightComponent::Process(CVehicle* pVeh, VehLightData& data) {
    if (!CanVehicleHaveHeadlights(pVeh)) return;

    bool isAlarmActive = pVeh->m_nAlarmState != 0 && pVeh->m_nAlarmState != 0xFFFF;
    bool isAlarmLightOn = isAlarmActive && ((pVeh->m_nAlarmState & 0x100) != 0);

    bool isNight = Util::IsNightTime() && !CarUtil::IsEngineOff(pVeh);
    bool isForcedOn = CarUtil::IsLightsForcedOn(pVeh);
    bool isForcedOff = CarUtil::IsLightsForcedOff(pVeh);
    bool isEngineOff = LightsConfig::Get().bLightsRequireEngine && CarUtil::IsEngineOff(pVeh);

    if (pVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE) {
        if (isForcedOff || isEngineOff) {
            pVeh->bLightsOn = false;
            data.bAutoNightLights = false;
        } else if (isForcedOn) {
            pVeh->bLightsOn = true;
            data.bAutoNightLights = false;
        } else if (isNight) {
            pVeh->bLightsOn = true;
            data.bAutoNightLights = true;
        } else if (data.bAutoNightLights) {
            pVeh->bLightsOn = false;
            data.bAutoNightLights = false;
        }
    }

    bool isHeadlightsActive = (CarUtil::AreLightsOn(pVeh) || isAlarmLightOn);
    if (isAlarmActive && !isAlarmLightOn) {
        isHeadlightsActive = false;
    }

    if (isHeadlightsActive && !data.bPrevHeadlightsOn) {
        data.nHeadlightsTurnedOnTime = CTimer::m_snTimeInMilliseconds;
    } else if (!isHeadlightsActive) {
        data.nHeadlightsTurnedOnTime = 0;
    }
    data.bPrevHeadlightsOn = isHeadlightsActive;

    CPed* pPlayer = FindPlayerPed();
    if (pPlayer && pVeh->IsDriver(pPlayer)) {
        if (!isHeadlightsActive && data.fLightFactor[eMaterialType::HeadLightLeft] <= 0.001f && data.fLightFactor[eMaterialType::HeadLightRight] <= 0.001f) {
            data.bLongLightsOn = false;
        }

        bool canToggleLongLights = !(gbProperShadersDetected && !LightsConfig::Get().gbLightPointLights);
        if (InputMgr::IsKeyJustDown(LightsConfig::Get().nLongLightKey) && isHeadlightsActive && canToggleLongLights) {
            data.bLongLightsOn = !data.bLongLightsOn;
            AudioMgr::PlaySwitchSound(pVeh);
        }
    }
}

void HeadlightComponent::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {
    if (!CanVehicleHaveHeadlights(pControlVeh)) {
        return;
    }

    bool isAlarmActive = pControlVeh->m_nAlarmState != 0 && pControlVeh->m_nAlarmState != 0xFFFF;
    bool isAlarmLightOn = isAlarmActive && ((pControlVeh->m_nAlarmState & 0x100) != 0);

    bool isNightOrOn = (CarUtil::AreLightsOn(pControlVeh) || isAlarmLightOn);
    if (isAlarmActive && !isAlarmLightOn) {
        isNightOrOn = false;
    }

    bool isOpen = AreHeadlightsOpen(pControlVeh, data);

    auto damage = LightDamageState::Get(pControlVeh, pTowedVeh);
    bool isHeadlightLeftOk = isNightOrOn && isOpen && damage.isHeadlightLeftOk;
    bool isHeadlightRightOk = isNightOrOn && isOpen && damage.isHeadlightRightOk;

    pControlVeh->m_renderLights.m_bLeftFront = isHeadlightLeftOk;
    pControlVeh->m_renderLights.m_bRightFront = isHeadlightRightOk;

    if (!isNightOrOn || !isOpen) return;

    bool isFoggy = Util::IsFoggy();
    std::string texName = data.bLongLightsOn ? "headlight_long" : "headlight_short";
    bool shadow = !gbProperShadersDetected;
    bool highlight = isFoggy || data.bLongLightsOn;

    if (isHeadlightLeftOk) {
        LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::HeadLightLeft, true, shadow ? texName : "", LightsConfig::Get().headlightSz, highlight, true);
        if (data.bLongLightsOn) {
            LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::HighBeamLeft, true, "", LightsConfig::Get().headlightSz, isFoggy, true);
        }
    }
    if (isHeadlightRightOk) {
        LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::HeadLightRight, true, shadow ? texName : "", LightsConfig::Get().headlightSz, highlight, true);
        if (data.bLongLightsOn) {
            LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::HighBeamRight, true, "", LightsConfig::Get().headlightSz, isFoggy, true);
        }
    }
}

void HeadlightComponent::ProcessPointLights(CVehicle* pVeh, VehLightData& data) {
    if (!CanVehicleHaveHeadlights(pVeh)) return;
    bool isAlarmActive = pVeh->m_nAlarmState != 0 && pVeh->m_nAlarmState != 0xFFFF;
    bool isAlarmLightOn = isAlarmActive && ((pVeh->m_nAlarmState & 0x100) != 0);

    bool isHeadlightsOn = (CarUtil::AreLightsOn(pVeh) || isAlarmLightOn);
    if (isAlarmActive && !isAlarmLightOn) {
        isHeadlightsOn = false;
    }

    if (isHeadlightsOn && AreHeadlightsOpen(pVeh, data)) {
        float rangeMul = data.bLongLightsOn ? LightsConfig::Get().fHighBeamPointLightMul : 1.0f;

        for (eMaterialType type : {eMaterialType::HeadLightLeft, eMaterialType::HeadLightRight}) {
            if (!LightManager::IsDummyAvailable(data, type) || !data.bLightStates[type]) {
                continue;
            }

            bool isLeft = (type == eMaterialType::HeadLightLeft);
            eLights lightEnum = isLeft ? eLights::LIGHT_FRONT_LEFT : eLights::LIGHT_FRONT_RIGHT;
            ePanels wingEnum = isLeft ? ePanels::WING_FRONT_LEFT : ePanels::WING_FRONT_RIGHT;
            if (Util::IsLightDamaged(pVeh, lightEnum) || Util::IsPanelDamaged(pVeh, wingEnum)) {
                continue;
            }

            for (auto& e : data.dummies[type]) {
                e->Update();
                RenderUtil::RegisterHeadlightPointLight(&e->Get(), rangeMul);
            }
        }

        if (data.bLongLightsOn) {
            for (eMaterialType type : {eMaterialType::HighBeamLeft, eMaterialType::HighBeamRight}) {
                if (!LightManager::IsDummyAvailable(data, type) || !data.bLightStates[type]) {
                    continue;
                }

                bool isLeft = (type == eMaterialType::HighBeamLeft);
                eLights lightEnum = isLeft ? eLights::LIGHT_FRONT_LEFT : eLights::LIGHT_FRONT_RIGHT;
                ePanels wingEnum = isLeft ? ePanels::WING_FRONT_LEFT : ePanels::WING_FRONT_RIGHT;
                if (Util::IsLightDamaged(pVeh, lightEnum) || Util::IsPanelDamaged(pVeh, wingEnum)) {
                    continue;
                }

                for (auto& e : data.dummies[type]) {
                    e->Update();
                    RenderUtil::RegisterHeadlightPointLight(&e->Get(), rangeMul);
                }
            }
        }
    }
}

