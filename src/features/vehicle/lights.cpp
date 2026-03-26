#include "pch.h"
#include "lights.h"
#include <CClock.h>
#include "defines.h"
#include <CShadows.h>
#include <eVehicleClass.h>
#include <rwcore.h>
#include <rpworld.h>
#include "spotlights.h"
#include "audiomgr.h"
#include <CWeather.h>
#include <CCoronas.h>
#include "enums/vehdummy.h"
#include "datamgr.h"
#include "core/colors.h"
#include <CPointLights.h>
#include "ModelExtrasAPI.h"

using namespace plugin;

// flags
bool gbGlobalIndicatorLights = false;
float gfGlobalCoronaSize = 0.3f;
int gGlobalCoronaIntensity = 80;
int gGlobalShadowIntensity = 80;
float headlightSz = 5.0f;

int GetStrobeIndex(CVehicle *pVeh, RpMaterial *pMat)
{
	return pMat->color.blue;
}

// Indicator lights
static uint64_t delay;

CVector2D GetCarPathLinkPosition(CCarPathLinkAddress &address)
{
	if (address.m_nAreaId >= 0 && address.m_nCarPathLinkId >= 0 && ThePaths.m_pNaviNodes && ThePaths.m_pNaviNodes[address.m_nAreaId])
	{
		return CVector2D(static_cast<float>(ThePaths.m_pNaviNodes[address.m_nAreaId][address.m_nCarPathLinkId].m_vecPosn.x) / 8.0f,
						 static_cast<float>(ThePaths.m_pNaviNodes[address.m_nAreaId][address.m_nCarPathLinkId].m_vecPosn.y) / 8.0f);
	}
	return CVector2D(0.0f, 0.0f);
}

inline float GetZAngleForPoint(CVector2D const &point)
{
	float angle = CGeneral::GetATanOfXY(point.x, point.y) * 57.295776f - 90.0f;
	while (angle < 0.0f)
		angle += 360.0f;
	return angle;
}

