#include "pch.h"
#include "lights.h"
#include <CClock.h>
#include "defines.h"
#include <CShadows.h>
#include <eVehicleClass.h>
#include <CCutsceneMgr.h>
#include <rwcore.h>
#include <rpworld.h>
#include "spotlights.h"
#include "../audiomgr.h"
#include <CWeather.h>
#include <CCoronas.h>
#include "../../enums/vehdummy.h"
#include "datamgr.h"
#include "core/colors.h"

// flags
bool gbGlobalIndicatorLights = false;
bool gbGlobalReverseLights = false;
float gfGlobalCoronaSize = 0.3f;
int gGlobalCoronaIntensity = 80;
int gGlobalShadowIntensity = 80;

bool IsNightTime()
{
	return CClock::GetIsTimeInRange(20, 6);
}

bool IsTailLightOn(CVehicle *pVeh)
{
	return IsNightTime() || pVeh->m_nOverrideLights == eLightOverride::ForceLightsOn || pVeh->m_nVehicleFlags.bLightsOn;
}

bool IsEngineOff(CVehicle *pVeh)
{
	return !pVeh->m_nVehicleFlags.bEngineOn || pVeh->m_nVehicleFlags.bEngineBroken;
}

unsigned char GetShadowAlphaForDayTime()
{

	if (IsNightTime())
	{
		return std::max(0, gGlobalShadowIntensity);
	}
	else
	{
		return std::max(0, gGlobalShadowIntensity - 15);
	}
}

unsigned char GetCoronaAlphaForDayTime()
{
	if (IsNightTime())
	{
		return std::max(0, gGlobalCoronaIntensity);
	}
	else
	{
		return std::max(0, gGlobalCoronaIntensity - 15);
	}
}

int GetStrobeIndex(CVehicle *pVeh, RpMaterial *pMat) {
	return pMat->color.blue;
}

// Indicator lights
static uint64_t delay;

CVector2D GetCarPathLinkPosition(CCarPathLinkAddress &address)
{
	if (address.m_nAreaId != -1 && address.m_nCarPathLinkId != -1 && ThePaths.m_pPathNodes[address.m_nAreaId])
	{
		return CVector2D(static_cast<float>(ThePaths.m_pNaviNodes[address.m_nAreaId][address.m_nCarPathLinkId].m_vecPosn.x) / 8.0f,
						 static_cast<float>(ThePaths.m_pNaviNodes[address.m_nAreaId][address.m_nCarPathLinkId].m_vecPosn.y) / 8.0f);
	}
	return CVector2D(0.0f, 0.0f);
}

void DrawGlobalLight(CVehicle *pVeh, bool isRear, bool isLeft, CRGBA col, std::string texture = "indicator",
					 CVector2D shdwSz = {1.0F, 1.0F}, CVector2D shdwOffset = {0.0F, 0.0F})
{
	if (pVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE)
	{
		CAutomobile *car = reinterpret_cast<CAutomobile *>(pVeh);
		bool broken = (isLeft && car->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_LEFT)) || (!isLeft && car->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_RIGHT));
		if (broken)
		{
			return;
		}
	}

	int dummyIdx = (isRear) ? 1 : 0;

	CVehicleModelInfo *pInfo = reinterpret_cast<CVehicleModelInfo *>(CModelInfo__ms_modelInfoPtrs[pVeh->m_nModelIndex]);
	CVector posn = pInfo->m_pVehicleStruct->m_avDummyPos[dummyIdx];

	if (posn.x == 0.0f)
	{
		posn.x = 0.15f;
	}

	if (isLeft)
	{
		posn.x *= -1.0f;
	}

	int dummyId = dummyIdx + (isLeft ? 0 : 2);
	float dummyAngle = isRear ? 180.0f : 0.0f;

	CRGBA shadowColor = {col.r, col.g, col.b, GetShadowAlphaForDayTime()};
	Util::RegisterShadow(pVeh, posn, shadowColor, dummyAngle, isRear ? eDummyPos::Rear : eDummyPos::Front, texture, shdwSz, shdwOffset);

	CRGBA coronaColor = {col.r, col.g, col.b, GetCoronaAlphaForDayTime()};
	int coronaId = reinterpret_cast<uintptr_t>(pVeh) + 255 * isRear + 128 * isLeft + col.r + col.g + col.b;
	Util::RegisterCoronaWithAngle(pVeh, coronaId, posn, coronaColor, dummyAngle, 180.0f, gfGlobalCoronaSize);
}

