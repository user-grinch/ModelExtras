#include "pch.h"
#include "lights.h"
#include <CClock.h>
#include "defines.h"
#include <CShadows.h>
#include <eVehicleClass.h>
#include <rwcore.h>
#include <rpworld.h>
#include "spotlights.h"
#include "utils/audiomgr.h"
#include <CWeather.h>
#include <CCoronas.h>
#include "enums/vehdummy.h"
#include "utils/datamgr.h"
#include "core/colors.h"
#include <CPointLights.h>
#include "ModelExtrasAPI.h"
#include "utils/meevents.h"
#include "lights/manager.h"

using namespace plugin;

// flags
bool gbGlobalIndicatorLights = false;
static bool gbLightCoronasFeature = false;
float gfGlobalCoronaSize = 0.3f;
int gGlobalCoronaIntensity = 80;
int gGlobalShadowIntensity = 80;
float gfTailLightCoronaSize = 0.8f;
int gTailLightCoronaIntensity = 60;
float headlightSz = 5.0f;

int GetStrobeIndex(CVehicle *pVeh, RpMaterial *pMat)
{
	return pMat->color.blue;
}

// Indicator lights
static uint64_t delay;

struct CarPathLinkAddress {
    unsigned short m_nCarPathLinkId : 10;
    unsigned short m_nAreaId : 6;

    constexpr static auto* Cast(CCarPathLinkAddress* oldFormat) {
        return (CarPathLinkAddress*)(oldFormat);
    }
    constexpr static const auto* Cast(const CCarPathLinkAddress* oldFormat) {
        return (const CarPathLinkAddress*)(oldFormat);
    }
};