void Lights::Initialize()
{
	m_bEnabled = true;
	patch::Nop(0x6E2722, 19);	  // CVehicle::DoHeadLightReflection
	patch::SetUChar(0x6E1A22, 0); // CVehicle::DoTailLightEffect

	// // CVehicle::DoHeadLightEffect
	patch::SetUChar(0x6E0CF8, 0);
	patch::SetUChar(0x6E0DEE, 0);

	// NOP CVehicle::DoHeadLightBeam
	if (!gConfig.ReadBoolean("TWEAKS", "HeadLightBeams", true))
	{
		// cmp ax, ax
		patch::SetRaw(0x6A2EA5, (void *)"\x66\x39\xC0\x90", 4);
		patch::SetRaw(0x6BDE63, (void *)"\x66\x39\xC0\x90\x90\x90\x90", 7);
	}

	Events::initGameEvent += []()
	{
		gbGlobalIndicatorLights = gConfig.ReadBoolean("VEHICLE_FEATURES", "StandardLights_GlobalIndicatorLights", false);

		gfGlobalCoronaSize = gConfig.ReadFloat("VISUAL", "LightCoronaSize", 0.3f);
		gGlobalShadowIntensity = gConfig.ReadInteger("VISUAL", "LightShadowIntensity", 220);
		gGlobalCoronaIntensity = gConfig.ReadInteger("VISUAL", "LightCoronaIntensity", 250);
	};

	Events::vehicleDtorEvent += [](CVehicle *pVeh)
	{
		m_Dummies.erase(pVeh);
	};

	ModelInfoMgr::RegisterMaterial([](CVehicle *pVeh, RpMaterial *pMat)
								   {
		if (!m_bEnabled) {
			return eMaterialType::UnknownMaterial;
		}
		// Headlights
		CRGBA matCol = *reinterpret_cast<CRGBA *>(RpMaterialGetColor(pMat));
		matCol.a = 255;
		if (matCol == VEHCOL_HEADLIGHT_LEFT) {
			return eMaterialType::HeadLightLeft;
		} else if (matCol == VEHCOL_HEADLIGHT_RIGHT) {
			return eMaterialType::HeadLightRight;
		}
		// Taillights
		else if (matCol == VEHCOL_TAILLIGHT_LEFT) {
			return eMaterialType::TailLightLeft;
		} else if (matCol == VEHCOL_TAILLIGHT_RIGHT) {
			return eMaterialType::TailLightRight;
		}
		// Reverse Lights
		else if (matCol == VEHCOL_REVERSELIGHT_LEFT) {
			return eMaterialType::ReverseLightLeft;
		}
		else if (matCol == VEHCOL_REVERSELIGHT_RIGHT) {
			return eMaterialType::ReverseLightRight;
		}
		// Brake Lights
		else if (matCol == VEHCOL_BRAKELIGHT_LEFT) {
			return eMaterialType::BrakeLightLeft;
		}
		else if (matCol == VEHCOL_BRAKELIGHT_RIGHT) {
			return eMaterialType::BrakeLightRight;
		}
		// All Day Lights
		else if (matCol == VEHCOL_ALLDAYLIGHT_1 || matCol == VEHCOL_ALLDAYLIGHT_2) {
			return eMaterialType::AllDayLight;
		}
		// Day Lights
		else if (matCol == VEHCOL_DAYLIGHT_1 || matCol == VEHCOL_DAYLIGHT_2) {
			return eMaterialType::DayLight;
		}
		// Night Lights
		else if (matCol == VEHCOL_NIGHTLIGHT_1 || matCol == VEHCOL_NIGHTLIGHT_2) {
			return eMaterialType::NightLight;
		}
		// Fog Lights
		else if (matCol == VEHCOL_FOGLIGHT_LEFT) {
			return eMaterialType::FogLightLeft;
		}
		else if (matCol == VEHCOL_FOGLIGHT_RIGHT) {
			return eMaterialType::FogLightRight;
		}
		// Sidelights
		else if (matCol == VEHCOL_SIDELIGHT_LEFT) {
			return eMaterialType::SideLightLeft;
		} else if (matCol == VEHCOL_SIDELIGHT_RIGHT) {
			return eMaterialType::SideLightRight;
		}
		// STT Lights
		else if (matCol == VEHCOL_STTLIGHT_LEFT) {
			return eMaterialType::STTLightLeft;
		} else if (matCol == VEHCOL_STTLIGHT_RIGHT) {
			return eMaterialType::STTLightRight;
		}
		// NA Brake Lights
		else if (matCol == VEHCOL_NABRAKE_LEFT) {
			return eMaterialType::NABrakeLightLeft;
		} else if (matCol == VEHCOL_NABRAKE_RIGHT) {
			return eMaterialType::NABrakeLightRight;
		}
		// Spot and Strobe Lights
		else if (matCol == VEHCOL_SPOTLIGHT) {
			return eMaterialType::SpotLight;
		} else if (matCol == VEHCOL_STROBELIGHT) {
			return eMaterialType::StrobeLight;
		}
		// Indicator Lights (Left)
		if (matCol == VEHCOL_INDICATOR_LEFT_REAR) {
			return eMaterialType::IndicatorLightLeftRear;
		}
		else if (matCol == VEHCOL_INDICATOR_LEFT_SIDE) {
			return eMaterialType::IndicatorLightLeftMiddle;
		}
		else if (matCol == VEHCOL_INDICATOR_LEFT_FRONT) {
			return eMaterialType::IndicatorLightLeftFront;
		}
		// Indicator Lights (Right)
		else if (matCol == VEHCOL_INDICATOR_RIGHT_REAR) {
			return eMaterialType::IndicatorLightRightRear;
		}
		else if (matCol == VEHCOL_INDICATOR_RIGHT_SIDE) {
			return eMaterialType::IndicatorLightRightMiddle;
		}
		else if (matCol == VEHCOL_INDICATOR_RIGHT_FRONT) {
			return eMaterialType::IndicatorLightRightFront;
		}
		// If no match is found
		return eMaterialType::UnknownMaterial; });

	ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *frame)
								{
		std::string name = GetFrameNodeName(frame);
		VehicleDummyConfig c = {
			.pVeh = pVeh,
			.frame = frame,
			.corona = {
				.color = {255, 255, 255, static_cast<unsigned char>(gGlobalCoronaIntensity)}
			}
		};

		auto &dummies = m_Dummies[pVeh];

		if (name.starts_with("fogl") && (STR_FOUND(name, "_l") || STR_FOUND(name, "_r"))) {
			c.dummyType = eDummyPos::Front;
			c.lightType = STR_FOUND(name, "_l") ? eMaterialType::FogLightLeft : eMaterialType::FogLightRight;
			c.shadow.render = false;
			c.corona.color = c.shadow.color = {255, 255, 255, static_cast<unsigned char>(gGlobalCoronaIntensity)};
			c.corona.lightingType = eLightingMode::Directional;
		}
		else if (name.starts_with("rev") && (STR_FOUND(name, "_l") || STR_FOUND(name, "_r"))) {
			c.dummyType = eDummyPos::Rear;
			c.lightType = STR_FOUND(name, "_l") ? eMaterialType::ReverseLightLeft : eMaterialType::ReverseLightRight;
			c.corona.color = c.shadow.color = {255, 255, 255, static_cast<unsigned char>(gGlobalCoronaIntensity)};
			c.corona.lightingType = eLightingMode::Directional;
		}
		else if (name.starts_with("breakl") && (STR_FOUND(name, "_l") || STR_FOUND(name, "_r"))) {
			c.dummyType = eDummyPos::Rear;
			c.lightType = STR_FOUND(name, "_l") ? eMaterialType::BrakeLightLeft : eMaterialType::BrakeLightRight;
			c.corona.color = c.shadow.color = {240, 0, 0, static_cast<unsigned char>(gGlobalCoronaIntensity)};
			c.corona.lightingType = eLightingMode::Directional;
		}
		else if (name.starts_with("light_d")) {
			c.lightType = eMaterialType::DayLight;
			c.dummyType = eDummyPos::Front;
		}
		else if (name.starts_with("light_n")) {
			c.lightType = eMaterialType::NightLight;
			c.dummyType = eDummyPos::Front;
		}
		else if (auto d = Util::GetDigitsAfter(name, "strobe_light")) {
			c.lightType = eMaterialType::StrobeLight;
			c.dummyType = eDummyPos::Front;
			c.dummyIdx = d.value();
		}
		else if (auto d = Util::GetCharsAfterPrefix(name, "sidelight_", 1)) {
			if (d == "L") {
				c.lightType = eMaterialType::SideLightLeft;
				c.dummyType = eDummyPos::Left;
			} else {
				c.lightType = eMaterialType::SideLightRight;
				c.dummyType = eDummyPos::Right;
			}
		}
		else if (auto d = Util::GetCharsAfterPrefix(name, "sttlight_", 1)) {
			c.lightType = (d == "L") ? eMaterialType::STTLightLeft : eMaterialType::STTLightRight;
			c.dummyType = eDummyPos::Rear;
			c.corona.color = c.shadow.color = {240, 0, 0, static_cast<unsigned char>(gGlobalCoronaIntensity)};
			c.corona.lightingType = eLightingMode::Directional;
		}
		else if (auto d = Util::GetCharsAfterPrefix(name, "nabrakelight_", 1)) {
			c.lightType = (d == "L") ? eMaterialType::NABrakeLightLeft : eMaterialType::NABrakeLightRight;
			c.dummyType = eDummyPos::Rear;
			c.corona.color = c.shadow.color = {240, 0, 0, static_cast<unsigned char>(gGlobalCoronaIntensity)};
			c.corona.lightingType = eLightingMode::Directional;
		}
		else if (name.starts_with("spotlight_light")) {
			c.lightType = eMaterialType::SpotLight;
		}
		else if (name.starts_with("light_a")) {
			c.lightType = eMaterialType::AllDayLight;
			c.dummyType = eDummyPos::Front;
		}
		else if (name == "taillights" || name == "taillights2") { // some models have dummies starting with taillights
			c.dummyType = eDummyPos::Rear;
			c.lightType = eMaterialType::TailLightRight;
			c.corona.color = c.shadow.color = {250, 0, 0, static_cast<unsigned char>(gGlobalCoronaIntensity)};
			c.corona.lightingType = eLightingMode::Directional; 				
			c.shadow.render = name != "taillights2";
			dummies[c.lightType].push_back(new VehicleDummy(c));
			c.mirroredX = true;
			c.lightType = eMaterialType::TailLightLeft;
		}
		else if (name == "headlights" || name == "headlights2") {
			c.dummyType = eDummyPos::Front;
			c.lightType = eMaterialType::HeadLightLeft;
			c.corona.color = c.shadow.color = {250, 250, 250, static_cast<unsigned char>(gGlobalCoronaIntensity)};
			c.corona.lightingType = eLightingMode::Directional;
			c.shadow.render = name != "headlights2";
			c.mirroredX = true;
			dummies[c.lightType].push_back(new VehicleDummy(c));
			c.mirroredX = false;
			c.lightType = eMaterialType::HeadLightRight;
		}
		else if (name.starts_with("turnl_") || name.starts_with("indicator_")) {
			auto d = Util::GetCharsAfterPrefix(name, "turnl_", 2);
			if (!d) d = Util::GetCharsAfterPrefix(name, "indicator_", 2);
			if (d) {
				bool isLeft = (d.value()[0] == 'L');
				c.corona.color = c.shadow.color = {255, 128, 0, static_cast<unsigned char>(gGlobalCoronaIntensity)};
				c.corona.lightingType = eLightingMode::Directional;

				switch (d.value()[1]) {
					case 'F':
						c.lightType = isLeft ? eMaterialType::IndicatorLightLeftFront : eMaterialType::IndicatorLightRightFront;
						c.dummyType = eDummyPos::Front;
						break;
					case 'R':
						c.lightType = isLeft ? eMaterialType::IndicatorLightLeftRear : eMaterialType::IndicatorLightRightRear;
						c.dummyType = eDummyPos::Rear;
						break;
					case 'M':
						c.lightType = isLeft ? eMaterialType::IndicatorLightLeftMiddle : eMaterialType::IndicatorLightRightMiddle;
						c.dummyType = isLeft ? eDummyPos::Right : eDummyPos::Left;
						break;
				}
			}
		}
		else {
			return;
		}

		dummies[c.lightType].push_back(new VehicleDummy(c)); });

	Events::processScriptsEvent += []()
	{
		size_t timestamp = CTimer::m_snTimeInMilliseconds;
		if ((timestamp - delay) > 500)
		{
			delay = timestamp;
			indicatorsDelay = !indicatorsDelay;
		}

		CVehicle *pVeh = FindPlayerVehicle(-1, false);
		if (pVeh && pVeh->IsDriver(FindPlayerPed()) && !Util::IsEngineOff(pVeh))
		{
			static size_t prev = 0;
			static uint32_t fogLightKey = gConfig.ReadInteger("KEYS", "FogLightKey", VK_J);

			static bool foglightTiedtoHeadlight = gConfig.ReadBoolean("TWEAKS", "FoglightTiedToHeadlight", true);
			bool headlightStatus = !foglightTiedtoHeadlight || pVeh->bLightsOn;
			if (KeyPressed(fogLightKey) && IsMatAvail(pVeh, {eMaterialType::FogLightLeft, eMaterialType::FogLightRight}) && headlightStatus)
			{
				size_t now = CTimer::m_snTimeInMilliseconds;
				if (now - prev > 500.0f)
				{
					VehLightData &data = m_VehData.Get(pVeh);
					data.m_bFogLightsOn = !data.m_bFogLightsOn;
					prev = now;
					AudioMgr::PlaySwitchSound(pVeh);
				}
			}

			static uint32_t longLightKey = gConfig.ReadInteger("KEYS", "LongLightKey", VK_G);
			if (KeyPressed(longLightKey) && (pVeh->bLightsOn || CarUtil::IsLightsForcedOn(pVeh)))
			{
				size_t now = CTimer::m_snTimeInMilliseconds;
				if (now - prev > 500.0f)
				{
					VehLightData &data = m_VehData.Get(pVeh);
					data.m_bLongLightsOn = !data.m_bLongLightsOn;
					prev = now;
					AudioMgr::PlaySwitchSound(pVeh);
				}
			}
		}
	};

	Events::processScriptsEvent += []()
	{
		for (CVehicle *pVeh : CPools::ms_pVehiclePool)
		{
			if (pVeh->m_pDriver == FindPlayerPed() || pVeh->m_nVehicleSubClass == VEHICLE_BMX || pVeh->m_nVehicleSubClass == VEHICLE_BOAT || pVeh->m_nVehicleSubClass == VEHICLE_TRAILER || Util::IsEngineOff(pVeh))
			{
				continue;
			}

			if (DistanceBetweenPoints(pVeh->GetPosition(), TheCamera.GetPosition()) < 150.0f || pVeh->GetIsOnScreen())
			{
				bool isLeftFrontOk = !Util::IsLightDamaged(pVeh, eLights::LIGHT_FRONT_LEFT);
				bool isRightFrontOk = !Util::IsLightDamaged(pVeh, eLights::LIGHT_FRONT_RIGHT);
				RenderHeadlights(pVeh, isLeftFrontOk, isRightFrontOk, false);
			}
		}
	};

	ModelInfoMgr::RegisterRender([](CVehicle *pControlVeh)
								 {
		int model = pControlVeh->m_nModelIndex;

		// skip directly processing trailers
		if (CModelInfo::IsTrailerModel(model)) {
			return;
		}

		CVehicle *pTowedVeh = pControlVeh;
		
		if (pControlVeh->m_pTrailer)
		{
			pTowedVeh = pControlVeh->m_pTrailer;
		}

		VehLightData &data = m_VehData.Get(pControlVeh);
		eIndicatorState indState = data.m_nIndicatorState;

		// Fix for UIF SAMP server https://github.com/user-grinch/ModelExtras/issues/112
		if (Util::IsEngineOff(pControlVeh) || CarUtil::IsLightsForcedOff(pControlVeh)) {
			pControlVeh->bLightsOn = false;
			pControlVeh->m_renderLights.m_bLeftFront = false;
			pControlVeh->m_renderLights.m_bRightFront = false;
			pControlVeh->m_renderLights.m_bLeftRear = false;
			pControlVeh->m_renderLights.m_bRightRear = false;
		}

		// Fix for park car alarm lights
		if (pControlVeh->m_fHealth == 0 || (Util::IsEngineOff(pControlVeh) && !CarUtil::IsLightsForcedOn(pControlVeh))) {
			return;
		}

		bool isLeftFrontOk = !Util::IsLightDamaged(pControlVeh, eLights::LIGHT_FRONT_LEFT);
		bool isRightFrontOk = !Util::IsLightDamaged(pControlVeh, eLights::LIGHT_FRONT_RIGHT);
		bool isLeftRearOk = !(Util::IsLightDamaged(pTowedVeh, eLights::LIGHT_REAR_LEFT)
								|| Util::IsPanelDamaged(pTowedVeh, ePanels::WING_REAR_LEFT) 
							);
		bool isRightRearOk = !(Util::IsLightDamaged(pTowedVeh, eLights::LIGHT_REAR_RIGHT)
								|| Util::IsPanelDamaged(pTowedVeh, ePanels::WING_REAR_RIGHT) 
							);
		RenderLights(pControlVeh, pTowedVeh, eMaterialType::AllDayLight);
		RenderLights(pControlVeh, pTowedVeh, eMaterialType::StrobeLight);
		RenderLights(pControlVeh, pTowedVeh, eMaterialType::SideLightLeft);
		RenderLights(pControlVeh, pTowedVeh, eMaterialType::SideLightRight);
		
		if (Util::IsNightTime()) {
			RenderLights(pControlVeh, pTowedVeh, eMaterialType::NightLight);
		} else {
			RenderLights(pControlVeh, pTowedVeh, eMaterialType::DayLight);
		}
		
		if (data.m_bFogLightsOn) {
			RenderLights(pControlVeh, pTowedVeh, eMaterialType::FogLightLeft, true, "foglight", 3.0f);
			RenderLights(pControlVeh, pTowedVeh, eMaterialType::FogLightRight, true, "foglight", 3.0f);
		}

		bool isBike = CModelInfo::IsBikeModel(pControlVeh->m_nModelIndex);

		// RenderLights checks are required for popup lights
		if (pControlVeh->m_pDriver == FindPlayerPed()) {
			RenderHeadlights(pControlVeh, pControlVeh->m_renderLights.m_bLeftFront && isLeftFrontOk, pControlVeh->m_renderLights.m_bRightFront && isRightFrontOk);
		}

		if (SpotLights::IsEnabled(pControlVeh)) {
			RenderLights(pControlVeh, pTowedVeh, eMaterialType::SpotLight, false);
		}

		std::string shdwName = (isBike ? "taillight_bike" : "taillight");
		float shdwSz = 2.0f;

		if (pControlVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pControlVeh->m_nVehicleSubClass == VEHICLE_MTRUCK
			|| pControlVeh->m_nVehicleSubClass == VEHICLE_QUAD || pControlVeh->m_nVehicleSubClass == VEHICLE_BIKE
			|| pControlVeh->m_nVehicleSubClass == VEHICLE_TRAILER
		)
		{
			bool isRevlightSupportedByModel = IsMatAvail(pTowedVeh, {eMaterialType::ReverseLightLeft, eMaterialType::ReverseLightRight});

			bool reverseLightsOn = !isBike && isRevlightSupportedByModel && pControlVeh->m_nCurrentGear == 0 && (Util::GetVehicleSpeed(pControlVeh) >= 0.001f) && pControlVeh->m_pDriver;
			if (reverseLightsOn) {
				if (isLeftRearOk) {
					RenderLights(pControlVeh, pTowedVeh, eMaterialType::ReverseLightLeft, true, shdwName, shdwSz);
				}
				if (isRightRearOk) {
					RenderLights(pControlVeh, pTowedVeh, eMaterialType::ReverseLightRight, true, shdwName, shdwSz);
				}
			}

			bool sttInstalled = IsMatAvail(pTowedVeh, {eMaterialType::STTLightLeft, eMaterialType::STTLightRight});
			// taillights/ brakelights
			if (pControlVeh->m_fBreakPedal && pControlVeh->m_pDriver) {
				if (sttInstalled) {
					if (isLeftRearOk) {
						RenderLights(pControlVeh, pTowedVeh, eMaterialType::STTLightLeft, true, shdwName, shdwSz);
					}
					if (isRightRearOk) {
						RenderLights(pControlVeh, pTowedVeh, eMaterialType::STTLightRight, true, shdwName, shdwSz);
					}
				} else {
					if (IsMatAvail(pTowedVeh, {eMaterialType::BrakeLightLeft, eMaterialType::BrakeLightRight})) {
						if (isLeftRearOk) {
							RenderLights(pControlVeh, pTowedVeh, eMaterialType::BrakeLightLeft, true, shdwName, shdwSz);
						}
						if (isRightRearOk) {
							RenderLights(pControlVeh, pTowedVeh, eMaterialType::BrakeLightRight, true, shdwName, shdwSz);
						}
					}
					else if (IsMatAvail(pTowedVeh, {eMaterialType::TailLightLeft, eMaterialType::TailLightRight})) {
						if (isLeftRearOk) {
							RenderLights(pControlVeh, pTowedVeh, eMaterialType::TailLightLeft, true, shdwName, shdwSz);
						}
						if (isRightRearOk) {
							RenderLights(pControlVeh, pTowedVeh, eMaterialType::TailLightRight, true, shdwName, shdwSz);
						}
					}
				}

				if (indState != eIndicatorState::BothOn) {
					if (indState != eIndicatorState::LeftOn && isLeftRearOk) {
						RenderLights(pControlVeh, pTowedVeh, eMaterialType::NABrakeLightLeft, true, shdwName, shdwSz);
					}

					if (indState != eIndicatorState::RightOn && isRightRearOk) {
						RenderLights(pControlVeh, pTowedVeh, eMaterialType::NABrakeLightRight, true, shdwName, shdwSz);
					}
				}
			}

			bool indicatorOn = data.m_bUsingGlobalIndicators && data.m_nIndicatorState != eIndicatorState::Off;
			bool tailLightFlag = (Util::IsNightTime() || CarUtil::IsLightsForcedOn(pControlVeh)) && !CarUtil::IsLightsForcedOff(pControlVeh);
			if (tailLightFlag || indicatorOn) {
				if (sttInstalled) {
					if (isLeftRearOk) {
						RenderLights(pControlVeh, pTowedVeh, eMaterialType::STTLightLeft, true, shdwName, shdwSz);
					}

					if (isRightRearOk) {
						RenderLights(pControlVeh, pTowedVeh, eMaterialType::STTLightRight, true, shdwName, shdwSz);
					}
				}
				else {
					auto tailLightsRender = [&](bool leftOk, bool rightOk) {
						if (IsMatAvail(pTowedVeh, {eMaterialType::TailLightLeft, eMaterialType::TailLightRight}) || IsDummyAvail(pTowedVeh, {eMaterialType::TailLightLeft, eMaterialType::TailLightRight})) {
							if (leftOk) {
								RenderLights(pControlVeh, pTowedVeh, eMaterialType::TailLightLeft, true, shdwName, shdwSz);
							}
							if (rightOk) {
								RenderLights(pControlVeh, pTowedVeh, eMaterialType::TailLightRight, true, shdwName, shdwSz);
							}
						} else if (IsMatAvail(pTowedVeh, {eMaterialType::BrakeLightLeft, eMaterialType::BrakeLightRight}) || IsDummyAvail(pTowedVeh, {eMaterialType::BrakeLightLeft, eMaterialType::BrakeLightRight})) {
							if (leftOk) {
								RenderLights(pControlVeh, pTowedVeh, eMaterialType::BrakeLightLeft, true, shdwName, shdwSz);
							}
							if (rightOk) {
								RenderLights(pControlVeh, pTowedVeh, eMaterialType::BrakeLightRight, true, shdwName, shdwSz);
							}
						}
					};

					if (indicatorOn) {
						if (data.m_nIndicatorState == eIndicatorState::BothOn) {
							tailLightsRender(isLeftRearOk && !indicatorsDelay, isRightRearOk && !indicatorsDelay);
						}

						if (data.m_nIndicatorState == eIndicatorState::LeftOn) {
							tailLightsRender(isLeftRearOk && !indicatorsDelay, isRightRearOk && tailLightFlag);
						}

						if (data.m_nIndicatorState == eIndicatorState::RightOn) {
							tailLightsRender(isLeftRearOk && tailLightFlag, isRightRearOk && !indicatorsDelay);
						}
					} else {
						tailLightsRender(isLeftRearOk, isRightRearOk);
					}
				}
			}
		}

			// Indicator Lights
			if (!gbGlobalIndicatorLights && !IsMatAvail(pControlVeh, INDICATOR_LIGHTS_TYPE))
			{
				return;
			}

			static bool bSAMP = GetModuleHandle("SAMP.asi") || GetModuleHandle("SAMP.dll");

			if (pControlVeh->m_pDriver == FindPlayerPed() &&
				(pControlVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pControlVeh->m_nVehicleSubClass == VEHICLE_BIKE || pControlVeh->m_nVehicleSubClass == VEHICLE_QUAD || pControlVeh->m_nVehicleSubClass == VEHICLE_MTRUCK))
			{
				static uint32_t indicatorNoneKey = gConfig.ReadInteger("KEYS", "IndicatorLightNoneKey", VK_SHIFT);
				static uint32_t indicatorLeftKey = gConfig.ReadInteger("KEYS", "IndicatorLightLeftKey", VK_Z);
				static uint32_t indicatorRightKey = gConfig.ReadInteger("KEYS", "IndicatorLightRightKey", VK_C);
				static uint32_t indicatorBothKey = gConfig.ReadInteger("KEYS", "IndicatorLightBothKey", VK_X);

				if (KeyPressed(indicatorNoneKey))
				{
					data.m_nIndicatorState = eIndicatorState::Off;
					delay = 0;
					indicatorsDelay = false;
				}

				if (KeyPressed(indicatorLeftKey))
				{
					data.m_nIndicatorState = eIndicatorState::LeftOn;
				}

				if (KeyPressed(indicatorRightKey))
				{
					data.m_nIndicatorState = eIndicatorState::RightOn;
				}

				if (KeyPressed(indicatorBothKey))
				{
					data.m_nIndicatorState = eIndicatorState::BothOn;
				}
			} else if (pControlVeh->m_pDriver && !bSAMP) {
				data.m_nIndicatorState = eIndicatorState::Off;
				CVector2D prevPoint = GetCarPathLinkPosition(pControlVeh->m_autoPilot.m_nPreviousPathNodeInfo);
				CVector2D currPoint = GetCarPathLinkPosition(pControlVeh->m_autoPilot.m_nCurrentPathNodeInfo);
				CVector2D nextPoint = GetCarPathLinkPosition(pControlVeh->m_autoPilot.m_nNextPathNodeInfo);

				float angle = GetZAngleForPoint(nextPoint - currPoint) - GetZAngleForPoint(currPoint - prevPoint);
				angle = Util::NormalizeAngle(angle);

				if (angle >= 30.0f && angle < 180.0f) {
					data.m_nIndicatorState = eIndicatorState::LeftOn;
				}
				else if (angle <= 330.0f && angle > 180.0f) {
					data.m_nIndicatorState = eIndicatorState::RightOn;
				}

				if (data.m_nIndicatorState == eIndicatorState::Off) {
					if (pControlVeh->m_autoPilot.m_nCurrentLane == 0 && pControlVeh->m_autoPilot.m_nNextLane == 1) {
						data.m_nIndicatorState = eIndicatorState::RightOn;
					}
					else if (pControlVeh->m_autoPilot.m_nCurrentLane == 1 && pControlVeh->m_autoPilot.m_nNextLane == 0) {
						data.m_nIndicatorState = eIndicatorState::LeftOn;
					}
				}
			}

			if (!indicatorsDelay || indState == eIndicatorState::Off)
			{
				return;
			}

			// global turn lights
			if (gbGlobalIndicatorLights && !IsMatAvail(pControlVeh, INDICATOR_LIGHTS_TYPE) && !IsMatAvail(pControlVeh, {eMaterialType::STTLightLeft, eMaterialType::STTLightRight}))
			{
				if ((pControlVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pControlVeh->m_nVehicleSubClass == VEHICLE_BIKE || pControlVeh->m_nVehicleSubClass == VEHICLE_QUAD) &&
					(pControlVeh->GetVehicleAppearance() == VEHICLE_APPEARANCE_AUTOMOBILE || pControlVeh->GetVehicleAppearance() == VEHICLE_APPEARANCE_BIKE) &&
					pControlVeh->bEngineOn && pControlVeh->m_fHealth > 0 && !pControlVeh->bIsDrowning && !pControlVeh->m_pAttachedTo)
				{
					data.m_bUsingGlobalIndicators = true;
				}
			} else {
				if (indState == eIndicatorState::BothOn || indState == eIndicatorState::LeftOn) {
					RenderLights(pControlVeh, pTowedVeh, eMaterialType::IndicatorLightLeftFront, true, "indicator", 1.0f, false, isLeftFrontOk);
					RenderLights(pControlVeh, pTowedVeh, eMaterialType::IndicatorLightLeftMiddle, true, "indicator", 1.0f);
					RenderLights(pControlVeh, pTowedVeh, eMaterialType::IndicatorLightLeftRear, true, "indicator", 1.0f, false, isLeftRearOk);
					if (isLeftRearOk) {
						RenderLights(pControlVeh, pTowedVeh, eMaterialType::STTLightLeft, true, shdwName, shdwSz, true);
					}
				}

				if (indState == eIndicatorState::BothOn || indState == eIndicatorState::RightOn) {
					RenderLights(pControlVeh, pTowedVeh, eMaterialType::IndicatorLightRightFront, true, "indicator", 1.0f, false, isRightFrontOk);
					RenderLights(pControlVeh, pTowedVeh, eMaterialType::IndicatorLightRightMiddle, true, "indicator", 1.0f);
					RenderLights(pControlVeh, pTowedVeh, eMaterialType::IndicatorLightRightRear, true, "indicator", 1.0f, false, isRightRearOk);
					if (isRightRearOk) {
						RenderLights(pControlVeh, pTowedVeh, eMaterialType::STTLightRight, true, shdwName, shdwSz, true, isRightRearOk);
					}
				}
			}
			if (indState == eIndicatorState::BothOn || indState == eIndicatorState::LeftOn) {
				if (isLeftRearOk) {
					RenderLights(pControlVeh, pTowedVeh, eMaterialType::NABrakeLightLeft, true, "indicator", 1.0f);
				}
			}

			if (indState == eIndicatorState::BothOn || indState == eIndicatorState::RightOn) {
				if (isRightRearOk) {
					RenderLights(pControlVeh, pTowedVeh, eMaterialType::NABrakeLightRight, true, "indicator", 1.0f);
				}
			} });
};

