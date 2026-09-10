#include "pch.h"
#include "headlight.h"
#include "utils/util.h"
#include "utils/car.h"
#include "utils/audiomgr.h"
#include "utils/render.h"
#include "../damage.h"
#include <CWeather.h>

extern bool gbProperShadersDetected;

void HeadlightComponent::RegisterMaterials(std::unordered_map<uint32_t, eMaterialType>& matMap) {
    matMap[VEHCOL_HEADLIGHT_LEFT.ToInt()] = eMaterialType::HeadLightLeft;
    matMap[VEHCOL_HEADLIGHT_RIGHT.ToInt()] = eMaterialType::HeadLightRight;
}

eMaterialType HeadlightComponent::GetMatType(CRGBA matCol) {
    if (matCol == VEHCOL_HEADLIGHT_LEFT) return eMaterialType::HeadLightLeft;
    if (matCol == VEHCOL_HEADLIGHT_RIGHT) return eMaterialType::HeadLightRight;
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
    bool hasPopUp = (pAuto->m_aCarNodes[CAR_MISC_A] != nullptr) || data.bHasVehFuncsPopUp;
    if (!hasPopUp) {
        return true;
    }

    return pAuto->m_renderLights.m_bLeftFront || pAuto->m_renderLights.m_bRightFront || pAuto->m_fPropRotate >= 0.68f;
}

bool HeadlightComponent::TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string_view name, VehLightData& data) {
    if (!CanVehicleHaveHeadlights(pVeh)) return false;
    if (name == "headlights" || name == "headlights2") {
        if (pFrame && !rwLinkListEmpty(&pFrame->objectList)) {
            return false;
        }
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.dummyPos = eDummyPos::Front;
        c.lightType = eMaterialType::HeadLightLeft;
        c.corona.size = LightsConfig::Get().gfHeadLightCoronaSize;
        c.corona.color = {250, 250, 250, static_cast<unsigned char>(LightsConfig::Get().gHeadLightCoronaIntensity)};
        c.shadow.color = {250, 250, 250, static_cast<unsigned char>(LightsConfig::Get().gHeadLightShadowIntensity)};
        c.shadow.size = LightsConfig::Get().gfHeadLightShadowSize;
        c.corona.lightingType = eLightingMode::Directional;
        c.shadow.render = name != "headlights2";
        
        c.mirroredX = true;
        data.dummies[eMaterialType::HeadLightLeft].push_back(VehicleDummy(c));
        
        if (pVeh->m_nVehicleSubClass != VEHICLE_BIKE || std::abs(c.frame->modelling.pos.x) > 0.05f) {
            c.mirroredX = false;
            c.lightType = eMaterialType::HeadLightRight;
            data.dummies[eMaterialType::HeadLightRight].push_back(VehicleDummy(c));
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

    bool isHeadlightsActive = (pVeh->bLightsOn || CarUtil::IsLightsForcedOn(pVeh) || (Util::IsNightTime() && !Util::IsEngineOff(pVeh))) && !CarUtil::IsLightsForcedOff(pVeh);
    if (pVeh->IsDriver(FindPlayerPed())) {
        if (!isHeadlightsActive) {
            data.bLongLightsOn = false;
        }

        bool canToggleLongLights = !(gbProperShadersDetected && !LightsConfig::Get().gbLightPointLights);
        if (InputMgr::IsKeyJustDown(LightsConfig::Get().nLongLightKey) && isHeadlightsActive && canToggleLongLights) {
            data.bLongLightsOn = !data.bLongLightsOn;
            AudioMgr::PlaySwitchSound(pVeh);
        }
    } else if (pVeh->m_fHealth > 0.0f) {
        if (CarUtil::IsLightsForcedOff(pVeh) || (Util::IsEngineOff(pVeh) && !CarUtil::IsLightsForcedOn(pVeh) && !pVeh->bLightsOn)) {
            return;
        }

        if (CVector::Distance(pVeh->GetPosition(), TheCamera.GetPosition()) < 300.0f || pVeh->GetIsOnScreen()) {
            bool isLeftFrontOk = !Util::IsLightDamaged(pVeh, eLights::LIGHT_FRONT_LEFT);
            bool isRightFrontOk = !Util::IsLightDamaged(pVeh, eLights::LIGHT_FRONT_RIGHT);

            if (isHeadlightsActive && AreHeadlightsOpen(pVeh, data)) {
                bool isFoggy = Util::IsFoggy();
                std::string texName = data.bLongLightsOn ? "headlight_long" : "headlight_short";
                bool shadow = !gbProperShadersDetected;
                bool highlight = isFoggy || data.bLongLightsOn;

                LightManager::RenderLight(pVeh, data, eMaterialType::HeadLightLeft, isLeftFrontOk, shadow ? texName : "", LightsConfig::Get().headlightSz, highlight);
                LightManager::RenderLight(pVeh, data, eMaterialType::HeadLightRight, isRightFrontOk, shadow ? texName : "", LightsConfig::Get().headlightSz, highlight);
                data.nHeadlightTickFrame = CTimer::m_FrameCounter;
            }
        }
    }
}

void HeadlightComponent::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {
    if (!CanVehicleHaveHeadlights(pControlVeh)) {
        return;
    }

    bool isNightOrOn = (pControlVeh->bLightsOn || CarUtil::IsLightsForcedOn(pControlVeh) || (Util::IsNightTime() && !Util::IsEngineOff(pControlVeh))) && !CarUtil::IsLightsForcedOff(pControlVeh);
    if (!isNightOrOn || !AreHeadlightsOpen(pControlVeh, data)) return;

    auto damage = LightDamageState::Get(pControlVeh, pTowedVeh);
    bool isHeadlightLeftOk = damage.isHeadlightLeftOk;
    bool isHeadlightRightOk = damage.isHeadlightRightOk;

    bool bTickRegistered = (data.nHeadlightTickFrame == CTimer::m_FrameCounter);
    bool isFoggy = Util::IsFoggy();
    std::string texName = data.bLongLightsOn ? "headlight_long" : "headlight_short";
    bool shadow = !gbProperShadersDetected;
    bool highlight = isFoggy || data.bLongLightsOn;

    if (isHeadlightLeftOk || isHeadlightRightOk) {
        if (isHeadlightLeftOk) {
            LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::HeadLightLeft, true, shadow ? texName : "", LightsConfig::Get().headlightSz, highlight, true, bTickRegistered);
        }
        if (isHeadlightRightOk) {
            LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::HeadLightRight, true, shadow ? texName : "", LightsConfig::Get().headlightSz, highlight, true, bTickRegistered);
        }
    }
}

void HeadlightComponent::ProcessPointLights(CVehicle* pVeh, VehLightData& data) {
    if (!CanVehicleHaveHeadlights(pVeh)) return;
    bool isHeadlightsOn = (pVeh->bLightsOn || CarUtil::IsLightsForcedOn(pVeh) || (Util::IsNightTime() && !Util::IsEngineOff(pVeh))) && !CarUtil::IsLightsForcedOff(pVeh);

    if (data.bLongLightsOn && isHeadlightsOn && AreHeadlightsOpen(pVeh, data)) {
        float highBeamMul = LightsConfig::Get().fHighBeamPointLightMul;

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
                RenderUtil::RegisterHeadlightPointLight(&e->Get(), highBeamMul);
            }
        }
    }
}