CVector2D GetCarPathLinkPosition(CCarPathLinkAddress &address)
{
	auto* addr = CarPathLinkAddress::Cast(&address);
	if (ThePaths.m_pNaviNodes && addr->m_nAreaId < 64 && ThePaths.m_pNaviNodes[addr->m_nAreaId])
	{
		return CVector2D(static_cast<float>(ThePaths.m_pNaviNodes[addr->m_nAreaId][addr->m_nCarPathLinkId].m_vecPosn.x) / 8.0f,
						 static_cast<float>(ThePaths.m_pNaviNodes[addr->m_nAreaId][addr->m_nCarPathLinkId].m_vecPosn.y) / 8.0f);
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

bool gbLightPointLights = true;
bool gbSirenPointLights = true;

static uint32_t g_nFogLightKey = VK_J;
static uint32_t g_nLongLightKey = VK_G;
static uint32_t g_nIndicatorNoneKey = VK_SHIFT;
static uint32_t g_nIndicatorLeftKey = VK_Z;
static uint32_t g_nIndicatorRightKey = VK_C;
static uint32_t g_nIndicatorBothKey = VK_X;
static bool g_bFoglightTiedToHeadlight = true;
static bool g_bAutoIndicatorsOnSteer = false;

void Lights::InitConfig()
{
	gbGlobalIndicatorLights = gConfig.ReadBoolean("LIGHTS", "StandardLights_GlobalIndicatorLights", gConfig.ReadBoolean("FEATURES", "StandardLights_GlobalIndicatorLights", false));
	gbLightCoronasFeature = gConfig.ReadBoolean("LIGHTS", "LightCoronas", gConfig.ReadBoolean("FEATURES", "LightCoronas", false));
	gbLightPointLights = gConfig.ReadBoolean("LIGHTS", "PointLights", gConfig.ReadBoolean("LIGHTS", "LightPointLights", gConfig.ReadBoolean("FEATURES", "PointLights", true)));
	gbSirenPointLights = gConfig.ReadBoolean("LIGHTS", "SirenPointLights", gConfig.ReadBoolean("FEATURES", "SirenPointLights", true));

	gfGlobalCoronaSize = gConfig.ReadFloat("LIGHTS", "LightCoronaSize", gConfig.ReadFloat("VISUAL", "LightCoronaSize", 0.3f));
	gGlobalShadowIntensity = gConfig.ReadInteger("LIGHTS", "LightShadowIntensity", gConfig.ReadInteger("VISUAL", "LightShadowIntensity", 80));
	gGlobalCoronaIntensity = gConfig.ReadInteger("LIGHTS", "LightCoronaIntensity", gConfig.ReadInteger("VISUAL", "LightCoronaIntensity", 80));
	gfTailLightCoronaSize = gConfig.ReadFloat("LIGHTS", "TailLightCoronaSize", gConfig.ReadFloat("VISUAL", "TailLightCoronaSize", 0.8f));
	gTailLightCoronaIntensity = gConfig.ReadInteger("LIGHTS", "TailLightCoronaIntensity", gConfig.ReadInteger("VISUAL", "TailLightCoronaIntensity", 60));

	g_nFogLightKey = gConfig.ReadInteger("KEYS", "FogLightKey", VK_J);
	g_nLongLightKey = gConfig.ReadInteger("KEYS", "LongLightKey", VK_G);
	g_nIndicatorNoneKey = gConfig.ReadInteger("KEYS", "IndicatorLightNoneKey", VK_SHIFT);
	g_nIndicatorLeftKey = gConfig.ReadInteger("KEYS", "IndicatorLightLeftKey", VK_Z);
	g_nIndicatorRightKey = gConfig.ReadInteger("KEYS", "IndicatorLightRightKey", VK_C);
	g_nIndicatorBothKey = gConfig.ReadInteger("KEYS", "IndicatorLightBothKey", VK_X);

	g_bFoglightTiedToHeadlight = gConfig.ReadBoolean("LIGHTS", "FoglightTiedToHeadlight", gConfig.ReadBoolean("TWEAKS", "FoglightTiedToHeadlight", true));
	g_bAutoIndicatorsOnSteer = gConfig.ReadBoolean("LIGHTS", "AutoIndicatorsOnSteer", false);
}
void Lights::Init()
{
	if (gConfig.ReadBoolean("LIGHTS", "StandardLightsv2", gConfig.ReadBoolean("FEATURES", "StandardLightsv2", false))) {
		return;
	}

	m_bEnabled = true;
	patch::Nop(0x6E2722, 19);	  // CVehicle::DoHeadLightReflection
	patch::SetUChar(0x6E1A22, 0); // CVehicle::DoTailLightEffect

	// // CVehicle::DoHeadLightEffect
	patch::SetUChar(0x6E0CF8, 0);
	patch::SetUChar(0x6E0DEE, 0);

	// NOP CVehicle::DoHeadLightBeam
	if (!gConfig.ReadBoolean("LIGHTS", "HeadLightBeams", gConfig.ReadBoolean("TWEAKS", "HeadLightBeams", true)))
	{
		// cmp ax, ax
		patch::SetRaw(0x6A2EA5, (void *)"\x66\x39\xC0\x90", 4);
		patch::SetRaw(0x6BDE63, (void *)"\x66\x39\xC0\x90\x90\x90\x90", 7);
	}

	Events::initGameEvent += []()
	{
		InitConfig();
	};

	Events::vehicleDtorEvent += [](CVehicle *pVeh)
	{
		auto it = m_Dummies.find(pVeh);
		if (it != m_Dummies.end()) {
			for (auto &pair : it->second) {
				for (auto *pDummy : pair.second) {
					delete pDummy;
				}
			}
			m_Dummies.erase(it);
		}
	};

	ModelInfoMgr::RegisterMaterialColProvider([](CVehicle *pVeh, RpMaterial *pMat, eMaterialType type) -> MatStateColor
	{
		if (type == eMaterialType::HeadLightLeft || type == eMaterialType::HeadLightRight)
		{
			bool longLights = pVeh && m_VehData.Get(pVeh).m_bLongLightsOn;
			if (longLights)
			{
				return { CRGBA(255, 255, 255, 255), DEFAULT_MAT_COL };
			}
			else
			{
				return { CRGBA(100, 100, 100, 255), DEFAULT_MAT_COL };
			}
		}
		if (type == eMaterialType::TailLightLeft || type == eMaterialType::TailLightRight)
		{
			if (pVeh)
			{
				bool hasDedicatedBrake = IsMatAvail(pVeh, {eMaterialType::BrakeLightLeft, eMaterialType::BrakeLightRight, eMaterialType::NABrakeLightLeft, eMaterialType::NABrakeLightRight, eMaterialType::STTLightLeft, eMaterialType::STTLightRight});
				if (!hasDedicatedBrake)
				{
					bool isBraking = (pVeh->m_fBreakPedal > 0.05f) && (pVeh->m_pDriver != nullptr);
					if (isBraking)
					{
						return { CRGBA(255, 255, 255, 255), DEFAULT_MAT_COL };
					}
					else
					{
						return { CRGBA(180, 180, 180, 255), DEFAULT_MAT_COL };
					}
				}
			}
		}
		return { DEFAULT_MAT_COL, DEFAULT_MAT_COL };
	});

	ModelInfoMgr::RegisterMaterial([](CVehicle *pVeh, RpMaterial *pMat)
								   {
		if (!m_bEnabled || Util::IsAntiPatternLightMaterial(pMat)) {
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

	ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame, const std::string_view name)
								{
		if (pFrame && !rwLinkListEmpty(&pFrame->objectList)) {
			return;
		}

		DummyConfig c;
		c.frame = pFrame;
		c.position = pFrame->modelling.pos;
		c.pVeh = pVeh;
		c.corona.size = gfGlobalCoronaSize;
		c.corona.color = {255, 255, 255, static_cast<unsigned char>(gGlobalCoronaIntensity)};
		c.corona.lightingType = eLightingMode::NonDirectional;
		
		auto &dummies = m_Dummies[pVeh];
		
		if ((name.starts_with("fogl") || name.starts_with("fog_")) && (STR_FOUND(name, "_l") || STR_FOUND(name, "_r"))) {
			c.dummyPos = eDummyPos::Front;
			bool isLeft = STR_FOUND(name, "_l") || !STR_FOUND(name, "_r");
			c.lightType = isLeft ? eMaterialType::FogLightLeft : eMaterialType::FogLightRight;
			c.shadow.render = false;
			c.corona.color = c.shadow.color = {255, 255, 255, static_cast<unsigned char>(gGlobalCoronaIntensity)};
			c.corona.lightingType = eLightingMode::NonDirectional;
		}
		else if (name.starts_with("revl") || (name.starts_with("rev_") && !name.starts_with("revolution")) || name.starts_with("reverselight")) {
			bool isLeft = STR_FOUND(name, "_l");
			if (!isLeft && !STR_FOUND(name, "_r")) {
				isLeft = (c.position.x < 0.0f);
			}
			c.dummyPos = eDummyPos::Rear;
			c.lightType = isLeft ? eMaterialType::ReverseLightLeft : eMaterialType::ReverseLightRight;
			c.corona.color = c.shadow.color = {255, 255, 255, static_cast<unsigned char>(gGlobalCoronaIntensity)};
			c.corona.lightingType = eLightingMode::Directional;
		}
		else if ((name.starts_with("breakl") || name.starts_with("brakel")) && (STR_FOUND(name, "_l") || STR_FOUND(name, "_r"))) {
			c.dummyPos = eDummyPos::Rear;
			c.lightType = STR_FOUND(name, "_l") ? eMaterialType::BrakeLightLeft : eMaterialType::BrakeLightRight;
			c.corona.color = c.shadow.color = {240, 0, 0, static_cast<unsigned char>(gGlobalCoronaIntensity)};
			c.corona.lightingType = eLightingMode::Directional;
		}
		else if (name.starts_with("light_d")) {
			c.lightType = eMaterialType::DayLight;
			c.dummyPos = eDummyPos::Front;
			c.shadow.size = 1.85f;
			c.shadow.color = {220, 220, 220, static_cast<unsigned char>(gGlobalShadowIntensity)};
		}
		else if (name.starts_with("light_n")) {
			c.lightType = eMaterialType::NightLight;
			c.dummyPos = eDummyPos::Front;
			c.shadow.size = 1.85f;
			c.shadow.color = {220, 220, 220, static_cast<unsigned char>(gGlobalShadowIntensity)};
		}
		else if (auto d = Util::GetDigitsAfter(name, "strobe_light")) {
			c.lightType = eMaterialType::StrobeLight;
			c.dummyPos = eDummyPos::Front;
			c.dummyIdx = d.value();
		}
		else if (auto d = Util::GetCharsAfterPrefix(name, "sidelight_", 1)) {
			if (d == "L") {
				c.lightType = eMaterialType::SideLightLeft;
				c.dummyPos = eDummyPos::Left;
			} else {
				c.lightType = eMaterialType::SideLightRight;
				c.dummyPos = eDummyPos::Right;
			}
		}
		else if (auto d = Util::GetCharsAfterPrefix(name, "sttlight_", 1)) {
			c.lightType = (d == "L") ? eMaterialType::STTLightLeft : eMaterialType::STTLightRight;
			c.dummyPos = eDummyPos::Rear;
			c.corona.color = c.shadow.color = {240, 0, 0, static_cast<unsigned char>(gGlobalCoronaIntensity)};
			c.corona.lightingType = eLightingMode::Directional;
		}
		else if (auto d = Util::GetCharsAfterPrefix(name, "nabrakelight_", 1)) {
			c.lightType = (d == "L") ? eMaterialType::NABrakeLightLeft : eMaterialType::NABrakeLightRight;
			c.dummyPos = eDummyPos::Rear;
			c.corona.color = c.shadow.color = {240, 0, 0, static_cast<unsigned char>(gGlobalCoronaIntensity)};
			c.corona.lightingType = eLightingMode::Directional;
		}
		else if (name.starts_with("spotlight_light")) {
			c.lightType = eMaterialType::SpotLight;
		}
		else if (name.starts_with("light_a")) {
			c.lightType = eMaterialType::AllDayLight;
			c.dummyPos = eDummyPos::Front;
			c.shadow.size = 1.85f;
			c.shadow.color = {220, 220, 220, static_cast<unsigned char>(gGlobalShadowIntensity)};
		}
		else if (name == "taillights" || name == "taillights2") { // some models have dummies starting with taillights
			c.dummyPos = eDummyPos::Rear;
			c.lightType = eMaterialType::TailLightRight;
			c.corona.size = gfTailLightCoronaSize;
			c.corona.color = {250, 0, 0, static_cast<unsigned char>(gTailLightCoronaIntensity)};
			c.shadow.color = {250, 0, 0, static_cast<unsigned char>(gGlobalShadowIntensity)};
			c.corona.lightingType = eLightingMode::Directional; 				
			c.shadow.render = name != "taillights2";
			dummies[c.lightType].push_back(new VehicleDummy(c));
			if (pVeh->m_nVehicleSubClass != VEHICLE_BIKE || std::abs(c.frame->modelling.pos.x) > 0.05f) {
				c.mirroredX = true;
				c.lightType = eMaterialType::TailLightLeft;
				dummies[c.lightType].push_back(new VehicleDummy(c));
			}
			return;
		}
		else if (name == "headlights" || name == "headlights2") {
			c.dummyPos = eDummyPos::Front;
			c.lightType = eMaterialType::HeadLightLeft;
			c.corona.color = c.shadow.color = {250, 250, 250, static_cast<unsigned char>(gGlobalCoronaIntensity)};
			c.corona.lightingType = eLightingMode::Directional;
			c.shadow.render = name != "headlights2";
			c.mirroredX = true;
			dummies[c.lightType].push_back(new VehicleDummy(c));
			// A single centered dummy would only produce a second corona in the same
			// spot, so mirror it for bikes that actually have it off to one side
			if (pVeh->m_nVehicleSubClass != VEHICLE_BIKE || std::abs(c.frame->modelling.pos.x) > 0.05f) {
				c.mirroredX = false;
				c.lightType = eMaterialType::HeadLightRight;
				dummies[c.lightType].push_back(new VehicleDummy(c));
			}
			return;
		}
		else if (name.starts_with("turnl_") || name.starts_with("indicator_")) {
			auto d = Util::GetCharsAfterPrefix(name, "turnl_", 2);
			if (!d) d = Util::GetCharsAfterPrefix(name, "indicator_", 2);
			if (!d) d = Util::GetCharsAfterPrefix(name, "turnl_", 1);
			if (!d) d = Util::GetCharsAfterPrefix(name, "indicator_", 1);
			if (d) {
				bool isLeft = (d.value()[0] == 'L' || d.value()[0] == 'l');
				c.corona.color = c.shadow.color = {255, 128, 0, static_cast<unsigned char>(gGlobalCoronaIntensity)};
				c.corona.lightingType = eLightingMode::Directional;

				char posChar = (d.value().length() >= 2) ? d.value()[1] : (c.position.y >= 0.0f ? 'F' : 'R');
				switch (posChar) {
					case 'F':
					case 'f':
						c.lightType = isLeft ? eMaterialType::IndicatorLightLeftFront : eMaterialType::IndicatorLightRightFront;
						c.dummyPos = eDummyPos::Front;
						break;
					case 'R':
					case 'r':
						c.lightType = isLeft ? eMaterialType::IndicatorLightLeftRear : eMaterialType::IndicatorLightRightRear;
						c.dummyPos = eDummyPos::Rear;
						break;
					case 'M':
					case 'm':
						c.lightType = isLeft ? eMaterialType::IndicatorLightLeftMiddle : eMaterialType::IndicatorLightRightMiddle;
						c.dummyPos = isLeft ? eDummyPos::Right : eDummyPos::Left;
						break;
					default:
						c.lightType = isLeft ? eMaterialType::IndicatorLightLeftFront : eMaterialType::IndicatorLightRightFront;
						c.dummyPos = eDummyPos::Front;
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
			bool isHeadlightsActive = (pVeh->bLightsOn || CarUtil::IsLightsForcedOn(pVeh) || Util::IsNightTime()) && !CarUtil::IsLightsForcedOff(pVeh);
			bool canToggleFogLight = !g_bFoglightTiedToHeadlight || isHeadlightsActive;
			if (Util::IsKeyPressed(g_nFogLightKey) && IsMatAvail(pVeh, {eMaterialType::FogLightLeft, eMaterialType::FogLightRight}) && canToggleFogLight)
			{
				size_t now = CTimer::m_snTimeInMilliseconds;
				if (now - prev > 500.0f)
				{
					VehLightDatav1 &data = m_VehData.Get(pVeh);
					data.m_bFogLightsOn = !data.m_bFogLightsOn;
					prev = now;
					AudioMgr::PlaySwitchSound(pVeh);
				}
			}

			bool isHeadlightsActiveForLong = (pVeh->bLightsOn || CarUtil::IsLightsForcedOn(pVeh) || Util::IsNightTime() || !Util::IsEngineOff(pVeh)) && !CarUtil::IsLightsForcedOff(pVeh);
			bool canToggleLongLights = !(gbProperShadersDetected && !gbLightPointLights);
			if (Util::IsKeyPressed(g_nLongLightKey) && isHeadlightsActiveForLong && canToggleLongLights)
			{
				size_t now = CTimer::m_snTimeInMilliseconds;
				if (now - prev > 500.0f)
				{
					VehLightDatav1 &data = m_VehData.Get(pVeh);
					data.m_bLongLightsOn = !data.m_bLongLightsOn;
					prev = now;
					AudioMgr::PlaySwitchSound(pVeh);
				}
			}
		}
	};

	MEEvents::vehPreRenderEvent.before += [](CVehicle *pVeh)
	{
		ProcessPointLights(pVeh);
	};

	// Headlight coronas & shadows for every vehicle the local player isn't driving.
	// Kept on the script tick on purpose: it keeps running while the vehicle itself is
	// culled off-screen, the vehicle render callback below doesn't.
	Events::processScriptsEvent += []()
	{
		for (CVehicle *pVeh : CPools::ms_pVehiclePool)
		{
			if (pVeh && pVeh->m_nVehicleSubClass == VEHICLE_BIKE)
			{
				ProcessPointLights(pVeh);
			}

			// Mirrors the checks in the render callback below so both paths agree on
			// which vehicles get headlights
			if (pVeh->m_pDriver == FindPlayerPed() || pVeh->m_fHealth <= 0.0f || pVeh->m_nVehicleSubClass == VEHICLE_BMX || pVeh->m_nVehicleSubClass == VEHICLE_BOAT || pVeh->m_nVehicleSubClass == VEHICLE_TRAILER || (Util::IsEngineOff(pVeh) && !CarUtil::IsLightsForcedOn(pVeh) && !pVeh->bLightsOn))
			{
				continue;
			}

			if (CVector::Distance(pVeh->GetPosition(), TheCamera.GetPosition()) < 150.0f || pVeh->GetIsOnScreen())
			{
				bool isLeftFrontOk = !Util::IsLightDamaged(pVeh, eLights::LIGHT_FRONT_LEFT);
				bool isRightFrontOk = !Util::IsLightDamaged(pVeh, eLights::LIGHT_FRONT_RIGHT);
				RenderHeadlights(pVeh, isLeftFrontOk, isRightFrontOk);

				// Claim this frame so the render callback doesn't register them twice
				m_VehData.Get(pVeh).m_nHeadlightTickFrame = CTimer::m_FrameCounter;
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

		VehLightDatav1 &data = m_VehData.Get(pControlVeh);
		eIndicatorState indState = data.m_nIndicatorState;

		// Fix for UIF SAMP server https://github.com/user-grinch/ModelExtras/issues/112
		// Don't clear light state when lights are forced on/already on via SAMP
		if (((Util::IsEngineOff(pControlVeh) && indState == eIndicatorState::Off) && !CarUtil::IsLightsForcedOn(pControlVeh) && !pControlVeh->bLightsOn) || CarUtil::IsLightsForcedOff(pControlVeh)) {
			pControlVeh->bLightsOn = false;
			pControlVeh->m_renderLights.m_bLeftFront = false;
			pControlVeh->m_renderLights.m_bRightFront = false;
			pControlVeh->m_renderLights.m_bLeftRear = false;
			pControlVeh->m_renderLights.m_bRightRear = false;
		}

		// Fix for park car alarm lights
		// Allow through if lights or indicators are explicitly on
		if (pControlVeh->m_fHealth <= 0.0f || ((Util::IsEngineOff(pControlVeh) && indState == eIndicatorState::Off) && !CarUtil::IsLightsForcedOn(pControlVeh) && !pControlVeh->bLightsOn)) {
			return;
		}

		bool isLeftFrontDamaged = Util::IsLightDamaged(pControlVeh, eLights::LIGHT_FRONT_LEFT) || Util::IsPanelDamaged(pControlVeh, ePanels::WING_FRONT_LEFT);
		bool isRightFrontDamaged = Util::IsLightDamaged(pControlVeh, eLights::LIGHT_FRONT_RIGHT) || Util::IsPanelDamaged(pControlVeh, ePanels::WING_FRONT_RIGHT);
		bool isHeadlightLeftOk = !isLeftFrontDamaged;
		bool isHeadlightRightOk = !isRightFrontDamaged;
		// When sirens are active on SAMP/UIF, server flasher scripts rapidly toggle light damage
		// Don't let flasher damage toggles disable indicator lights
		bool isLeftFrontOk = (!Util::IsLightDamaged(pControlVeh, eLights::LIGHT_FRONT_LEFT) && !Util::IsPanelDamaged(pControlVeh, ePanels::WING_FRONT_LEFT)) || pControlVeh->bSirenOrAlarm;
		bool isRightFrontOk = (!Util::IsLightDamaged(pControlVeh, eLights::LIGHT_FRONT_RIGHT) && !Util::IsPanelDamaged(pControlVeh, ePanels::WING_FRONT_RIGHT)) || pControlVeh->bSirenOrAlarm;
		bool isLeftMiddleOk = !Util::IsPanelDamaged(pControlVeh, ePanels::WING_FRONT_LEFT);
		bool isRightMiddleOk = !Util::IsPanelDamaged(pControlVeh, ePanels::WING_FRONT_RIGHT);

		bool isFrontBumperDamaged = Util::IsPanelDamaged(pControlVeh, ePanels::BUMP_FRONT);
		bool isLeftRearOk = !(Util::IsLightDamaged(pTowedVeh, eLights::LIGHT_REAR_LEFT) || Util::IsPanelDamaged(pTowedVeh, ePanels::WING_REAR_LEFT));
		bool isRightRearOk = !(Util::IsLightDamaged(pTowedVeh, eLights::LIGHT_REAR_RIGHT) || Util::IsPanelDamaged(pTowedVeh, ePanels::WING_REAR_RIGHT));
		RenderLights(pControlVeh, pTowedVeh, eMaterialType::AllDayLight, true, "indicator", 1.85f);
		RenderLights(pControlVeh, pTowedVeh, eMaterialType::StrobeLight);
		RenderLights(pControlVeh, pTowedVeh, eMaterialType::SideLightLeft, true, "indicator", 1.85f, false, isLeftMiddleOk);
		RenderLights(pControlVeh, pTowedVeh, eMaterialType::SideLightRight, true, "indicator", 1.85f, false, isRightMiddleOk);
		
		if (Util::IsNightTime()) {
			RenderLights(pControlVeh, pTowedVeh, eMaterialType::NightLight, true, "indicator", 1.85f);
		} else {
			RenderLights(pControlVeh, pTowedVeh, eMaterialType::DayLight, true, "indicator", 1.85f);
		}
		
		static bool foglightTiedtoHeadlight = gConfig.ReadBoolean("LIGHTS", "FoglightTiedToHeadlight", gConfig.ReadBoolean("TWEAKS", "FoglightTiedToHeadlight", true));
		bool isHeadlightsActive = (pControlVeh->bLightsOn || CarUtil::IsLightsForcedOn(pControlVeh) || Util::IsNightTime()) && !CarUtil::IsLightsForcedOff(pControlVeh);
		bool shouldRenderFog = !foglightTiedtoHeadlight || isHeadlightsActive;
		if (data.m_bFogLightsOn && shouldRenderFog) {
			bool isFogOk = !isFrontBumperDamaged;
			RenderLights(pControlVeh, pTowedVeh, eMaterialType::FogLightLeft, true, "foglight", 3.0f, false, isFogOk);
			RenderLights(pControlVeh, pTowedVeh, eMaterialType::FogLightRight, true, "foglight", 3.0f, false, isFogOk);
		}

		bool isBike = CModelInfo::IsBikeModel(pControlVeh->m_nModelIndex);

		// Only register coronas & shadows here if the script tick above didn't already
		// claim this frame. Testing m_pDriver on both sides instead would drop a frame
		// while entering/exiting, since the driver pointer changes between the script
		// tick and the render pass, leaving neither path responsible for that frame.
		// Lit materials are always set from here, SetupRender clears the material states
		// every frame right before the vehicle is drawn.
		bool bTickRegistered = data.m_nHeadlightTickFrame == CTimer::m_FrameCounter;
		RenderHeadlights(pControlVeh, isHeadlightLeftOk, isHeadlightRightOk, bTickRegistered);

		if (SpotLights::IsEnabled(pControlVeh)) {
			RenderLights(pControlVeh, pTowedVeh, eMaterialType::SpotLight, false, "", 1.0f, false, true, true);
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
					RenderLights(pControlVeh, pTowedVeh, eMaterialType::ReverseLightLeft, true, shdwName, shdwSz, false, isLeftRearOk);
				}
				if (isRightRearOk) {
					RenderLights(pControlVeh, pTowedVeh, eMaterialType::ReverseLightRight, true, shdwName, shdwSz, false, isRightRearOk);
				}
			}

			bool sttInstalled = IsMatAvail(pTowedVeh, {eMaterialType::STTLightLeft, eMaterialType::STTLightRight});
			// taillights/ brakelights
			if (pControlVeh->m_fBreakPedal && pControlVeh->m_pDriver) {
				if (sttInstalled) {
					if (isLeftRearOk) {
						RenderLights(pControlVeh, pTowedVeh, eMaterialType::STTLightLeft, true, shdwName, shdwSz, false, isLeftRearOk);
					}
					if (isRightRearOk) {
						RenderLights(pControlVeh, pTowedVeh, eMaterialType::STTLightRight, true, shdwName, shdwSz, false, isRightRearOk);
					}
				} else {
					if (IsMatAvail(pTowedVeh, {eMaterialType::BrakeLightLeft, eMaterialType::BrakeLightRight})) {
						if (isLeftRearOk) {
							RenderLights(pControlVeh, pTowedVeh, eMaterialType::BrakeLightLeft, true, shdwName, shdwSz, false, isLeftRearOk);
						}
						if (isRightRearOk) {
							RenderLights(pControlVeh, pTowedVeh, eMaterialType::BrakeLightRight, true, shdwName, shdwSz, false, isRightRearOk);
						}
					}
					else if (IsMatAvail(pTowedVeh, {eMaterialType::TailLightLeft, eMaterialType::TailLightRight})) {
						if (isLeftRearOk) {
							RenderLights(pControlVeh, pTowedVeh, eMaterialType::TailLightLeft, true, shdwName, shdwSz, true, isLeftRearOk);
						}
						if (isRightRearOk) {
							RenderLights(pControlVeh, pTowedVeh, eMaterialType::TailLightRight, true, shdwName, shdwSz, true, isRightRearOk);
						}
					}
				}

				if (indState != eIndicatorState::BothOn) {
					if (indState != eIndicatorState::LeftOn && isLeftRearOk) {
						RenderLights(pControlVeh, pTowedVeh, eMaterialType::NABrakeLightLeft, true, shdwName, shdwSz, false, isLeftRearOk);
					}

					if (indState != eIndicatorState::RightOn && isRightRearOk) {
						RenderLights(pControlVeh, pTowedVeh, eMaterialType::NABrakeLightRight, true, shdwName, shdwSz, false, isRightRearOk);
					}
				}
			}

			bool indicatorOn = data.m_bUsingGlobalIndicators && data.m_nIndicatorState != eIndicatorState::Off;
			bool tailLightFlag = (Util::IsNightTime() || pControlVeh->bLightsOn || CarUtil::IsLightsForcedOn(pControlVeh)) && !CarUtil::IsLightsForcedOff(pControlVeh);
			if (tailLightFlag || indicatorOn) {
				if (sttInstalled) {
					if (isLeftRearOk) {
						RenderLights(pControlVeh, pTowedVeh, eMaterialType::STTLightLeft, true, shdwName, shdwSz, false, isLeftRearOk);
					}

					if (isRightRearOk) {
						RenderLights(pControlVeh, pTowedVeh, eMaterialType::STTLightRight, true, shdwName, shdwSz, false, isRightRearOk);
					}
				}
				else {
					bool hasDedicatedBrake = IsMatAvail(pTowedVeh, {eMaterialType::BrakeLightLeft, eMaterialType::BrakeLightRight, eMaterialType::NABrakeLightLeft, eMaterialType::NABrakeLightRight, eMaterialType::STTLightLeft, eMaterialType::STTLightRight}) ||
					                         IsDummyAvail(pTowedVeh, {eMaterialType::BrakeLightLeft, eMaterialType::BrakeLightRight, eMaterialType::NABrakeLightLeft, eMaterialType::NABrakeLightRight, eMaterialType::STTLightLeft, eMaterialType::STTLightRight});
					bool isBraking = (pControlVeh->m_fBreakPedal > 0.05f) && (pControlVeh->m_pDriver != nullptr);
					bool tailHighlight = !hasDedicatedBrake && isBraking;

					auto tailLightsRender = [&](bool leftOk, bool rightOk) {
						if (IsMatAvail(pTowedVeh, {eMaterialType::TailLightLeft, eMaterialType::TailLightRight}) || IsDummyAvail(pTowedVeh, {eMaterialType::TailLightLeft, eMaterialType::TailLightRight})) {
							if (leftOk) {
								RenderLights(pControlVeh, pTowedVeh, eMaterialType::TailLightLeft, true, shdwName, shdwSz, tailHighlight, leftOk);
							}
							if (rightOk) {
								RenderLights(pControlVeh, pTowedVeh, eMaterialType::TailLightRight, true, shdwName, shdwSz, tailHighlight, rightOk);
							}
						} else if (IsMatAvail(pTowedVeh, {eMaterialType::BrakeLightLeft, eMaterialType::BrakeLightRight}) || IsDummyAvail(pTowedVeh, {eMaterialType::BrakeLightLeft, eMaterialType::BrakeLightRight})) {
							if (leftOk) {
								RenderLights(pControlVeh, pTowedVeh, eMaterialType::BrakeLightLeft, true, shdwName, shdwSz, false, leftOk);
							}
							if (rightOk) {
								RenderLights(pControlVeh, pTowedVeh, eMaterialType::BrakeLightRight, true, shdwName, shdwSz, false, rightOk);
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

			static bool bSAMP = GetModuleHandle("samp.dll") != nullptr;

			if (pControlVeh->m_pDriver == FindPlayerPed() &&
				(pControlVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pControlVeh->m_nVehicleSubClass == VEHICLE_BIKE || pControlVeh->m_nVehicleSubClass == VEHICLE_QUAD || pControlVeh->m_nVehicleSubClass == VEHICLE_MTRUCK))
			{
				if (Util::IsKeyPressed(g_nIndicatorNoneKey))
				{
					data.m_nIndicatorState = eIndicatorState::Off;
					delay = 0;
					indicatorsDelay = false;
				}

				if (Util::IsKeyPressed(g_nIndicatorLeftKey))
				{
					data.m_nIndicatorState = eIndicatorState::LeftOn;
				}

				if (Util::IsKeyPressed(g_nIndicatorRightKey))
				{
					data.m_nIndicatorState = eIndicatorState::RightOn;
				}

				if (Util::IsKeyPressed(g_nIndicatorBothKey))
				{
					data.m_nIndicatorState = eIndicatorState::BothOn;
				}

				if (g_bAutoIndicatorsOnSteer && data.m_nIndicatorState != eIndicatorState::BothOn)
				{
					static bool bWasAutoSteerActive = false;
					bool bSteerLeft = (pControlVeh->m_fSteerAngle > 0.08f) || Util::IsKeyPressed('A') || Util::IsKeyPressed(VK_LEFT);
					bool bSteerRight = (pControlVeh->m_fSteerAngle < -0.08f) || Util::IsKeyPressed('D') || Util::IsKeyPressed(VK_RIGHT);

					if (bSteerLeft && !bSteerRight)
					{
						data.m_nIndicatorState = eIndicatorState::LeftOn;
						bWasAutoSteerActive = true;
					}
					else if (bSteerRight && !bSteerLeft)
					{
						data.m_nIndicatorState = eIndicatorState::RightOn;
						bWasAutoSteerActive = true;
					}
					else if (bWasAutoSteerActive)
					{
						data.m_nIndicatorState = eIndicatorState::Off;
						bWasAutoSteerActive = false;
					}
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
					RenderLights(pControlVeh, pTowedVeh, eMaterialType::IndicatorLightLeftMiddle, true, "indicator", 1.0f, false, isLeftMiddleOk);
					RenderLights(pControlVeh, pTowedVeh, eMaterialType::IndicatorLightLeftRear, true, "indicator", 1.0f, false, isLeftRearOk);
					if (isLeftRearOk) {
						RenderLights(pControlVeh, pTowedVeh, eMaterialType::STTLightLeft, true, shdwName, shdwSz, true, isLeftRearOk);
					}
				}

				if (indState == eIndicatorState::BothOn || indState == eIndicatorState::RightOn) {
					RenderLights(pControlVeh, pTowedVeh, eMaterialType::IndicatorLightRightFront, true, "indicator", 1.0f, false, isRightFrontOk);
					RenderLights(pControlVeh, pTowedVeh, eMaterialType::IndicatorLightRightMiddle, true, "indicator", 1.0f, false, isRightMiddleOk);
					RenderLights(pControlVeh, pTowedVeh, eMaterialType::IndicatorLightRightRear, true, "indicator", 1.0f, false, isRightRearOk);
					if (isRightRearOk) {
						RenderLights(pControlVeh, pTowedVeh, eMaterialType::STTLightRight, true, shdwName, shdwSz, true, isRightRearOk);
					}
				}
			}
			if (indState == eIndicatorState::BothOn || indState == eIndicatorState::LeftOn) {
				if (isLeftRearOk) {
					RenderLights(pControlVeh, pTowedVeh, eMaterialType::NABrakeLightLeft, true, "indicator", 1.0f, false, isLeftRearOk);
				}
			}

			if (indState == eIndicatorState::BothOn || indState == eIndicatorState::RightOn) {
				if (isRightRearOk) {
					RenderLights(pControlVeh, pTowedVeh, eMaterialType::NABrakeLightRight, true, "indicator", 1.0f, false, isRightRearOk);
				}
			} });
};

void Lights::RenderLight(CVehicle *pVeh, eMaterialType state, bool shadows, std::string texture, float sz, bool highlight, bool isDummyOk, bool materialsOnly)
{
	int id = static_cast<int>(state) * 1000;
	bool hasActiveDummy = false;
	bool isDummyAvailable = IsDummyAvail(pVeh, state);

	if (isDummyAvailable)
	{
		for (auto e : m_Dummies[pVeh][state])
		{
			const DummyConfig &c = e->GetRef();
			e->Update();
			RwFrame *parent = RwFrameGetParent(e->Get().frame);
			eMaterialType type = e->GetRef().lightType;
			bool isBike = pVeh->m_nVehicleSubClass == VEHICLE_BIKE;
			bool isDamaged = Util::IsFrameDamaged(pVeh, parent) || !FrameUtil::IsOkAtomicVisible(parent);
			bool atomicCheck = !isBike && pVeh->GetIsOnScreen() && type != eMaterialType::HeadLightLeft && type != eMaterialType::HeadLightRight && isDamaged;

			if (atomicCheck || (c.dummyPos == eDummyPos::Rear && pVeh->m_pTrailer) || !isDummyOk)
			{
				continue;
			}

			hasActiveDummy = true;

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
			// Coronas & shadows are handled by the processScripts loop for this vehicle
			if (materialsOnly)
			{
				continue;
			}

			float szMul = 1.0f;
			if (highlight)
			{
				szMul = (state == eMaterialType::TailLightLeft || state == eMaterialType::TailLightRight) ? 1.50f : 3.00f;
			}
			EnableDummy((int)pVeh + 42 + id++, e, pVeh, szMul);

			// Skip front shadows on bike wheelie
			if (c.dummyPos == eDummyPos::Front && Util::IsVehicleDoingWheelie(pVeh))
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

	if (!isDummyAvailable || hasActiveDummy)
	{
		ModelInfoMgr::EnableMaterial(pVeh, state);
	}
}

void Lights::RenderLights(CVehicle *pControlVeh, CVehicle *pTowedVeh, eMaterialType state, bool shadows, std::string texture, float sz, bool highlight, bool isDummyOk, bool materialsOnly)
{
	int model = pControlVeh->m_nModelIndex;
	// if (CModelInfo::IsHeliModel(model) || CModelInfo::IsPlaneModel(model)) {
	// 	sz = 1.0f;
	// 	texture = "pointlight";
	// }

	if (GetLightState(pControlVeh, state))
	{
		RenderLight(pControlVeh, state, shadows, texture, sz, highlight, isDummyOk, materialsOnly);
	}

	if (pControlVeh != pTowedVeh && GetLightState(pTowedVeh, state))
	{
		RenderLight(pTowedVeh, state, shadows, texture, sz, highlight, isDummyOk, materialsOnly);
	}
}

void Lights::RenderHeadlights(CVehicle *pControlVeh, bool isLeftOn, bool isRightOn, bool materialsOnly)
{
	CVehicle *pTowedVeh = pControlVeh;
	VehLightDatav1 &data = m_VehData.Get(pControlVeh);

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
		bool isFoggy = (CWeather::Foggyness > 0.1f) || (CWeather::Rain > 0.3f) || (CWeather::NewWeatherType == WEATHER_FOGGY_SF || CWeather::NewWeatherType == WEATHER_SANDSTORM_DESERT || CWeather::OldWeatherType == WEATHER_FOGGY_SF || CWeather::OldWeatherType == WEATHER_SANDSTORM_DESERT);
		std::string texName = data.m_bLongLightsOn ? "headlight_long" : "headlight_short";
		bool shadow = !gbProperShadersDetected;
		bool highlight = isFoggy || data.m_bLongLightsOn;

		if (isLeftOn || isRightOn)
		{
			if (isLeftOn && GetLightState(pControlVeh, eMaterialType::HeadLightLeft))
			{
				RenderLights(pControlVeh, pTowedVeh, eMaterialType::HeadLightLeft, shadow, texName, headlightSz, highlight, true, materialsOnly);
			}
			if (isRightOn && GetLightState(pControlVeh, eMaterialType::HeadLightRight))
			{
				RenderLights(pControlVeh, pTowedVeh, eMaterialType::HeadLightRight, shadow, texName, headlightSz, highlight, true, materialsOnly);
			}
		}
	}
}

void Lights::EnableDummy(int id, VehicleDummy *dummy, CVehicle *pVeh, float szMul)
{
	if (gbLightCoronasFeature)
	{
		const DummyConfig &c = dummy->GetRef();
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

// NOT

void Lights::Reload(CVehicle* pVeh)
{
	InitConfig();
	if (pVeh) {
		auto it = m_Dummies.find(pVeh);
		if (it != m_Dummies.end()) {
			for (auto &pair : it->second) {
				for (auto *pDummy : pair.second) {
					delete pDummy;
				}
			}
			m_Dummies.erase(it);
		}
		DataMgr::Reload(pVeh->m_nModelIndex);
	}
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

void Lights::ProcessPointLights(CVehicle *pVeh)
{
	if (!gbLightPointLights || !pVeh || pVeh->m_fHealth <= 0.0f || pVeh->m_nVehicleSubClass == VEHICLE_BMX || pVeh->m_nVehicleSubClass == VEHICLE_BOAT || pVeh->m_nVehicleSubClass == VEHICLE_TRAILER)
	{
		return;
	}

	if (CVector::Distance(pVeh->GetPosition(), TheCamera.GetPosition()) > 75.0f)
	{
		return;
	}

	VehLightDatav1 &data = m_VehData.Get(pVeh);
	bool isBike = pVeh->m_nVehicleSubClass == VEHICLE_BIKE;
	bool isHeadlightsOn = (pVeh->bLightsOn || CarUtil::IsLightsForcedOn(pVeh) || Util::IsNightTime() || (isBike && !Util::IsEngineOff(pVeh))) && !CarUtil::IsLightsForcedOff(pVeh);

	// 1. High Beam Headlights
	if (data.m_bLongLightsOn && isHeadlightsOn)
	{
		static float rawMul = gConfig.ReadFloat("LIGHTS", "HighBeamPointLightMul", gConfig.ReadFloat("TWEAKS", "HighBeamPointLightMul", 2.0f));
		static float highBeamMul = (rawMul < 1.0f) ? 1.0f : ((rawMul > 4.0f) ? 4.0f : rawMul);

		for (eMaterialType type : {eMaterialType::HeadLightLeft, eMaterialType::HeadLightRight})
		{
			if (!IsDummyAvail(pVeh, type) || !GetLightState(pVeh, type))
			{
				continue;
			}

			bool isLeft = (type == eMaterialType::HeadLightLeft);
			eLights lightEnum = isLeft ? eLights::LIGHT_FRONT_LEFT : eLights::LIGHT_FRONT_RIGHT;
			ePanels wingEnum = isLeft ? ePanels::WING_FRONT_LEFT : ePanels::WING_FRONT_RIGHT;
			if (Util::IsLightDamaged(pVeh, lightEnum) || Util::IsPanelDamaged(pVeh, wingEnum))
			{
				continue;
			}

			for (auto e : m_Dummies[pVeh][type])
			{
				e->Update();
				RenderUtil::RegisterHeadlightPointLight(&e->Get(), highBeamMul);
			}
		}
	}

	// 2. Fog Lights
	static bool foglightTiedtoHeadlight = gConfig.ReadBoolean("LIGHTS", "FoglightTiedToHeadlight", gConfig.ReadBoolean("TWEAKS", "FoglightTiedToHeadlight", true));
	bool shouldRenderFog = !foglightTiedtoHeadlight || isHeadlightsOn;
	if (data.m_bFogLightsOn && shouldRenderFog)
	{
		for (eMaterialType type : {eMaterialType::FogLightLeft, eMaterialType::FogLightRight})
		{
			if (!IsDummyAvail(pVeh, type) || !GetLightState(pVeh, type))
			{
				continue;
			}

			bool isLeft = (type == eMaterialType::FogLightLeft);
			eLights lightEnum = isLeft ? eLights::LIGHT_FRONT_LEFT : eLights::LIGHT_FRONT_RIGHT;
			ePanels wingEnum = isLeft ? ePanels::WING_FRONT_LEFT : ePanels::WING_FRONT_RIGHT;
			if (Util::IsLightDamaged(pVeh, lightEnum) || Util::IsPanelDamaged(pVeh, wingEnum) || Util::IsPanelDamaged(pVeh, ePanels::BUMP_FRONT))
			{
				continue;
			}

			for (auto e : m_Dummies[pVeh][type])
			{
				e->Update();
				RenderUtil::RegisterPointLight(&e->Get(), e->Get().corona.color, 8.5f, true);
			}
		}
	}

	// 3. Reverse Lights
	bool isReversing = (pVeh->m_nCurrentGear == 0) && (Util::GetVehicleSpeed(pVeh) >= 0.001f || pVeh->m_fBreakPedal > 0.05f) && (pVeh->m_pDriver != nullptr) &&
	                   (pVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pVeh->m_nVehicleSubClass == VEHICLE_MTRUCK || pVeh->m_nVehicleSubClass == VEHICLE_QUAD);
	if (isReversing)
	{
		for (eMaterialType type : {eMaterialType::ReverseLightLeft, eMaterialType::ReverseLightRight})
		{
			if (!IsDummyAvail(pVeh, type) || !GetLightState(pVeh, type))
			{
				continue;
			}

			bool isLeft = (type == eMaterialType::ReverseLightLeft);
			eLights lightEnum = isLeft ? eLights::LIGHT_REAR_LEFT : eLights::LIGHT_REAR_RIGHT;
			ePanels wingEnum = isLeft ? ePanels::WING_REAR_LEFT : ePanels::WING_REAR_RIGHT;
			if (Util::IsLightDamaged(pVeh, lightEnum) || Util::IsPanelDamaged(pVeh, wingEnum))
			{
				continue;
			}

			for (auto e : m_Dummies[pVeh][type])
			{
				e->Update();
				RenderUtil::RegisterPointLight(&e->Get(), e->Get().corona.color, 3.2f, true);
			}
		}
	}

	// 4. Taillights & Brake Lights
	bool isBraking = (pVeh->m_fBreakPedal > 0.05f) && (pVeh->m_pDriver != nullptr);
	bool hasDedicatedBrakeDummy = IsDummyAvail(pVeh, eMaterialType::BrakeLightLeft) ||
	                              IsDummyAvail(pVeh, eMaterialType::BrakeLightRight) ||
	                              IsDummyAvail(pVeh, eMaterialType::NABrakeLightLeft) ||
	                              IsDummyAvail(pVeh, eMaterialType::NABrakeLightRight) ||
	                              IsDummyAvail(pVeh, eMaterialType::STTLightLeft) ||
	                              IsDummyAvail(pVeh, eMaterialType::STTLightRight);

	if (isHeadlightsOn || (isBraking && !hasDedicatedBrakeDummy))
	{
		float tailRadius = 3.5f;
		constexpr float tailPointLightMul = 0.40f;

		for (eMaterialType type : {eMaterialType::TailLightLeft, eMaterialType::TailLightRight})
		{
			if (!IsDummyAvail(pVeh, type) || !GetLightState(pVeh, type))
			{
				continue;
			}

			bool isLeft = (type == eMaterialType::TailLightLeft);
			if (data.m_bUsingGlobalIndicators && !IsMatAvail(pVeh, INDICATOR_LIGHTS_TYPE))
			{
				if (isLeft && (data.m_nIndicatorState == eIndicatorState::LeftOn || data.m_nIndicatorState == eIndicatorState::BothOn))
					continue;
				if (!isLeft && (data.m_nIndicatorState == eIndicatorState::RightOn || data.m_nIndicatorState == eIndicatorState::BothOn))
					continue;
			}

			eLights lightEnum = isLeft ? eLights::LIGHT_REAR_LEFT : eLights::LIGHT_REAR_RIGHT;
			ePanels wingEnum = isLeft ? ePanels::WING_REAR_LEFT : ePanels::WING_REAR_RIGHT;
			if (Util::IsLightDamaged(pVeh, lightEnum) || Util::IsPanelDamaged(pVeh, wingEnum))
			{
				continue;
			}

			for (auto e : m_Dummies[pVeh][type])
			{
				e->Update();
				CRGBA baseCol = e->Get().corona.color;
				CRGBA tailColor = (isBraking && !hasDedicatedBrakeDummy) ? CRGBA(255, 20, 20, 255) : CRGBA(static_cast<unsigned char>(baseCol.r * tailPointLightMul), static_cast<unsigned char>(baseCol.g * tailPointLightMul), static_cast<unsigned char>(baseCol.b * tailPointLightMul), baseCol.a);
				RenderUtil::RegisterPointLight(&e->Get(), tailColor, tailRadius, true);
			}
		}
	}

	if (isBraking)
	{
		for (eMaterialType type : {eMaterialType::BrakeLightLeft, eMaterialType::BrakeLightRight, eMaterialType::NABrakeLightLeft, eMaterialType::NABrakeLightRight, eMaterialType::STTLightLeft, eMaterialType::STTLightRight})
		{
			if (!IsDummyAvail(pVeh, type) || !GetLightState(pVeh, type))
			{
				continue;
			}

			bool isLeft = (type == eMaterialType::BrakeLightLeft || type == eMaterialType::NABrakeLightLeft || type == eMaterialType::STTLightLeft);
			eLights lightEnum = isLeft ? eLights::LIGHT_REAR_LEFT : eLights::LIGHT_REAR_RIGHT;
			ePanels wingEnum = isLeft ? ePanels::WING_REAR_LEFT : ePanels::WING_REAR_RIGHT;
			if (Util::IsLightDamaged(pVeh, lightEnum) || Util::IsPanelDamaged(pVeh, wingEnum))
			{
				continue;
			}

			for (auto e : m_Dummies[pVeh][type])
			{
				e->Update();
				RenderUtil::RegisterPointLight(&e->Get(), e->Get().corona.color, 3.5f, true);
			}
		}
	}

	// 5. Turn Indicators
	if (data.m_nIndicatorState != eIndicatorState::Off)
	{
		if (indicatorsDelay)
		{
			if (data.m_nIndicatorState == eIndicatorState::LeftOn || data.m_nIndicatorState == eIndicatorState::BothOn)
			{
				for (eMaterialType type : {eMaterialType::IndicatorLightLeftFront, eMaterialType::IndicatorLightLeftRear, eMaterialType::IndicatorLightLeftMiddle, eMaterialType::NABrakeLightLeft})
				{
					if (!IsDummyAvail(pVeh, type) || !GetLightState(pVeh, type))
					{
						continue;
					}

					bool isRear = (type == eMaterialType::IndicatorLightLeftRear || type == eMaterialType::NABrakeLightLeft);
					bool isFront = (type == eMaterialType::IndicatorLightLeftFront);
					bool isMiddle = (type == eMaterialType::IndicatorLightLeftMiddle);

					if (isFront && !pVeh->bSirenOrAlarm && (Util::IsLightDamaged(pVeh, eLights::LIGHT_FRONT_LEFT) || Util::IsPanelDamaged(pVeh, ePanels::WING_FRONT_LEFT)))
						continue;
					if (isRear && (Util::IsLightDamaged(pVeh, eLights::LIGHT_REAR_LEFT) || Util::IsPanelDamaged(pVeh, ePanels::WING_REAR_LEFT)))
						continue;
					if (isMiddle && Util::IsPanelDamaged(pVeh, ePanels::WING_FRONT_LEFT))
						continue;

					for (auto e : m_Dummies[pVeh][type])
					{
						e->Update();
						RenderUtil::RegisterPointLight(&e->Get(), e->Get().corona.color, 1.40f, true);
					}
				}
			}

			if (data.m_nIndicatorState == eIndicatorState::RightOn || data.m_nIndicatorState == eIndicatorState::BothOn)
			{
				for (eMaterialType type : {eMaterialType::IndicatorLightRightFront, eMaterialType::IndicatorLightRightRear, eMaterialType::IndicatorLightRightMiddle, eMaterialType::NABrakeLightRight})
				{
					if (!IsDummyAvail(pVeh, type) || !GetLightState(pVeh, type))
					{
						continue;
					}

					bool isRear = (type == eMaterialType::IndicatorLightRightRear || type == eMaterialType::NABrakeLightRight);
					bool isFront = (type == eMaterialType::IndicatorLightRightFront);
					bool isMiddle = (type == eMaterialType::IndicatorLightRightMiddle);

					if (isFront && !pVeh->bSirenOrAlarm && (Util::IsLightDamaged(pVeh, eLights::LIGHT_FRONT_RIGHT) || Util::IsPanelDamaged(pVeh, ePanels::WING_FRONT_RIGHT)))
						continue;
					if (isRear && (Util::IsLightDamaged(pVeh, eLights::LIGHT_REAR_RIGHT) || Util::IsPanelDamaged(pVeh, ePanels::WING_REAR_RIGHT)))
						continue;
					if (isMiddle && Util::IsPanelDamaged(pVeh, ePanels::WING_FRONT_RIGHT))
						continue;

					for (auto e : m_Dummies[pVeh][type])
					{
						e->Update();
						RenderUtil::RegisterPointLight(&e->Get(), e->Get().corona.color, 1.40f, true);
					}
				}
			}
		}

		// Non-adapted Global Indicators (rendered during !indicatorsDelay when taillight is illuminated)
		if (!indicatorsDelay && data.m_bUsingGlobalIndicators && !IsMatAvail(pVeh, INDICATOR_LIGHTS_TYPE))
		{
			for (bool isLeft : {true, false})
			{
				bool isActive = isLeft ? (data.m_nIndicatorState == eIndicatorState::LeftOn || data.m_nIndicatorState == eIndicatorState::BothOn)
				                       : (data.m_nIndicatorState == eIndicatorState::RightOn || data.m_nIndicatorState == eIndicatorState::BothOn);
				if (!isActive)
					continue;

				eLights lightEnum = isLeft ? eLights::LIGHT_REAR_LEFT : eLights::LIGHT_REAR_RIGHT;
				ePanels wingEnum = isLeft ? ePanels::WING_REAR_LEFT : ePanels::WING_REAR_RIGHT;
				if (Util::IsLightDamaged(pVeh, lightEnum) || Util::IsPanelDamaged(pVeh, wingEnum))
					continue;

				eMaterialType indType = isLeft ? eMaterialType::IndicatorLightLeftRear : eMaterialType::IndicatorLightRightRear;
				eMaterialType naType = isLeft ? eMaterialType::NABrakeLightLeft : eMaterialType::NABrakeLightRight;
				if (IsDummyAvail(pVeh, {indType, naType}))
					continue;

				for (eMaterialType t : {isLeft ? eMaterialType::TailLightLeft : eMaterialType::TailLightRight, isLeft ? eMaterialType::BrakeLightLeft : eMaterialType::BrakeLightRight})
				{
					if (!IsDummyAvail(pVeh, t) && isBike)
					{
						if (IsDummyAvail(pVeh, eMaterialType::TailLightRight))
							t = eMaterialType::TailLightRight;
						else if (IsDummyAvail(pVeh, eMaterialType::TailLightLeft))
							t = eMaterialType::TailLightLeft;
					}
					if (IsDummyAvail(pVeh, t) && GetLightState(pVeh, t))
					{
						for (auto e : m_Dummies[pVeh][t])
						{
							e->Update();
							RenderUtil::RegisterPointLight(&e->Get(), e->Get().corona.color, 1.40f, true);
						}
						break;
					}
				}
			}
		}
	}

	// 6. Side Lights
	if (isHeadlightsOn)
	{
		for (eMaterialType type : {eMaterialType::SideLightLeft, eMaterialType::SideLightRight})
		{
			if (!IsDummyAvail(pVeh, type) || !GetLightState(pVeh, type))
			{
				continue;
			}

			bool isLeft = (type == eMaterialType::SideLightLeft);
			ePanels wingEnum = isLeft ? ePanels::WING_FRONT_LEFT : ePanels::WING_FRONT_RIGHT;
			ePanels rearWingEnum = isLeft ? ePanels::WING_REAR_LEFT : ePanels::WING_REAR_RIGHT;
			if (Util::IsPanelDamaged(pVeh, wingEnum) || Util::IsPanelDamaged(pVeh, rearWingEnum))
			{
				continue;
			}

			for (auto e : m_Dummies[pVeh][type])
			{
				e->Update();
				RenderUtil::RegisterPointLight(&e->Get(), e->Get().corona.color, 1.5f, true);
			}
		}
	}

	// 7. AllDay, Day, Night DRL Lights
	if (IsDummyAvail(pVeh, eMaterialType::AllDayLight) && GetLightState(pVeh, eMaterialType::AllDayLight))
	{
		for (auto e : m_Dummies[pVeh][eMaterialType::AllDayLight])
		{
			e->Update();
			RenderUtil::RegisterPointLight(&e->Get(), e->Get().corona.color, 0.85f, true);
		}
	}

	if (!Util::IsNightTime() && IsDummyAvail(pVeh, eMaterialType::DayLight) && GetLightState(pVeh, eMaterialType::DayLight))
	{
		for (auto e : m_Dummies[pVeh][eMaterialType::DayLight])
		{
			e->Update();
			RenderUtil::RegisterPointLight(&e->Get(), e->Get().corona.color, 0.85f, true);
		}
	}

	if (Util::IsNightTime() && IsDummyAvail(pVeh, eMaterialType::NightLight) && GetLightState(pVeh, eMaterialType::NightLight))
	{
		for (auto e : m_Dummies[pVeh][eMaterialType::NightLight])
		{
			e->Update();
			RenderUtil::RegisterPointLight(&e->Get(), e->Get().corona.color, 0.85f, true);
		}
	}

	// 8. Strobe Lights
	if (IsDummyAvail(pVeh, eMaterialType::StrobeLight) && GetLightState(pVeh, eMaterialType::StrobeLight))
	{
		for (auto e : m_Dummies[pVeh][eMaterialType::StrobeLight])
		{
			const DummyConfig &c = e->GetRef();
			if (c.strobe.enabled)
			{
				e->Update();
				RenderUtil::RegisterPointLight(&e->Get(), c.corona.color, 6.0f, true);
			}
		}
	}
}

bool Lights::IsIndicatorOn(CVehicle *pVeh)
{
	if (!pVeh || pVeh->m_fHealth <= 0.0f) return false;
	if (pVeh->m_nVehicleSubClass != VEHICLE_AUTOMOBILE && pVeh->m_nVehicleSubClass != VEHICLE_BIKE && pVeh->m_nVehicleSubClass != VEHICLE_QUAD && pVeh->m_nVehicleSubClass != VEHICLE_MTRUCK) return false;

	if (gConfig.ReadBoolean("LIGHTS", "StandardLightsv2", gConfig.ReadBoolean("FEATURES", "StandardLightsv2", false))) {
		return LightManager::IsIndicatorOn(pVeh);
	}
	return indicatorsDelay && m_VehData.Get(pVeh).m_nIndicatorState != eIndicatorState::Off;
}

VehLightDatav1 Lights::GetVehicleData(CVehicle *pVeh)
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

#include "ModelExtrasAPI.h"

extern "C"
{
	bool ME_GetVehicleLightState(CVehicle *pVeh, ME_LightID lightId)
	{
		if (gConfig.ReadBoolean("LIGHTS", "StandardLightsv2", gConfig.ReadBoolean("FEATURES", "StandardLightsv2", false))) {
			return LightManager::GetLightState(pVeh, static_cast<eMaterialType>(lightId));
		}
		return Lights::GetLightState(pVeh, static_cast<eMaterialType>(lightId));
	}

	void ME_SetVehicleLightState(CVehicle *pVeh, ME_LightID lightId, bool state)
	{
		if (gConfig.ReadBoolean("LIGHTS", "StandardLightsv2", gConfig.ReadBoolean("FEATURES", "StandardLightsv2", false))) {
			LightManager::SetLightState(pVeh, static_cast<eMaterialType>(lightId), state);
			return;
		}
		Lights::SetLightState(pVeh, static_cast<eMaterialType>(lightId), state);
	}

	// Dummy function to show on crash logs
	int __declspec(dllexport) ignore4(int i)
	{
		return 1;
	}
}