void Lights::RenderLight(CVehicle *pVeh, eMaterialType state, bool shadows, std::string texture, float sz, bool highlight, bool isDummyOk)
{
	int id = static_cast<int>(state) * 1000;
	bool litMats = true;
	if (IsDummyAvail(pVeh, state))
	{
		for (auto e : m_Dummies[pVeh][state])
		{
			const VehicleDummyConfig &c = e->GetRef();
			e->Update();
			RwFrame *parent = RwFrameGetParent(e->Get().frame);
			eMaterialType type = e->GetRef().lightType;
			bool atomicCheck = type != eMaterialType::HeadLightLeft && type != eMaterialType::HeadLightRight && !FrameUtil::IsOkAtomicVisible(parent);

			if (atomicCheck || (c.dummyType == eDummyPos::Rear && pVeh->m_pTrailer) || (!c.isParentDummy && !isDummyOk))
			{
				litMats = false;
				break;
			}

			if (state == eMaterialType::StrobeLight)
			{
				size_t timer = CTimer::m_snTimeInMilliseconds;
				if (timer - c.strobe.timer > c.strobe.delay)
				{
					e->Get().strobe.enabled = !c.strobe.enabled;
					e->Get().strobe.timer = timer;
				}

				if (c.strobe.enabled)
				{
					ModelInfoMgr::EnableStrobeMaterial(pVeh, c.dummyIdx);
				}
				else
				{
					continue;
				}
			}
			EnableDummy((int)pVeh + 42 + id++, e, pVeh, highlight ? 3.00f : 1.0f);

			// Skip front shadows on bike wheelie
			if (c.dummyType == eDummyPos::Front && Util::IsVehicleDoingWheelie(pVeh))
			{
				continue;
			}

			if (shadows && c.shadow.render)
			{
				texture = (c.shadow.texture == "") ? texture : c.shadow.texture;
				RenderUtil::RegisterShadowDirectional(&e->Get(), texture, sz * c.shadow.size);
			}
		}
	}

	if (litMats)
	{
		ModelInfoMgr::EnableMaterial(pVeh, state);
	}
}