inline float GetZAngleForPoint(CVector2D const &point)
{
	float angle = CGeneral::GetATanOfXY(point.x, point.y) * 57.295776f - 90.0f;
	while (angle < 0.0f)
		angle += 360.0f;
	return angle;
}

inline bool IsBumperOrWingDamaged(CVehicle *pVeh, eParentType part)
{
	if (pVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE)
	{
		CAutomobile *ptr = reinterpret_cast<CAutomobile *>(pVeh);
		return ptr->m_damageManager.GetPanelStatus((int)part);
	}
	return false;
}

void Lights::Initialize()
{
	m_bEnabled = true;

	patch::Nop(0x6E2722, 19);	  // CVehicle::DoHeadLightReflection
	patch::SetUChar(0x6E1A22, 0); // CVehicle::DoTailLightEffect

	plugin::Events::initGameEvent += []()
	{
		gbGlobalIndicatorLights = gConfig.ReadBoolean("VEHICLE_FEATURES", "StandardLights_GlobalIndicatorLights", false);
		gbGlobalReverseLights = gConfig.ReadBoolean("VEHICLE_FEATURES", "StandardLights_GlobalReverseLights", false);
		gfGlobalCoronaSize = gConfig.ReadFloat("VEHICLE_FEATURES", "StandardLights_GlobalCoronaSize", 0.3f);
		gGlobalShadowIntensity = gConfig.ReadInteger("VEHICLE_FEATURES", "StandardLights_GlobalShadowIntensity", 220);
		gGlobalCoronaIntensity = gConfig.ReadInteger("VEHICLE_FEATURES", "StandardLights_GlobalCoronaIntensity", 250);
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
		else if (matCol == VEHCOL_REVERSELIGHT_LEFT || matCol == VEHCOL_REVERSELIGHT_RIGHT) {
			return eLightType::ReverseLight;
		}
		// Brake Lights
		else if (matCol == VEHCOL_BRAKELIGHT_LEFT || matCol == VEHCOL_BRAKELIGHT_RIGHT) {
			return eLightType::BrakeLight;
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
		else if (matCol == VEHCOL_FOGLIGHT_1 || matCol == VEHCOL_FOGLIGHT_2) {
			return eLightType::FogLight;
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
		else if (matCol == VEHCOL_INDICATOR_LEFT_REAR || matCol == VEHCOL_INDICATOR_LEFT_SIDE || matCol == VEHCOL_INDICATOR_LEFT_FRONT) {
			return eLightType::IndicatorLightLeft;
		}
		// Indicator Lights (Right)
		else if (matCol == VEHCOL_INDICATOR_RIGHT_REAR || matCol == VEHCOL_INDICATOR_RIGHT_SIDE || matCol == VEHCOL_INDICATOR_RIGHT_FRONT) {
			return eLightType::IndicatorLightRight;
		}

		// If no match is found
		return eLightType::UnknownLight;
	});
	ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *frame)
							   {
		std::string name = GetFrameNodeName(frame);
		eLightType state = eLightType::UnknownLight;
		RwRGBA col{255, 255, 255, GetCoronaAlphaForDayTime()};
		eDummyPos dummyPos = eDummyPos::None;
		int32_t dummyIdx = 0;
		bool directioanlByDef = false;
		if (name.starts_with("fogl") && (STR_FOUND(name, "_l") || STR_FOUND(name, "_r")))
		{
			state = eLightType::FogLight;
			dummyPos = eDummyPos::Front;
			directioanlByDef = true;
		}
		else if (name.starts_with("rev") && (STR_FOUND(name, "_l") || STR_FOUND(name, "_r")))
		{
			state = eLightType::ReverseLight;
			dummyPos = eDummyPos::Rear;
			directioanlByDef = true;
		}
		else if (name.starts_with("breakl") && (STR_FOUND(name, "_l") || STR_FOUND(name, "_r")))
		{
			state = eLightType::BrakeLight;
			dummyPos = eDummyPos::Rear;
			col = {240, 0, 0, GetCoronaAlphaForDayTime()};
			directioanlByDef = true;
		}
		else if (name.starts_with("light_day"))
		{
			state = eLightType::DayLight;
			dummyPos = eDummyPos::Front;
		}
		else if (name.starts_with("light_nigh"))
		{
			state = eLightType::NightLight;
			dummyPos = eDummyPos::Front;
		}
		else if (auto d = Util::GetDigitsAfter(name, "strobe_light"))
		{
			dummyIdx = d.value();
			dummyPos = eDummyPos::Front;
			state = eLightType::StrobeLight;
		}
		else if (auto d = Util::GetCharsAfterPrefix(name, "sidelight_", 1))
		{
			if (d.value() == "L")
			{
				state = eLightType::SideLightLeft;
				dummyPos = eDummyPos::Left;
			}
			else
			{
				state = eLightType::SideLightRight;
				dummyPos = eDummyPos::Right;
			}
		}
		else if (auto d = Util::GetCharsAfterPrefix(name, "sttlight_", 1))
		{
			if (d.value() == "L")
			{
				state = eLightType::STTLightLeft;
			}
			else
			{
				state = eLightType::STTLightRight;
			}
			dummyPos = eDummyPos::Rear;
			directioanlByDef = true;
			col = {240, 0, 0, GetCoronaAlphaForDayTime()};
		}
		else if (auto d = Util::GetCharsAfterPrefix(name, "nabrakelight_", 1))
		{
			if (d.value() == "L")
			{
				state = eLightType::NABrakeLightLeft;
			}
			else
			{
				state = eLightType::NABrakeLightRight;
			}
			dummyPos = eDummyPos::Rear;
			directioanlByDef = true;
			col = {240, 0, 0, GetCoronaAlphaForDayTime()};
		}
		else if (name.starts_with("spotlight_light"))
		{
			state = eLightType::SpotLight;
		}
		else if (name.starts_with("light_allday"))
		{
			state = eLightType::AllDayLight;
			dummyPos = eDummyPos::Front;
		}
		else if (name.starts_with("taillights"))
		{
			col = {250, 0, 0, GetCoronaAlphaForDayTime()};
			state = eLightType::TailLightLeft;
			dummyPos = eDummyPos::Rear;
			directioanlByDef = true;
			VehicleDummy *pDummy = new VehicleDummy(pVeh, frame, name, dummyPos, col, dummyIdx, directioanlByDef, true);
			m_Dummies[pVeh][state].push_back(pDummy);
			state = eLightType::TailLightRight;
		}
		else if (name.starts_with("headlights"))
		{
			col = {250, 250, 250, GetCoronaAlphaForDayTime()};
			state = eLightType::HeadLightLeft;
			dummyPos = eDummyPos::Front;
			directioanlByDef = true;
			VehicleDummy *pDummy = new VehicleDummy(pVeh, frame, name, dummyPos, col, dummyIdx, directioanlByDef, true);
			m_Dummies[pVeh][state].push_back(pDummy);
			state = eLightType::HeadLightRight;
		}
		else if (name.starts_with("turnl_") || name.starts_with("indicator_"))
		{
			auto d = Util::GetCharsAfterPrefix(name, "turnl_", 2);
			if (!d) {
				d = Util::GetCharsAfterPrefix(name, "indicator_", 2);
			}

			if (d) {
				state = (d.value()[0] == 'L') ? eLightType::IndicatorLightLeft : eLightType::IndicatorLightRight;
				col = {255, 128, 0, GetCoronaAlphaForDayTime()};

				switch (d.value()[1]) {
					case 'F':
						dummyPos = (state == eLightType::IndicatorLightRight) ? eDummyPos::Front : eDummyPos::Front;
						break;
					case 'R':
						dummyPos = (state == eLightType::IndicatorLightRight) ? eDummyPos::Rear : eDummyPos::Rear;
						break;
					case 'M':
						dummyPos = (state == eLightType::IndicatorLightRight) ? eDummyPos::Right : eDummyPos::Left;
						break;
				}
				directioanlByDef = true;
			}
		}
		else
		{
			return;
		}

		m_Dummies[pVeh][state].push_back(new VehicleDummy(pVeh, frame, name, dummyPos, col, dummyIdx, directioanlByDef)); });

	Events::processScriptsEvent += []()
	{
		size_t timestamp = CTimer::m_snTimeInMilliseconds;
		if ((timestamp - delay) > 500)
		{
			delay = timestamp;
			indicatorsDelay = !indicatorsDelay;
		}

		CVehicle *pVeh = FindPlayerVehicle(-1, false);
		if (pVeh && pVeh->m_nOverrideLights != eLightOverride::ForceLightsOff && !IsEngineOff(pVeh))
		{
			static size_t prev = 0;
			static uint32_t fogLightKey = gConfig.ReadInteger("KEYS", "FogLightKey", VK_J);
			if (KeyPressed(fogLightKey) && IsDummyAvail(pVeh, eLightType::FogLight))
			{
				size_t now = CTimer::m_snTimeInMilliseconds;
				if (now - prev > 500.0f)
				{
					VehData &data = m_VehData.Get(pVeh);
					data.m_bFogLightsOn = !data.m_bFogLightsOn;
					prev = now;
					AudioMgr::PlaySwitchSound(pVeh);
				}
			}

			static uint32_t longLightKey = gConfig.ReadInteger("KEYS", "LongLightKey", VK_G);
			if (KeyPressed(longLightKey) && pVeh->m_nVehicleFlags.bLightsOn)
			{
				size_t now = CTimer::m_snTimeInMilliseconds;
				if (now - prev > 500.0f)
				{
					VehData &data = m_VehData.Get(pVeh);
					data.m_bLongLightsOn = !data.m_bLongLightsOn;
					prev = now;
					AudioMgr::PlaySwitchSound(pVeh);
				}
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

		if (pControlVeh->m_nOverrideLights == eLightOverride::ForceLightsOff || pControlVeh->ms_forceVehicleLightsOff || (!pControlVeh->GetIsOnScreen() && !pTowedVeh->GetIsOnScreen()))
		{
			return;
		}

		if (pControlVeh->m_nOverrideLights == eLightOverride::ForceLightsOn) {
			pControlVeh->m_nVehicleFlags.bLightsOn = true;
			if (pControlVeh->m_pTrailer) {
				pControlVeh->m_pTrailer->m_nVehicleFlags.bLightsOn = true;
			}
		}

		VehData &data = m_VehData.Get(pControlVeh);
		eIndicatorState indState = data.m_nIndicatorState;

		if (pControlVeh->m_fHealth == 0 || IsEngineOff(pControlVeh))
		{
			return;
		}

		CAutomobile *pAutoMobile = reinterpret_cast<CAutomobile *>(pControlVeh);
		RenderLights(pControlVeh, pTowedVeh, eLightType::AllDayLight);
		RenderLights(pControlVeh, pTowedVeh, eLightType::StrobeLight);
		RenderLights(pControlVeh, pTowedVeh, eLightType::SideLightLeft);
		RenderLights(pControlVeh, pTowedVeh, eLightType::SideLightRight);
		
		if (IsNightTime())
		{
			RenderLights(pControlVeh, pTowedVeh, eLightType::NightLight);
		}
		else
		{
			RenderLights(pControlVeh, pTowedVeh, eLightType::DayLight);
		}
		
		CVector2D shdwOffset = {0.0f, 0.7f};
		CVector2D headlightOffset = {0.0f, shdwOffset.y + 0.5f};
		if (data.m_bFogLightsOn)
		{
			RenderLights(pControlVeh, pTowedVeh, eLightType::FogLight, true, "foglight", {3.0f, 7.0f}, shdwOffset);
		}

		bool isBike = CModelInfo::IsBikeModel(pControlVeh->m_nModelIndex);
		if (pControlVeh->m_nVehicleFlags.bLightsOn)
		{
			bool leftOk = true;
			bool rightOk = true;
			if (CModelInfo::IsCarModel(pControlVeh->m_nModelIndex))
			{
				leftOk = pAutoMobile->m_renderLights.m_bLeftFront && !pAutoMobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_LEFT);
				rightOk = pAutoMobile->m_renderLights.m_bRightFront && !pAutoMobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_RIGHT);
			}

			std::string texName = "headlight_short";
			CVector2D sz = {4.0f, 8.0f};

			bool isFoggy = (CWeather::NewWeatherType == WEATHER_FOGGY_SF || CWeather::NewWeatherType == WEATHER_SANDSTORM_DESERT || CWeather::OldWeatherType == WEATHER_FOGGY_SF || CWeather::OldWeatherType == WEATHER_SANDSTORM_DESERT);
			if (data.m_bLongLightsOn)
			{
				texName = "headlight_long";
			}

			if (leftOk)
			{
				RenderLights(pControlVeh, pTowedVeh, eLightType::HeadLightLeft, true, texName, sz, headlightOffset, isFoggy || data.m_bLongLightsOn);
			}

			if (rightOk)
			{
				RenderLights(pControlVeh, pTowedVeh, eLightType::HeadLightRight, true, texName, sz, headlightOffset, isFoggy || data.m_bLongLightsOn);
			}
		}

		if (SpotLights::IsEnabled(pControlVeh))
		{
			RenderLights(pControlVeh, pTowedVeh, eLightType::SpotLight, false);
		}


		std::string shdwName = (isBike ? "taillight_bike" : "taillight");

		CVector2D shdwSz = {1.0f, 1.5f};
		if (isBike || CModelInfo::IsCarModel(pControlVeh->m_nModelIndex))
		{
			bool isRevlightSupportedByModel = IsDummyAvail(pTowedVeh, eLightType::ReverseLight);

			bool reverseLightsOn = !isBike && (isRevlightSupportedByModel || gbGlobalReverseLights) && pControlVeh->m_nCurrentGear == 0 && (Util::GetVehicleSpeed(pControlVeh) >= 0.001f) && pControlVeh->m_pDriver;
			if (reverseLightsOn)
			{
				if (isRevlightSupportedByModel)
				{
					RenderLights(pControlVeh, pTowedVeh, eLightType::ReverseLight, true, shdwName, shdwSz, shdwOffset);
				}
				else
				{
					DrawGlobalLight(pTowedVeh, true, true, {240, 240, 240, 0}, shdwName, shdwSz, shdwOffset);
					DrawGlobalLight(pTowedVeh, true, false, {240, 240, 240, 0}, shdwName, shdwSz, shdwOffset);
				}
			}

			bool sttInstalled = IsDummyAvail(pTowedVeh, eLightType::STTLightLeft) || IsDummyAvail(pTowedVeh, eLightType::STTLightRight);
			// taillights/ brakelights
			if (pControlVeh->m_fBreakPedal && pControlVeh->m_pDriver)
			{
				if (sttInstalled) {
					RenderLights(pControlVeh, pTowedVeh, eLightType::STTLightLeft, true, shdwName, shdwSz, shdwOffset);
					RenderLights(pControlVeh, pTowedVeh, eLightType::STTLightRight, true, shdwName, shdwSz, shdwOffset);
				} else {
					if (IsDummyAvail(pTowedVeh, eLightType::BrakeLight))
					{
						RenderLights(pControlVeh, pTowedVeh, eLightType::BrakeLight, true, shdwName, shdwSz, shdwOffset);
					}
					else if (IsDummyAvail(pTowedVeh, eLightType::TailLightLeft))
					{
						RenderLights(pControlVeh, pTowedVeh, eLightType::TailLightLeft, true, shdwName, shdwSz, shdwOffset);
						RenderLights(pControlVeh, pTowedVeh, eLightType::TailLightRight, true, shdwName, shdwSz, shdwOffset);
					}
				}

				if (indState != eIndicatorState::BothOn)
				{
					if (indState != eIndicatorState::LeftOn)
					{
						RenderLights(pControlVeh, pTowedVeh, eLightType::NABrakeLightLeft, true, shdwName, shdwSz, shdwOffset);
					}

					if (indState != eIndicatorState::RightOn)
					{
						RenderLights(pControlVeh, pTowedVeh, eLightType::NABrakeLightRight, true, shdwName, shdwSz, shdwOffset);
					}
				}
			}

			if (IsTailLightOn(pControlVeh))
			{
				if (sttInstalled) {
					RenderLights(pControlVeh, pTowedVeh, eLightType::STTLightLeft, true, shdwName, shdwSz, shdwOffset);
					RenderLights(pControlVeh, pTowedVeh, eLightType::STTLightRight, true, shdwName, shdwSz, shdwOffset);
				}
				else {
					if (IsDummyAvail(pTowedVeh, eLightType::TailLightLeft))
					{
						RenderLights(pControlVeh, pTowedVeh, eLightType::TailLightLeft, !sttInstalled, shdwName, shdwSz, shdwOffset);
						RenderLights(pControlVeh, pTowedVeh, eLightType::TailLightRight, !sttInstalled, shdwName, shdwSz, shdwOffset);
					}
					else if (IsDummyAvail(pTowedVeh, eLightType::BrakeLight))
					{
						RenderLights(pControlVeh, pTowedVeh, eLightType::BrakeLight, !sttInstalled, shdwName, shdwSz, shdwOffset);
					}
				}
			}
		}

			// Indicator Lights
			if (!gbGlobalIndicatorLights && !IsDummyAvail(pControlVeh, eLightType::IndicatorLightLeft) && !IsDummyAvail(pControlVeh, eLightType::IndicatorLightLeft))
			{
				return;
			}

			if (CCutsceneMgr::ms_running || TheCamera.m_bWideScreenOn)
			{
				return;
			}

			if (pControlVeh->m_pDriver == FindPlayerPed() &&
				(pControlVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pControlVeh->m_nVehicleSubClass == VEHICLE_BIKE || pControlVeh->m_nVehicleSubClass == VEHICLE_QUAD))
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
			}
			else if (pControlVeh->m_pDriver)
			{
				data.m_nIndicatorState = eIndicatorState::Off;
				CVector2D prevPoint = GetCarPathLinkPosition(pControlVeh->m_autoPilot.m_nPreviousPathNodeInfo);
				CVector2D currPoint = GetCarPathLinkPosition(pControlVeh->m_autoPilot.m_nCurrentPathNodeInfo);
				CVector2D nextPoint = GetCarPathLinkPosition(pControlVeh->m_autoPilot.m_nNextPathNodeInfo);

				float angle = GetZAngleForPoint(nextPoint - currPoint) - GetZAngleForPoint(currPoint - prevPoint);
				while (angle < 0.0f)
					angle += 360.0f;
				while (angle > 360.0f)
					angle -= 360.0f;

				if (angle >= 30.0f && angle < 180.0f)
					data.m_nIndicatorState = eIndicatorState::LeftOn;
				else if (angle <= 330.0f && angle > 180.0f)
					data.m_nIndicatorState = eIndicatorState::RightOn;

				if (data.m_nIndicatorState == eIndicatorState::Off)
				{
					if (pControlVeh->m_autoPilot.m_nCurrentLane == 0 && pControlVeh->m_autoPilot.m_nNextLane == 1)
						data.m_nIndicatorState = eIndicatorState::RightOn;
					else if (pControlVeh->m_autoPilot.m_nCurrentLane == 1 && pControlVeh->m_autoPilot.m_nNextLane == 0)
						data.m_nIndicatorState = eIndicatorState::LeftOn;
				}
			}

			if (!indicatorsDelay || indState == eIndicatorState::Off)
			{
				return;
			}

			// global turn lights
			if (gbGlobalIndicatorLights &&
				((!IsDummyAvail(pControlVeh, eLightType::IndicatorLightLeft) && !IsDummyAvail(pControlVeh, eLightType::STTLightLeft)) || (!IsDummyAvail(pControlVeh, eLightType::IndicatorLightRight) && !IsDummyAvail(pControlVeh, eLightType::STTLightRight))))
			{
				if ((pControlVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pControlVeh->m_nVehicleSubClass == VEHICLE_BIKE || pControlVeh->m_nVehicleSubClass == VEHICLE_QUAD) &&
					(pControlVeh->GetVehicleAppearance() == VEHICLE_APPEARANCE_AUTOMOBILE || pControlVeh->GetVehicleAppearance() == VEHICLE_APPEARANCE_BIKE) &&
					pControlVeh->m_nVehicleFlags.bEngineOn && pControlVeh->m_fHealth > 0 && !pControlVeh->m_nVehicleFlags.bIsDrowning && !pControlVeh->m_pAttachedTo)
				{
					if (DistanceBetweenPoints(TheCamera.m_vecGameCamPos, pControlVeh->GetPosition()) < 150.0f)
					{
						if (indState == eIndicatorState::BothOn || indState == eIndicatorState::RightOn)
						{
							DrawGlobalLight(pControlVeh, false, false, {255, 128, 0, 0}, "indicator", {1.0f, 1.0f}, shdwOffset);
							DrawGlobalLight(pTowedVeh, true, false, {255, 128, 0, 0}, "indicator", {1.0f, 1.0f}, shdwOffset);
						}
						if (indState == eIndicatorState::BothOn || indState == eIndicatorState::LeftOn)
						{
							DrawGlobalLight(pControlVeh, false, true, {255, 128, 0, 0}, "indicator", {1.0f, 1.0f}, shdwOffset);
							DrawGlobalLight(pTowedVeh, true, true, {255, 128, 0, 0}, "indicator", {1.0f, 1.0f}, shdwOffset);
						}
					}
				}
			}
			else
			{
				if (indState == eIndicatorState::BothOn || indState == eIndicatorState::LeftOn)
				{
					RenderLights(pControlVeh, pTowedVeh, eLightType::IndicatorLightLeft, true, "indicator", {1.0f, 1.0f}, shdwOffset);
					RenderLights(pControlVeh, pTowedVeh, eLightType::STTLightLeft, true, shdwName, shdwSz, shdwOffset, true);
				}

				if (indState == eIndicatorState::BothOn || indState == eIndicatorState::RightOn)
				{
					RenderLights(pControlVeh, pTowedVeh, eLightType::IndicatorLightRight, true, "indicator", {1.0f, 1.0f}, shdwOffset);
					RenderLights(pControlVeh, pTowedVeh, eLightType::STTLightRight, true, shdwName, shdwSz, shdwOffset, true);
				}
			}
			if (indState == eIndicatorState::BothOn || indState == eIndicatorState::LeftOn)
			{
				RenderLights(pControlVeh, pTowedVeh, eLightType::NABrakeLightLeft, true, "indicator", {1.0f, 1.0f}, shdwOffset);
			}

			if (indState == eIndicatorState::BothOn || indState == eIndicatorState::RightOn)
			{
				RenderLights(pControlVeh, pTowedVeh, eLightType::NABrakeLightRight, true, "indicator", {1.0f, 1.0f}, shdwOffset);
			} });
};

