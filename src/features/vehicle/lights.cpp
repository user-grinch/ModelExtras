#include "pch.h"
#include "lights.h"
#include <CClock.h>
#include "defines.h"
#include <CShadows.h>
#include <eVehicleClass.h>
#include <rwcore.h>
#include <rpworld.h>
#include "spotlights.h"
#include "../audiomgr.h"
#include <CWeather.h>
#include <CCoronas.h>
#include "../../enums/vehdummy.h"
#include "datamgr.h"
#include "core/colors.h"
#include <CPointLights.h>

// flags
bool gbGlobalIndicatorLights = false;
float gfGlobalCoronaSize = 0.3f;
int gGlobalCoronaIntensity = 80;
int gGlobalShadowIntensity = 80;
float headlightSz = 5.0f;

int GetStrobeIndex(CVehicle *pVeh, RpMaterial *pMat) {
	return pMat->color.blue;
}

// Indicator lights
static uint64_t delay;

CVector2D GetCarPathLinkPosition(CCarPathLinkAddress &address) {
	if (address.m_nAreaId != -1 && address.m_nCarPathLinkId != -1 && ThePaths.m_pPathNodes[address.m_nAreaId]) {
		return CVector2D(static_cast<float>(ThePaths.m_pNaviNodes[address.m_nAreaId][address.m_nCarPathLinkId].m_vecPosn.x) / 8.0f,
						 static_cast<float>(ThePaths.m_pNaviNodes[address.m_nAreaId][address.m_nCarPathLinkId].m_vecPosn.y) / 8.0f);
	}
	return CVector2D(0.0f, 0.0f);
}