void Lights::RenderLights(CVehicle *pControlVeh, CVehicle *pTowedVeh, eMaterialType state, bool shadows, std::string texture, float sz, bool highlight, bool isDummyOk)
{
	int model = pControlVeh->m_nModelIndex;
	// if (CModelInfo::IsHeliModel(model) || CModelInfo::IsPlaneModel(model)) {
	// 	sz = 1.0f;
	// 	texture = "pointlight";
	// }

	if (GetLightState(pControlVeh, state))
	{
		RenderLight(pControlVeh, state, shadows, texture, sz, highlight, isDummyOk);
	}

	if (pControlVeh != pTowedVeh && GetLightState(pTowedVeh, state))
	{
		RenderLight(pTowedVeh, state, shadows, texture, sz, highlight, isDummyOk);
	}
}

void Lights::RenderHeadlights(CVehicle *pControlVeh, bool isLeftOn, bool isRightOn, bool realTime)
{
	CVehicle *pTowedVeh = pControlVeh;
	VehLightData &data = m_VehData.Get(pControlVeh);

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
		std::string texName = data.m_bLongLightsOn ? "headlight_long" : "headlight_short";

		if (isLeftOn || isRightOn)
		{
			if (isLeftOn && GetLightState(pControlVeh, eMaterialType::HeadLightLeft))
			{
				RenderLights(pControlVeh, pTowedVeh, eMaterialType::HeadLightLeft, true, texName, headlightSz, isFoggy || data.m_bLongLightsOn);
			}
			if (isRightOn && GetLightState(pControlVeh, eMaterialType::HeadLightRight))
			{
				RenderLights(pControlVeh, pTowedVeh, eMaterialType::HeadLightRight, true, texName, headlightSz, isFoggy || data.m_bLongLightsOn);
			}
		}
	}
}

