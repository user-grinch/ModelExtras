#include "headlight.h"
#include "enums/materialtype.h"
#include <string_view>
#include <CWeather.h>
#include "utils/audiomgr.h"

#define VEHCOL_HEADLIGHT_LEFT  CRGBA(255, 175, 0, 255)
#define VEHCOL_HEADLIGHT_RIGHT CRGBA(0, 255, 200, 255)

bool HeadlightBehavior::IsValidDummy(RwFrame *pFrame) {
    std::string_view dummyName = GetFrameNodeName(pFrame);
    return dummyName.starts_with("headlights");
}

bool HeadlightBehavior::IsValidMaterial(RpMaterial *pMat) {
    CRGBA color = Util::GetMaterialColor(pMat);
    return color == VEHCOL_HEADLIGHT_LEFT || color == VEHCOL_HEADLIGHT_RIGHT;
}

bool HeadlightBehavior::RegisterDummy(CVehicle *pVeh, RwFrame *pFrame) {
    auto& c = GetTypeData().Get(pVeh).config;
    auto& typeData = GetTypeData().Get(pVeh);
    std::string_view name = GetFrameNodeName(pFrame);

    c.pVeh = pVeh;
    c.frame = pFrame;
    c.dummyPos = eDummyPos::Front;
    c.corona.color = c.shadow.color = {250, 250, 250, 80};
    c.lightType = eMaterialType::HeadLightLeft;
    c.corona.lightingType = eLightingMode::Directional;

    // no shadow for headlights2
    c.shadow.render = name != "headlights2";

    c.mirroredX = true;
    typeData.dummies[c.lightType].push_back(new VehicleDummy(c));
    c.mirroredX = false;
    c.lightType = eMaterialType::HeadLightRight;
    typeData.dummies[c.lightType].push_back(new VehicleDummy(c));
}

eMaterialType HeadlightBehavior::GetMatType(RpMaterial *pMat) {
    CRGBA color = Util::GetMaterialColor(pMat);
    if (color == VEHCOL_HEADLIGHT_LEFT) {
        return eMaterialType::HeadLightLeft;
    }

    if (color == VEHCOL_HEADLIGHT_RIGHT) {
        return eMaterialType::HeadLightRight;
    }

    return eMaterialType::UnknownMaterial;
}

std::vector<eMaterialType> HeadlightBehavior::GetSupportedMatTypes() {
    return {eMaterialType::HeadLightLeft, eMaterialType::HeadLightRight};
}

void HeadlightBehavior::RenderHeadlights(CVehicle *pControlVeh, bool isLeftOn, bool isRightOn)
{
	CVehicle *pTowedVeh = pControlVeh;
	auto &data = GetTypeData().Get(pControlVeh);

	if (pControlVeh->m_pTrailer)
	{
		pTowedVeh = pControlVeh->m_pTrailer;
	}

	int model = pControlVeh->m_nModelIndex;
	if (CModelInfo::IsTrailerModel(model) || CarUtil::IsLightsForcedOff(pControlVeh) || CModelInfo::IsBmxModel(model) || CModelInfo::IsBoatModel(model) || CModelInfo::IsHeliModel(model) || CModelInfo::IsPlaneModel(model))
	{
		return;
	}

	if (pControlVeh->bLightsOn || CarUtil::IsLightsForcedOn(pControlVeh) || Util::IsNightTime())
	{
		bool isFoggy = (CWeather::NewWeatherType == WEATHER_FOGGY_SF || CWeather::NewWeatherType == WEATHER_SANDSTORM_DESERT || CWeather::OldWeatherType == WEATHER_FOGGY_SF || CWeather::OldWeatherType == WEATHER_SANDSTORM_DESERT);
		std::string texName = data.bLongLights ? "headlight_long" : "headlight_short";

		if (isLeftOn || isRightOn)
		{
			if (isLeftOn)
			{
				RenderLights(pControlVeh, pTowedVeh);
			}
			if (isRightOn)
			{
				RenderLights(pControlVeh, pTowedVeh);
			}
		}
	}
}

void HeadlightBehavior::Process(CVehicle *pVeh) {
    if (Util::IsEngineOff(pVeh) 
    || pVeh->m_nVehicleSubClass == VEHICLE_BMX || pVeh->m_nVehicleSubClass == VEHICLE_BOAT || pVeh->m_nVehicleSubClass == VEHICLE_TRAILER) {
        return;
    }
    
    if (pVeh->m_pDriver == FindPlayerPed()) {
        static size_t prev = 0;
        static uint32_t longLightKey = gConfig.ReadInteger("KEYS", "LongLightKey", VK_G);
        if (KeyPressed(longLightKey) && (pVeh->bLightsOn || CarUtil::IsLightsForcedOn(pVeh)))
        {
            size_t now = CTimer::m_snTimeInMilliseconds;
            if (now - prev > 500.0f)
            {
                auto& data = GetTypeData().Get(pVeh);
                data.bLongLights = !data.bLongLights;
                prev = now;
                AudioMgr::PlaySwitchSound(pVeh);
            }
        }
    } else {
        if (DistanceBetweenPoints(pVeh->GetPosition(), TheCamera.GetPosition()) < 150.0f || pVeh->GetIsOnScreen())
        {
            bool isLeftFrontOk = !Util::IsLightDamaged(pVeh, eLights::LIGHT_FRONT_LEFT);
            bool isRightFrontOk = !Util::IsLightDamaged(pVeh, eLights::LIGHT_FRONT_RIGHT);
            RenderHeadlights(pVeh, isLeftFrontOk, isRightFrontOk);
        }
    }
}

void HeadlightBehavior::Render(CVehicle *pControlVeh, CVehicle *pTowedVeh) {
    if (pControlVeh->m_pDriver == FindPlayerPed()) {
        bool isLeftFrontOk = !Util::IsLightDamaged(pControlVeh, eLights::LIGHT_FRONT_LEFT);
        bool isRightFrontOk = !Util::IsLightDamaged(pControlVeh, eLights::LIGHT_FRONT_RIGHT);
        RenderHeadlights(pControlVeh, pControlVeh->m_renderLights.m_bLeftFront && isLeftFrontOk, pControlVeh->m_renderLights.m_bRightFront && isRightFrontOk);
    }
}
