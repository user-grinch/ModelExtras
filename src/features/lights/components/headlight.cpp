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

bool HeadlightComponent::TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string_view name, VehLightData& data) {
    if (name == "headlights" || name == "headlights2") {
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.dummyPos = eDummyPos::Front;
        c.lightType = eMaterialType::HeadLightLeft;
        c.corona.color = c.shadow.color = {250, 250, 250, static_cast<unsigned char>(LightsConfig::Get().gGlobalCoronaIntensity)};
        c.corona.lightingType = eLightingMode::Directional;
        c.shadow.render = name != "headlights2";
        
        c.mirroredX = true;
        data.dummies[eMaterialType::HeadLightLeft].push_back(new VehicleDummy(c));
        
        if (pVeh->m_nVehicleSubClass != VEHICLE_BIKE || std::abs(c.frame->modelling.pos.x) > 0.05f) {
            c.mirroredX = false;
            c.lightType = eMaterialType::HeadLightRight;
            data.dummies[eMaterialType::HeadLightRight].push_back(new VehicleDummy(c));
        }
        return true;
    }
    return false;
}

void HeadlightComponent::Process(CVehicle* pVeh, VehLightData& data) {
    if (pVeh->IsDriver(FindPlayerPed()) && !Util::IsEngineOff(pVeh)) {
        static size_t prev = 0;
        bool isHeadlightsActiveForLong = (pVeh->bLightsOn || CarUtil::IsLightsForcedOn(pVeh) || Util::IsNightTime() || !Util::IsEngineOff(pVeh)) && !CarUtil::IsLightsForcedOff(pVeh);
        bool canToggleLongLights = !(gbProperShadersDetected && !LightsConfig::Get().gbLightPointLights);
        if (Util::IsKeyPressed(LightsConfig::Get().nLongLightKey) && isHeadlightsActiveForLong && canToggleLongLights) {
            size_t now = CTimer::m_snTimeInMilliseconds;
            if (now - prev > 500.0f) {
                data.bLongLightsOn = !data.bLongLightsOn;
                prev = now;
                AudioMgr::PlaySwitchSound(pVeh);
            }
        }
    } else if (pVeh->m_nVehicleSubClass != VEHICLE_BMX && pVeh->m_nVehicleSubClass != VEHICLE_BOAT && pVeh->m_nVehicleSubClass != VEHICLE_TRAILER && pVeh->m_fHealth > 0.0f) {
        if (CarUtil::IsLightsForcedOff(pVeh) || (Util::IsEngineOff(pVeh) && !CarUtil::IsLightsForcedOn(pVeh) && !pVeh->bLightsOn)) {
            return;
        }

        if (CVector::Distance(pVeh->GetPosition(), TheCamera.GetPosition()) < 150.0f || pVeh->GetIsOnScreen()) {
            bool isLeftFrontOk = !Util::IsLightDamaged(pVeh, eLights::LIGHT_FRONT_LEFT);
            bool isRightFrontOk = !Util::IsLightDamaged(pVeh, eLights::LIGHT_FRONT_RIGHT);
            bool isNightOrOn = (pVeh->bLightsOn || CarUtil::IsLightsForcedOn(pVeh) || (Util::IsNightTime() && !Util::IsEngineOff(pVeh))) && !CarUtil::IsLightsForcedOff(pVeh);
            if (isNightOrOn) {
                bool isFoggy = (CWeather::Foggyness > 0.1f) || (CWeather::Rain > 0.3f) || (CWeather::NewWeatherType == WEATHER_FOGGY_SF || CWeather::NewWeatherType == WEATHER_SANDSTORM_DESERT || CWeather::OldWeatherType == WEATHER_FOGGY_SF || CWeather::OldWeatherType == WEATHER_SANDSTORM_DESERT);
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
    int model = pControlVeh->m_nModelIndex;
    if (CModelInfo::IsTrailerModel(model) || CarUtil::IsLightsForcedOff(pControlVeh) || CModelInfo::IsBmxModel(model) || CModelInfo::IsBoatModel(model) || CModelInfo::IsHeliModel(model) || CModelInfo::IsPlaneModel(model)) {
        return;
    }

    bool isNightOrOn = (pControlVeh->bLightsOn || CarUtil::IsLightsForcedOn(pControlVeh) || (Util::IsNightTime() && !Util::IsEngineOff(pControlVeh))) && !CarUtil::IsLightsForcedOff(pControlVeh);
    if (!isNightOrOn) return;

    auto damage = LightDamageState::Get(pControlVeh, pTowedVeh);
    bool isHeadlightLeftOk = damage.isHeadlightLeftOk;
    bool isHeadlightRightOk = damage.isHeadlightRightOk;

    bool bTickRegistered = (data.nHeadlightTickFrame == CTimer::m_FrameCounter);
    bool isFoggy = (CWeather::Foggyness > 0.1f) || (CWeather::Rain > 0.3f) || (CWeather::NewWeatherType == WEATHER_FOGGY_SF || CWeather::NewWeatherType == WEATHER_SANDSTORM_DESERT || CWeather::OldWeatherType == WEATHER_FOGGY_SF || CWeather::OldWeatherType == WEATHER_SANDSTORM_DESERT);
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
    bool isHeadlightsOn = (pVeh->bLightsOn || CarUtil::IsLightsForcedOn(pVeh) || (Util::IsNightTime() && !Util::IsEngineOff(pVeh)) || (pVeh->m_nVehicleSubClass == VEHICLE_BIKE && !Util::IsEngineOff(pVeh))) && !CarUtil::IsLightsForcedOff(pVeh);

    if (data.bLongLightsOn && isHeadlightsOn) {
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

            for (auto e : data.dummies[type]) {
                e->Update();
                RenderUtil::RegisterHeadlightPointLight(&e->Get(), highBeamMul);
            }
        }
    }
}