void Lights::EnableDummy(int id, VehicleDummy *dummy, CVehicle *pVeh, float szMul)
{
	if (gConfig.ReadBoolean("VEHICLE_FEATURES", "LightCoronas", false))
	{
		const VehicleDummyConfig &c = dummy->GetRef();
		if (c.corona.lightingType == eLightingMode::NonDirectional)
		{
			RenderUtil::RegisterCorona(pVeh, (reinterpret_cast<unsigned int>(pVeh) * 255) + 255 + id, c.position, c.corona.color, c.corona.size * szMul);
		}
		else
		{
			RenderUtil::RegisterCoronaDirectional(&dummy->Get(), c.rotation.angle, 180.0f, szMul, c.corona.lightingType == eLightingMode::Inversed, false);
		}
	}
}

void Lights::Reload(CVehicle *pVeh)
{
	m_Dummies.erase(pVeh);
	DataMgr::Reload(pVeh->m_nModelIndex);
}

bool Lights::IsDummyAvail(CVehicle *pVeh, eMaterialType state)
{
	return m_Dummies[pVeh][state].size() != 0;
}

bool Lights::IsDummyAvail(CVehicle *pVeh, std::initializer_list<eMaterialType> states)
{
	for (eMaterialType state : states)
	{
		if (IsDummyAvail(pVeh, state))
		{
			return true;
		}
	}
	return false;
}