void Lights::RenderLight(CVehicle *pVeh, eLightType state, bool shadows, std::string texture, CVector2D sz, CVector2D offset, bool highlight)
{
	bool FrontDisabled = false;
	bool RearDisabled = false;
	bool MidDisabled = false;
	int id = static_cast<int>(state) * 1000;

	if (IsDummyAvail(pVeh, state))
	{
		for (auto e : m_Dummies[pVeh][state])
		{
			if (e->PartType != eParentType::Unknown && IsBumperOrWingDamaged(pVeh, e->PartType))
			{
				if (e->DummyType == eDummyPos::Front)
				{
					FrontDisabled = true;
				}
				if ((e->DummyType == eDummyPos::Left || e->DummyType == eDummyPos::Right))
				{
					MidDisabled = true;
				}
				if (e->DummyType == eDummyPos::Rear)
				{
					RearDisabled = true;
				}
				continue;
			}

			if (e->DummyType == eDummyPos::Rear)
			{
				if (pVeh->m_pTrailer)
				{
					RearDisabled = true;
					continue;
				}
			}

			if (state == eLightType::StrobeLight)
			{
				size_t timer = CTimer::m_snTimeInMilliseconds;
				if (timer - e->strobeLightTimer > e->strobeDelay)
				{
					e->strobeLightOn = !e->strobeLightOn;
					e->strobeLightTimer = timer;
				}

				if (e->strobeLightOn)
				{
					ModelInfoMgr::EnableStrobeMaterial(pVeh, e->DummyIdx);
				} else {
					continue;
				}
			}

			EnableDummy((int)pVeh + 42 + id++, e, pVeh, highlight ? 1.5f : 1.0f);

			if (shadows)
			{
				texture = (e->shdwTex == "") ? texture : e->shdwTex;
				e->Update(pVeh);
				Util::RegisterShadow(pVeh, e->Position, e->shdwCol, e->Angle, e->DummyType, texture, {sz.x * e->shdowSize.x, sz.y * e->shdowSize.y}, {offset.x + e->shdwOffSet.x, offset.y + e->shdwOffSet.y});
			}
		}
	}

	ModelInfoMgr::EnableLightMaterial(pVeh, state);
}