inline float GetZAngleForPoint(CVector2D const &point) {
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

	plugin::Events::initGameEvent += []()
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

	ModelInfoMgr::RegisterMaterial([](CVehicle *pVeh, RpMaterial *pMat){
		if (!m_bEnabled) {
			return eLightType::UnknownLight;
		}
		// Headlights
		CRGBA matCol = *reinterpret_cast<CRGBA *>(RpMaterialGetColor(pMat));
		matCol.a = 255;
		if (matCol == VEHCOL_HEADLIGHT_LEFT) {
			return eLightType::HeadLightLeft;
		} else if (matCol == VEHCOL_HEADLIGHT_RIGHT) {
			return eLightType::HeadLightRight;
		}
		// Taillights
		else if (matCol == VEHCOL_TAILLIGHT_LEFT) {
			return eLightType::TailLightLeft;
		} else if (matCol == VEHCOL_TAILLIGHT_RIGHT) {
			return eLightType::TailLightRight;
		}
		// Reverse Lights
		else if (matCol == VEHCOL_REVERSELIGHT_LEFT) {
			return eLightType::ReverseLightLeft;
		}
		else if (matCol == VEHCOL_REVERSELIGHT_RIGHT) {
			return eLightType::ReverseLightRight;
		}
		// Brake Lights
		else if (matCol == VEHCOL_BRAKELIGHT_LEFT) {
			return eLightType::BrakeLightLeft;
		}
		else if (matCol == VEHCOL_BRAKELIGHT_RIGHT) {
			return eLightType::BrakeLightRight;
		}
		// All Day Lights
		else if (matCol == VEHCOL_ALLDAYLIGHT_1 || matCol == VEHCOL_ALLDAYLIGHT_2) {
			return eLightType::AllDayLight;
		}
		// Day Lights
		else if (matCol == VEHCOL_DAYLIGHT_1 || matCol == VEHCOL_DAYLIGHT_2) {
			return eLightType::DayLight;
		}
		// Night Lights
		else if (matCol == VEHCOL_NIGHTLIGHT_1 || matCol == VEHCOL_NIGHTLIGHT_2) {
			return eLightType::NightLight;
		}
		// Fog Lights
		else if (matCol == VEHCOL_FOGLIGHT_LEFT) {
			return eLightType::FogLightLeft;
		}
		else if (matCol == VEHCOL_FOGLIGHT_RIGHT) {
			return eLightType::FogLightRight;
		}
		// Sidelights
		else if (matCol == VEHCOL_SIDELIGHT_LEFT) {
			return eLightType::SideLightLeft;
		} else if (matCol == VEHCOL_SIDELIGHT_RIGHT) {
			return eLightType::SideLightRight;
		}
		// STT Lights
		else if (matCol == VEHCOL_STTLIGHT_LEFT) {
			return eLightType::STTLightLeft;
		} else if (matCol == VEHCOL_STTLIGHT_RIGHT) {
			return eLightType::STTLightRight;
		}
		// NA Brake Lights
		else if (matCol == VEHCOL_NABRAKE_LEFT) {
			return eLightType::NABrakeLightLeft;
		} else if (matCol == VEHCOL_NABRAKE_RIGHT) {
			return eLightType::NABrakeLightRight;
		}
		// Spot and Strobe Lights
		else if (matCol == VEHCOL_SPOTLIGHT) {
			return eLightType::SpotLight;
		} else if (matCol == VEHCOL_STROBELIGHT) {
			return eLightType::StrobeLight;
		}
		// Indicator Lights (Left)
		if (matCol == VEHCOL_INDICATOR_LEFT_REAR) {
			return eLightType::IndicatorLightLeftRear;
		}
		else if (matCol == VEHCOL_INDICATOR_LEFT_SIDE) {
			return eLightType::IndicatorLightLeftMiddle;
		}
		else if (matCol == VEHCOL_INDICATOR_LEFT_FRONT) {
			return eLightType::IndicatorLightLeftFront;
		}
		// Indicator Lights (Right)
		else if (matCol == VEHCOL_INDICATOR_RIGHT_REAR) {
			return eLightType::IndicatorLightRightRear;
		}
		else if (matCol == VEHCOL_INDICATOR_RIGHT_SIDE) {
			return eLightType::IndicatorLightRightMiddle;
		}
		else if (matCol == VEHCOL_INDICATOR_RIGHT_FRONT) {
			return eLightType::IndicatorLightRightFront;
		}
		// If no match is found
		return eLightType::UnknownLight;
	});

	ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *frame) {
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
			c.lightType = STR_FOUND(name, "_l") ? eLightType::FogLightLeft : eLightType::FogLightRight;
			c.corona.color = c.shadow.color = {255, 255, 255, static_cast<unsigned char>(gGlobalCoronaIntensity)};
			c.corona.lightingType = eLightingMode::Directional;
		}
		else if (name.starts_with("rev") && (STR_FOUND(name, "_l") || STR_FOUND(name, "_r"))) {
			c.dummyType = eDummyPos::Rear;
			c.lightType = STR_FOUND(name, "_l") ? eLightType::ReverseLightLeft : eLightType::ReverseLightRight;
			c.corona.color = c.shadow.color = {255, 255, 255, static_cast<unsigned char>(gGlobalCoronaIntensity)};
			c.corona.lightingType = eLightingMode::Directional;
		}
		else if (name.starts_with("breakl") && (STR_FOUND(name, "_l") || STR_FOUND(name, "_r"))) {
			c.dummyType = eDummyPos::Rear;
			c.lightType = STR_FOUND(name, "_l") ? eLightType::BrakeLightLeft : eLightType::BrakeLightRight;
			c.corona.color = c.shadow.color = {240, 0, 0, static_cast<unsigned char>(gGlobalCoronaIntensity)};
			c.corona.lightingType = eLightingMode::Directional;
		}
		else if (name.starts_with("light_d")) {
			c.lightType = eLightType::DayLight;
			c.dummyType = eDummyPos::Front;
		}
		else if (name.starts_with("light_n")) {
			c.lightType = eLightType::NightLight;
			c.dummyType = eDummyPos::Front;
		}
		else if (auto d = Util::GetDigitsAfter(name, "strobe_light")) {
			c.lightType = eLightType::StrobeLight;
			c.dummyType = eDummyPos::Front;
			c.dummyIdx = d.value();
		}
		else if (auto d = Util::GetCharsAfterPrefix(name, "sidelight_", 1)) {
			if (d == "L") {
				c.lightType = eLightType::SideLightLeft;
				c.dummyType = eDummyPos::Left;
			} else {
				c.lightType = eLightType::SideLightRight;
				c.dummyType = eDummyPos::Right;
			}
		}
		else if (auto d = Util::GetCharsAfterPrefix(name, "sttlight_", 1)) {
			c.lightType = (d == "L") ? eLightType::STTLightLeft : eLightType::STTLightRight;
			c.dummyType = eDummyPos::Rear;
			c.corona.color = c.shadow.color = {240, 0, 0, static_cast<unsigned char>(gGlobalCoronaIntensity)};
			c.corona.lightingType = eLightingMode::Directional;
		}
		else if (auto d = Util::GetCharsAfterPrefix(name, "nabrakelight_", 1)) {
			c.lightType = (d == "L") ? eLightType::NABrakeLightLeft : eLightType::NABrakeLightRight;
			c.dummyType = eDummyPos::Rear;
			c.corona.color = c.shadow.color = {240, 0, 0, static_cast<unsigned char>(gGlobalCoronaIntensity)};
			c.corona.lightingType = eLightingMode::Directional;
		}
		else if (name.starts_with("spotlight_light")) {
			c.lightType = eLightType::SpotLight;
		}
		else if (name.starts_with("light_a")) {
			c.lightType = eLightType::AllDayLight;
			c.dummyType = eDummyPos::Front;
		}
		else if (name == "taillights" || name == "taillights2") {
			c.dummyType = eDummyPos::Rear;
			c.lightType = eLightType::TailLightRight;
			c.corona.color = c.shadow.color = {250, 0, 0, static_cast<unsigned char>(gGlobalCoronaIntensity)};
			c.corona.lightingType = eLightingMode::Directional; 				
			c.shadow.render = name != "taillights2";
			dummies[c.lightType].push_back(new VehicleDummy(c));
			c.mirroredX = true;
			c.lightType = eLightType::TailLightLeft;
		}
		else if (name == "headlights" || name == "headlights2") {
			c.dummyType = eDummyPos::Front;
			c.lightType = eLightType::HeadLightLeft;
			c.corona.color = c.shadow.color = {250, 250, 250, static_cast<unsigned char>(gGlobalCoronaIntensity)};
			c.corona.lightingType = eLightingMode::Directional;
			c.shadow.render = name != "headlights2";
			c.mirroredX = true;
			dummies[c.lightType].push_back(new VehicleDummy(c));
			c.mirroredX = false;
			c.lightType = eLightType::HeadLightRight;
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
						c.lightType = isLeft ? eLightType::IndicatorLightLeftFront : eLightType::IndicatorLightRightFront;
						c.dummyType = eDummyPos::Front;
						break;
					case 'R':
						c.lightType = isLeft ? eLightType::IndicatorLightLeftRear : eLightType::IndicatorLightRightRear;
						c.dummyType = eDummyPos::Rear;
						break;
					case 'M':
						c.lightType = isLeft ? eLightType::IndicatorLightLeftMiddle : eLightType::IndicatorLightRightMiddle;
						c.dummyType = isLeft ? eDummyPos::Right : eDummyPos::Left;
						break;
				}
			}
		}
		else {
			return;
		}

		dummies[c.lightType].push_back(new VehicleDummy(c));
	});


	Events::processScriptsEvent += []()
	{
		size_t timestamp = CTimer::m_snTimeInMilliseconds;
		if ((timestamp - delay) > 500) {
			delay = timestamp;
			indicatorsDelay = !indicatorsDelay;
		}

		CVehicle *pVeh = FindPlayerVehicle(-1, false);
		if (pVeh && pVeh->IsDriver(FindPlayerPed()) && pVeh->m_nOverrideLights != eLightOverride::ForceLightsOff && !Util::IsEngineOff(pVeh))
		{
			static size_t prev = 0;
			static uint32_t fogLightKey = gConfig.ReadInteger("KEYS", "FogLightKey", VK_J);
			if (KeyPressed(fogLightKey) && IsMatAvail(pVeh, {eLightType::FogLightLeft, eLightType::FogLightRight}))
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
			if (KeyPressed(longLightKey) && pVeh->bLightsOn)
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

	Events::processScriptsEvent += []() {
		for (CVehicle *pVeh : CPools::ms_pVehiclePool) {
			if (pVeh->m_pDriver == FindPlayerPed() 
			|| pVeh->m_nVehicleSubClass == VEHICLE_BMX
			|| pVeh->m_nVehicleSubClass == VEHICLE_BOAT 
			|| pVeh->m_nVehicleSubClass == VEHICLE_TRAILER || Util::IsEngineOff(pVeh)) {
				continue;
			}

			if (DistanceBetweenPoints(pVeh->GetPosition(), TheCamera.GetPosition()) < 50.0f || pVeh->GetIsOnScreen()) {
				bool isLeftFrontOk = !Util::IsLightDamaged(pVeh, eLights::LIGHT_FRONT_LEFT);
				bool isRightFrontOk = !Util::IsLightDamaged(pVeh, eLights::LIGHT_FRONT_RIGHT);
				bool isHeadlightLeftOn = pVeh->m_renderLights.m_bLeftFront && isLeftFrontOk;
				bool isHeadlightRightOn = pVeh->m_renderLights.m_bRightFront && isRightFrontOk;
				RenderHeadlights(pVeh, isHeadlightLeftOn, isHeadlightRightOn, false);
			}
		}
	};

	ModelInfoMgr::RegisterRender([](CVehicle *pControlVeh) {
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

		if (pControlVeh->m_nOverrideLights == eLightOverride::ForceLightsOff || pControlVeh->ms_forceVehicleLightsOff )
		{
			return;
		}

		if (pControlVeh->m_nOverrideLights == eLightOverride::ForceLightsOn) {
			pControlVeh->bLightsOn = true;
			pControlVeh->m_renderLights.m_bLeftFront = true;
			pControlVeh->m_renderLights.m_bRightFront = true;
			if (pTowedVeh) {
				pTowedVeh->bLightsOn = true;
				pTowedVeh->m_renderLights.m_bLeftRear = true;
				pTowedVeh->m_renderLights.m_bRightRear = true;
			}
		}

		VehLightData &data = m_VehData.Get(pControlVeh);
		eIndicatorState indState = data.m_nIndicatorState;

		// Fix for park car alarm lights
		if (pControlVeh->m_fHealth == 0 || (Util::IsEngineOff(pControlVeh) && pControlVeh->m_nOverrideLights != eLightOverride::ForceLightsOn)) {
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
		bool isHeadlightLeftOn = pControlVeh->m_renderLights.m_bLeftFront && isLeftFrontOk;
		bool isHeadlightRightOn = pControlVeh->m_renderLights.m_bRightFront && isRightFrontOk;

		RenderLights(pControlVeh, pTowedVeh, eLightType::AllDayLight);
		RenderLights(pControlVeh, pTowedVeh, eLightType::StrobeLight);
		RenderLights(pControlVeh, pTowedVeh, eLightType::SideLightLeft);
		RenderLights(pControlVeh, pTowedVeh, eLightType::SideLightRight);
		
		if (Util::IsNightTime()) {
			RenderLights(pControlVeh, pTowedVeh, eLightType::NightLight);
		} else {
			RenderLights(pControlVeh, pTowedVeh, eLightType::DayLight);
		}
		
		if (data.m_bFogLightsOn) {
			RenderLights(pControlVeh, pTowedVeh, eLightType::FogLightLeft, true, "foglight", 3.0f);
			RenderLights(pControlVeh, pTowedVeh, eLightType::FogLightRight, true, "foglight", 3.0f);
		}

		bool isBike = CModelInfo::IsBikeModel(pControlVeh->m_nModelIndex);

		if (pControlVeh->m_pDriver == FindPlayerPed()) {
			RenderHeadlights(pControlVeh, isHeadlightLeftOn, isHeadlightRightOn);
		}

		if (SpotLights::IsEnabled(pControlVeh)) {
			RenderLights(pControlVeh, pTowedVeh, eLightType::SpotLight, false);
		}

		std::string shdwName = (isBike ? "taillight_bike" : "taillight");
		float shdwSz = 2.0f;

		if (pControlVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pControlVeh->m_nVehicleSubClass == VEHICLE_MTRUCK
			|| pControlVeh->m_nVehicleSubClass == VEHICLE_QUAD || pControlVeh->m_nVehicleSubClass == VEHICLE_BIKE
			|| pControlVeh->m_nVehicleSubClass == VEHICLE_TRAILER
		)
		{
			bool isRevlightSupportedByModel = IsMatAvail(pTowedVeh, {eLightType::ReverseLightLeft, eLightType::ReverseLightRight});

			bool reverseLightsOn = !isBike && isRevlightSupportedByModel && pControlVeh->m_nCurrentGear == 0 && (Util::GetVehicleSpeed(pControlVeh) >= 0.001f) && pControlVeh->m_pDriver;
			if (reverseLightsOn) {
				if (isLeftRearOk) {
					RenderLights(pControlVeh, pTowedVeh, eLightType::ReverseLightLeft, true, shdwName, shdwSz);
				}
				if (isRightRearOk) {
					RenderLights(pControlVeh, pTowedVeh, eLightType::ReverseLightRight, true, shdwName, shdwSz);
				}
			}

			bool sttInstalled = IsMatAvail(pTowedVeh, {eLightType::STTLightLeft, eLightType::STTLightRight});
			// taillights/ brakelights
			if (pControlVeh->m_fBreakPedal && pControlVeh->m_pDriver) {
				if (sttInstalled) {
					if (isLeftRearOk) {
						RenderLights(pControlVeh, pTowedVeh, eLightType::STTLightLeft, true, shdwName, shdwSz);
					}
					if (isRightRearOk) {
						RenderLights(pControlVeh, pTowedVeh, eLightType::STTLightRight, true, shdwName, shdwSz);
					}
				} else {
					if (IsMatAvail(pTowedVeh, {eLightType::BrakeLightLeft, eLightType::BrakeLightRight})) {
						if (isLeftRearOk) {
							RenderLights(pControlVeh, pTowedVeh, eLightType::BrakeLightLeft, true, shdwName, shdwSz);
						}
						if (isRightRearOk) {
							RenderLights(pControlVeh, pTowedVeh, eLightType::BrakeLightRight, true, shdwName, shdwSz);
						}
					}
					else if (IsMatAvail(pTowedVeh, {eLightType::TailLightLeft, eLightType::TailLightRight})) {
						if (isLeftRearOk) {
							RenderLights(pControlVeh, pTowedVeh, eLightType::TailLightLeft, true, shdwName, shdwSz);
						}
						if (isRightRearOk) {
							RenderLights(pControlVeh, pTowedVeh, eLightType::TailLightRight, true, shdwName, shdwSz);
						}
					}
				}

				if (indState != eIndicatorState::BothOn) {
					if (indState != eIndicatorState::LeftOn && isLeftRearOk) {
						RenderLights(pControlVeh, pTowedVeh, eLightType::NABrakeLightLeft, true, shdwName, shdwSz);
					}

					if (indState != eIndicatorState::RightOn && isRightRearOk) {
						RenderLights(pControlVeh, pTowedVeh, eLightType::NABrakeLightRight, true, shdwName, shdwSz);
					}
				}
			}

			if (Util::IsNightTime() || pControlVeh->m_nOverrideLights == eLightOverride::ForceLightsOn || pControlVeh->bLightsOn) {
				if (sttInstalled) {
					if (isLeftRearOk) {
						RenderLights(pControlVeh, pTowedVeh, eLightType::STTLightLeft, true, shdwName, shdwSz);
					}

					if (isRightRearOk) {
						RenderLights(pControlVeh, pTowedVeh, eLightType::STTLightRight, true, shdwName, shdwSz);
					}
				}
				else {
					auto tailLightsRender = [&](bool leftOk, bool rightOk) {
						if (IsMatAvail(pTowedVeh, {eLightType::TailLightLeft, eLightType::TailLightRight})) {
							if (leftOk) {
								RenderLights(pControlVeh, pTowedVeh, eLightType::TailLightLeft, true, shdwName, shdwSz);
							}
							if (rightOk) {
								RenderLights(pControlVeh, pTowedVeh, eLightType::TailLightRight, true, shdwName, shdwSz);
							}
						} else if (IsMatAvail(pTowedVeh, {eLightType::BrakeLightLeft, eLightType::BrakeLightRight})) {
							if (leftOk) {
								RenderLights(pControlVeh, pTowedVeh, eLightType::BrakeLightLeft, true, shdwName, shdwSz);
							}
							if (rightOk) {
								RenderLights(pControlVeh, pTowedVeh, eLightType::BrakeLightRight, true, shdwName, shdwSz);
							}
						}
					};

					if (data.m_bUsingGlobalIndicators && data.m_nIndicatorState != eIndicatorState::Off) {
						if (data.m_nIndicatorState == eIndicatorState::BothOn) {
							tailLightsRender(isLeftRearOk && !indicatorsDelay, isRightRearOk && !indicatorsDelay);
						}

						if (data.m_nIndicatorState == eIndicatorState::LeftOn) {
							tailLightsRender(isLeftRearOk && !indicatorsDelay, isRightRearOk);
						}

						if (data.m_nIndicatorState == eIndicatorState::RightOn) {
							tailLightsRender(isLeftRearOk, isRightRearOk && !indicatorsDelay);
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
			} else if (pControlVeh->m_pDriver) {
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
			if (gbGlobalIndicatorLights && !IsMatAvail(pControlVeh, INDICATOR_LIGHTS_TYPE) && !IsMatAvail(pControlVeh, {eLightType::STTLightLeft, eLightType::STTLightRight}))
			{
				if ((pControlVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pControlVeh->m_nVehicleSubClass == VEHICLE_BIKE || pControlVeh->m_nVehicleSubClass == VEHICLE_QUAD) &&
					(pControlVeh->GetVehicleAppearance() == VEHICLE_APPEARANCE_AUTOMOBILE || pControlVeh->GetVehicleAppearance() == VEHICLE_APPEARANCE_BIKE) &&
					pControlVeh->bEngineOn && pControlVeh->m_fHealth > 0 && !pControlVeh->bIsDrowning && !pControlVeh->m_pAttachedTo)
				{
					data.m_bUsingGlobalIndicators = true;
				}
			} else {
				if (indState == eIndicatorState::BothOn || indState == eIndicatorState::LeftOn) {
					if (isLeftFrontOk) {
						RenderLights(pControlVeh, pTowedVeh, eLightType::IndicatorLightLeftFront, true, "indicator", 1.0f);
					}
					RenderLights(pControlVeh, pTowedVeh, eLightType::IndicatorLightLeftMiddle, true, "indicator", 1.0f);
					if (isLeftRearOk) {
						RenderLights(pControlVeh, pTowedVeh, eLightType::IndicatorLightLeftRear, true, "indicator", 1.0f);
						RenderLights(pControlVeh, pTowedVeh, eLightType::STTLightLeft, true, shdwName, shdwSz, true);
					}
				}

				if (indState == eIndicatorState::BothOn || indState == eIndicatorState::RightOn) {
					if (isRightFrontOk) {
						RenderLights(pControlVeh, pTowedVeh, eLightType::IndicatorLightRightFront, true, "indicator", 1.0f);
					}
					RenderLights(pControlVeh, pTowedVeh, eLightType::IndicatorLightRightMiddle, true, "indicator", 1.0f);
					if (isRightRearOk) {
						RenderLights(pControlVeh, pTowedVeh, eLightType::IndicatorLightRightRear, true, "indicator", 1.0f);
						RenderLights(pControlVeh, pTowedVeh, eLightType::STTLightRight, true, shdwName, shdwSz, true);
					}
				}
			}
			if (indState == eIndicatorState::BothOn || indState == eIndicatorState::LeftOn) {
				if (isLeftRearOk) {
					RenderLights(pControlVeh, pTowedVeh, eLightType::NABrakeLightLeft, true, "indicator", 1.0f);
				}
			}

			if (indState == eIndicatorState::BothOn || indState == eIndicatorState::RightOn) {
				if (isRightRearOk) {
					RenderLights(pControlVeh, pTowedVeh, eLightType::NABrakeLightRight, true, "indicator", 1.0f);
				}
			} 
		}
	);
};

void Lights::RenderLight(CVehicle *pVeh, eLightType state, bool shadows, std::string texture, float sz, bool highlight)
{
	int id = static_cast<int>(state) * 1000;
	bool litMats = true;
	if (IsDummyAvail(pVeh, state))
	{
		for (auto e : m_Dummies[pVeh][state])
		{
			const VehicleDummyConfig& c = e->GetRef();
			e->Update();
			RwFrame *parent = RwFrameGetParent(e->Get().frame);
			eLightType type = e->GetRef().lightType;
			bool atomicCheck = type != eLightType::HeadLightLeft 
								&& type != eLightType::HeadLightRight 
								&& !FrameUtil::IsOkAtomicVisible(parent);

			if (atomicCheck || (c.dummyType == eDummyPos::Rear && pVeh->m_pTrailer)) {
				litMats = false;
				break;
			}

			if (state == eLightType::StrobeLight)
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
				} else {
					continue;
				}
			}

			EnableDummy((int)pVeh + 42 + id++, e, pVeh, highlight ? 1.75f : 1.0f);

			// Skip front shadows on bike wheelie
			if ( c.dummyType == eDummyPos::Front && Util::IsVehicleDoingWheelie(pVeh)) {
				continue;
			}

			if (shadows && c.shadow.render)
			{
				texture = (c.shadow.texture == "") ? texture : c.shadow.texture;
				RenderUtil::RegisterShadowDirectional(&e->Get(), texture, sz * c.shadow.size);
			}
		}
	}

	if (litMats) {
		ModelInfoMgr::EnableLightMaterial(pVeh, state);
	}
}

void Lights::RenderLights(CVehicle *pControlVeh, CVehicle *pTowedVeh, eLightType state, bool shadows, std::string texture, float sz, bool highlight)
{	
	int model = pControlVeh->m_nModelIndex;
	if (CModelInfo::IsHeliModel(model) || CModelInfo::IsPlaneModel(model)) {
		sz = 1.0f;
		texture = "pointlight";
	}

	if (GetLightState(pControlVeh, state)) {
		RenderLight(pControlVeh, state, shadows, texture, sz, highlight);
	}

	if (pControlVeh != pTowedVeh && GetLightState(pTowedVeh, state)) {
		RenderLight(pTowedVeh, state, shadows, texture, sz, highlight);
	}
}

void Lights::RenderHeadlights(CVehicle *pControlVeh, bool isLeftOn, bool isRightOn, bool realTime) {
	CVehicle *pTowedVeh = pControlVeh;
	VehLightData &data = m_VehData.Get(pControlVeh);

	if (pControlVeh->m_pTrailer) {
		pTowedVeh = pControlVeh->m_pTrailer;
	}

	if (pControlVeh->bLightsOn) {
		bool isFoggy = (CWeather::NewWeatherType == WEATHER_FOGGY_SF || CWeather::NewWeatherType == WEATHER_SANDSTORM_DESERT || CWeather::OldWeatherType == WEATHER_FOGGY_SF || CWeather::OldWeatherType == WEATHER_SANDSTORM_DESERT);
		std::string texName = data.m_bLongLightsOn ? "headlight_long" : "headlight_short";
		
		if (isLeftOn || isRightOn) {
			if (isLeftOn && GetLightState(pControlVeh, eLightType::HeadLightLeft)) {
				RenderLights(pControlVeh, pTowedVeh, eLightType::HeadLightLeft, true, texName, headlightSz, isFoggy || data.m_bLongLightsOn);
			}
			if (isRightOn && GetLightState(pControlVeh, eLightType::HeadLightRight)) {
				RenderLights(pControlVeh, pTowedVeh, eLightType::HeadLightRight, true, texName, headlightSz, isFoggy || data.m_bLongLightsOn);
			}
		}
	}
}

void Lights::EnableDummy(int id, VehicleDummy *dummy, CVehicle *pVeh, float szMul)
{
	if (gConfig.ReadBoolean("VEHICLE_FEATURES", "LightCoronas", false))
	{
		const VehicleDummyConfig& c = dummy->GetRef();
		if (c.corona.lightingType == eLightingMode::NonDirectional)
		{
			RenderUtil::RegisterCorona(pVeh, (reinterpret_cast<unsigned int>(pVeh) * 255) + 255 + id, c.position, c.corona.color, c.corona.size * szMul);
		}
		else
		{
			RenderUtil::RegisterCoronaDirectional(&dummy->Get(), c.rotation.angle, 180.0f, szMul, true, c.corona.lightingType == eLightingMode::Inversed);
		}
	}
}

void Lights::Reload(CVehicle *pVeh)
{
	m_Dummies.erase(pVeh);
	DataMgr::Reload(pVeh->m_nModelIndex);
}

bool Lights::IsDummyAvail(CVehicle *pVeh, eLightType state)
{
	return m_Dummies[pVeh][state].size() != 0;
}

bool Lights::IsDummyAvail(CVehicle* pVeh, std::initializer_list<eLightType> states)
{
	for (eLightType state : states) {
		if (IsDummyAvail(pVeh, state)) {
			return true;
		}
	}
	return false;
}

bool Lights::IsMatAvail(CVehicle *pVeh, eLightType type)
{
	return ModelInfoMgr::IsMaterialAvailable(pVeh, type);
}

bool Lights::IsMatAvail(CVehicle* pVeh, std::initializer_list<eLightType> states)
{
	for (eLightType type : states) {
		if (IsMatAvail(pVeh, type)) {
			return true;
		}
	}
	return false;
}

bool Lights::IsIndicatorOn(CVehicle *pVeh) {
	return !Util::IsEngineOff(pVeh) && (pVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pVeh->m_nVehicleSubClass == VEHICLE_BIKE) && indicatorsDelay && m_VehData.Get(pVeh).m_nIndicatorState != eIndicatorState::Off && pVeh->m_nOverrideLights != eLightOverride::ForceLightsOff;
}


bool Lights::GetLightState(CVehicle *pVeh, eLightType lightId) {
	return m_VehData.Get(pVeh).m_bLightStates[lightId];
}

void Lights::SetLightState(CVehicle *pVeh, eLightType lightId, bool state) {
	m_VehData.Get(pVeh).m_bLightStates[lightId] = state;
}

extern enum ME_LightID;

extern "C" {
	bool ME_GetVehicleLightState(CVehicle *pVeh, ME_LightID lightId) {
		return Lights::GetLightState(pVeh, static_cast<eLightType>(lightId));
	}
    
	void ME_SetVehicleLightState(CVehicle *pVeh, ME_LightID lightId, bool state) {
		Lights::SetLightState(pVeh, static_cast<eLightType>(lightId), state);
	}
}