bool Lights::IsMatAvail(CVehicle *pVeh, eMaterialType type)
{
	return ModelInfoMgr::IsMaterialAvailable(pVeh, type);
}

bool Lights::IsMatAvail(CVehicle *pVeh, std::initializer_list<eMaterialType> states)
{
	for (eMaterialType type : states)
	{
		if (IsMatAvail(pVeh, type))
		{
			return true;
		}
	}
	return false;
}

bool Lights::IsIndicatorOn(CVehicle *pVeh)
{
	return !Util::IsEngineOff(pVeh) && (pVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pVeh->m_nVehicleSubClass == VEHICLE_BIKE) && indicatorsDelay && m_VehData.Get(pVeh).m_nIndicatorState != eIndicatorState::Off;
}

VehLightData Lights::GetVehicleData(CVehicle *pVeh)
{
	return m_VehData.Get(pVeh);
}

bool Lights::GetLightState(CVehicle *pVeh, eMaterialType lightId)
{
	return m_VehData.Get(pVeh).m_bLightStates[lightId];
}

void Lights::SetLightState(CVehicle *pVeh, eMaterialType lightId, bool state)
{
	m_VehData.Get(pVeh).m_bLightStates[lightId] = state;
}

extern enum ME_MaterialID;

extern "C"
{
	bool ME_GetVehicleLightState(CVehicle *pVeh, ME_MaterialID lightId)
	{
		return Lights::GetLightState(pVeh, static_cast<eMaterialType>(lightId));
	}

	void ME_SetVehicleLightState(CVehicle *pVeh, ME_MaterialID lightId, bool state)
	{
		Lights::SetLightState(pVeh, static_cast<eMaterialType>(lightId), state);
	}

	// Dummy function to show on crash logs
	int __declspec(dllexport) ignore4(int i)
	{
		return 1;
	}
}