void Lights::RenderLights(CVehicle *pControlVeh, CVehicle *pTowedVeh, eLightType state, bool shadows, std::string texture, CVector2D sz, CVector2D offset, bool highlight)
{	
	int model = pControlVeh->m_nModelIndex;
	if (CModelInfo::IsHeliModel(model) || CModelInfo::IsPlaneModel(model)) {
		offset = {0.0f, 0.0f};
		sz = {1.0f, 1.0f};
		texture = "pointlight";
	}

	RenderLight(pControlVeh, state, shadows, texture, sz, offset, highlight);

	if (pControlVeh != pTowedVeh)
	{
		RenderLight(pTowedVeh, state, shadows, texture, sz, offset, highlight);
	}
}

void Lights::EnableDummy(int id, VehicleDummy *dummy, CVehicle *pVeh, float szMul)
{
	if (gConfig.ReadBoolean("VEHICLE_FEATURES", "LightCoronas", false))
	{
		if (dummy->LightType == eLightingMode::NonDirectional)
		{
			Util::RegisterCorona(pVeh, (reinterpret_cast<unsigned int>(pVeh) * 255) + 255 + id, dummy->Position, dummy->coronaCol, dummy->coronaSize * szMul);
		}
		else
		{
			Util::RegisterCoronaWithAngle(pVeh, (reinterpret_cast<unsigned int>(pVeh) * 255) + 255 + id, dummy->Position, dummy->coronaCol, dummy->Angle + (dummy->LightType == eLightingMode::Inversed ? 180.0f : 0.0f), 180.0f, dummy->coronaSize * szMul);
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