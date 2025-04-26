#include "pch.h"
#include "lights.h"
#include <CClock.h>
#include "core/common.h"
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

// flags
bool gbGlobalIndicatorLights = false;
bool gbGlobalReverseLights = false;
float gfGlobalCoronaSize = 0.3f;
int gGlobalCoronaIntensity = 80;
int gGlobalShadowIntensity = 80;

bool IsNightTime()
{
	return CClock::GetIsTimeInRange(20, 7);
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

void DrawGlobalLight(CVehicle *pVeh, eDummyPos pos, CRGBA col, std::string texture = "indicator", CVector2D shdwSz = {1.0F, 1.0F}, CVector2D shdwOffset = {0.0F, 0.0F})
{
	if (pVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE)
	{
		CAutomobile *ptr = reinterpret_cast<CAutomobile *>(pVeh);
		if ((pos == eDummyPos::FrontLeft && ptr->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_LEFT)) || (pos == eDummyPos::FrontRight && ptr->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_RIGHT)) || (pos == eDummyPos::RearLeft && ptr->m_damageManager.GetLightStatus(eLights::LIGHT_REAR_LEFT)) || (pos == eDummyPos::RearRight && ptr->m_damageManager.GetLightStatus(eLights::LIGHT_REAR_RIGHT)))
		{
			return;
		}
	}

	int idx = (pos == eDummyPos::RearLeft) || (pos == eDummyPos::RearRight);
	bool leftSide = (pos == eDummyPos::RearLeft) || (pos == eDummyPos::FrontLeft);

	CVector posn =
		reinterpret_cast<CVehicleModelInfo *>(CModelInfo__ms_modelInfoPtrs[pVeh->m_nModelIndex])->m_pVehicleStruct->m_avDummyPos[idx];

	if (posn.x == 0.0f)
		posn.x = 0.15f;
	if (leftSide)
		posn.x *= -1.0f;
	int dummyId = static_cast<int>(idx) + (leftSide ? 0 : 2);
	float dummyAngle = (pos == eDummyPos::RearLeft || pos == eDummyPos::RearRight) ? 180.0f : 0.0f;
	CRGBA color = {col.r, col.g, col.b, GetShadowAlphaForDayTime()};
	Common::RegisterShadow(pVeh, posn, color, dummyAngle, 0.0f, texture, shdwSz, shdwOffset);
	Common::RegisterCoronaWithAngle(pVeh, (reinterpret_cast<unsigned int>(pVeh) * 255) + 255 + int(pos), posn, {col.r, col.g, col.b, GetCoronaAlphaForDayTime()}, dummyAngle, 180.0f, gfGlobalCoronaSize);
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

// Shadows
unsigned int HEADLIGHT_SHADOW_ALPHA = 240;
float HEADLIGHT_SHADOW_WIDTH_BIKE = 2.75f;
float HEADLIGHT_SHADOW_WIDTH = 240;

// Coronas
float HEADLIGHT_CORONA_SIZE_SHORT = 0.075f;
float HEADLIGHT_CORONA_SIZE_LONG = 0.115f;

int HEADLIGHT_CORONA_ALPHA_SHORT = 230;
int HEADLIGHT_CORONA_ALPHA_LONG = 255;

void __cdecl Lights::hkTailLightCCoronas_RegisterCorona(uint32_t id, CVehicle *pVeh, uint8_t r, uint8_t g, uint8_t b, uint8_t a, CVector *pos, float size, float range, int coronaType, uint8_t flareType, uint8_t reflectionType, bool checkObstacles, int bUsesTrails, float fNormalAngle, bool bNeonFade, float fPullTowardsCam, bool bFullBrightAtStart, float fadeSpeed, bool bOnlyFromBelow, bool bWhiteCore)
{
	if (pVeh->m_nOverrideLights == eLightOverride::ForceLightsOff)
	{
		return;
	}

	bool breakLightInstalled = IsDummyAvail(pVeh, eLightState::Brakelight);

	if (!breakLightInstalled || IsTailLightOn(pVeh))
	{
		pos->y -= 0.05f;
		Common::RegisterCorona(pVeh, id, *pos, {r, g, b, GetCoronaAlphaForDayTime()}, size * 3.0f);
	}
}

void __cdecl hkCShadows_StoreCarLightShadow(CVehicle *pVeh, int32_t id, RwTexture *pTex, CVector *shadowPos, float fwdX, float fwdY, float sideX, float sideY, uint8_t red, uint8_t green, uint8_t blue, float radius)
{
	CShadows::StoreCarLightShadow(pVeh, id, pTex, shadowPos, fwdX, fwdY, sideX * 1.5f, sideY * 1.5f, red, green, blue, radius);
}

void Lights::Initialize()
{
	patch::ReplaceFunctionCall(0x6E1A2D, hkTailLightCCoronas_RegisterCorona);

	patch::ReplaceFunctionCall(0x6E15E2, hkCShadows_StoreCarLightShadow);
	patch::ReplaceFunctionCall(0x6E170F, hkCShadows_StoreCarLightShadow);
	patch::SetPointer(0x70C6CB, &HEADLIGHT_SHADOW_ALPHA);
	patch::SetPointer(0x70C72D, &HEADLIGHT_SHADOW_ALPHA);

	// headlight distance fix
	patch::SetFloat(0x872748, 50000);
	patch::SetFloat(0x872740, 0.0011f);

	patch::SetUInt(0x6E0CF8, 0xC0); // Decrease inner corona alpha a bit

	static RwTexture *hss = nullptr;
	static RwTexture *hsl = nullptr;
	static RwTexture *hts = nullptr;
	static RwTexture *htl = nullptr;

	plugin::Events::initGameEvent += []()
	{
		hss = Util::LoadTextureFromFile(MOD_DATA_PATH_S(std::string("textures/headlight_single_short.png")), 255);
		hsl = Util::LoadTextureFromFile(MOD_DATA_PATH_S(std::string("textures/headlight_single_long.png")), 255);
		hts = Util::LoadTextureFromFile(MOD_DATA_PATH_S(std::string("textures/headlight_twin_short.png")), 255);
		htl = Util::LoadTextureFromFile(MOD_DATA_PATH_S(std::string("textures/headlight_twin_long.png")), 255);

		gbGlobalIndicatorLights = gConfig.ReadBoolean("VEHICLE_FEATURES", "StandardLights_GlobalIndicatorLights", false);
		gbGlobalReverseLights = gConfig.ReadBoolean("VEHICLE_FEATURES", "StandardLights_GlobalReverseLights", false);
		gfGlobalCoronaSize = gConfig.ReadFloat("VEHICLE_FEATURES", "StandardLights_GlobalCoronaSize", 0.3f);
		gGlobalShadowIntensity = gConfig.ReadInteger("VEHICLE_FEATURES", "StandardLights_GlobalShadowIntensity", 220);
		gGlobalCoronaIntensity = gConfig.ReadInteger("VEHICLE_FEATURES", "StandardLights_GlobalCoronaIntensity", 250);
	};

	static FastcallEvent<AddressList<0x6E1A76, H_CALL>, PRIORITY_BEFORE,
						 ArgPick2N<CVehicle *, 0, int, 1>, void(CVehicle *, int)>
		DoHeadLightEvent;

	DoHeadLightEvent += [](CVehicle *pVeh, int b)
	{
		bool isFoggy = (CWeather::NewWeatherType == WEATHER_FOGGY_SF || CWeather::NewWeatherType == WEATHER_SANDSTORM_DESERT || CWeather::OldWeatherType == WEATHER_FOGGY_SF || CWeather::OldWeatherType == WEATHER_SANDSTORM_DESERT);
		VehData &data = m_VehData.Get(pVeh);
		if (data.m_bLongLightsOn)
		{
			plugin::patch::SetPointer(0x6E1693, htl);															 // Twin
			plugin::patch::SetPointer(0x6E151D, hsl);															 // Single
			patch::SetFloat(0x6E0CA6, isFoggy ? 1.5f * HEADLIGHT_CORONA_SIZE_LONG : HEADLIGHT_CORONA_SIZE_LONG); // HeadLightCoronaSize
			patch::SetUInt(0x6E0DEE, HEADLIGHT_CORONA_ALPHA_LONG);												 // HeadLightCoronaAlpha
		}
		else
		{
			plugin::patch::SetPointer(0x6E1693, hts); // Twin
			plugin::patch::SetPointer(0x6E151D, hss); // Single
			patch::SetFloat(0x6E0CA6, isFoggy ? 1.5f * HEADLIGHT_CORONA_SIZE_SHORT : HEADLIGHT_CORONA_SIZE_SHORT);

			patch::SetUInt(0x6E0DEE, isFoggy ? HEADLIGHT_CORONA_ALPHA_LONG : HEADLIGHT_CORONA_ALPHA_SHORT); // HeadLightCoronaAlpha
		}
	};

	Events::vehicleDtorEvent += [](CVehicle *pVeh)
	{
		m_Dummies.erase(pVeh);
	};

	VehicleMaterials::Register([](CVehicle *vehicle, RpMaterial *material, CRGBA col)
							   {
		eDummyPos pos = eDummyPos::None;
		if ((col.r == 255 && col.g == 173 && col.b == 0) || (col.r == 0 && col.g == 255 && col.b == 198))
			RegisterMaterial(vehicle, material, eLightState::Reverselight, col);
		else if ((col.r == 184 && col.g == 255 && col.b == 0) || (col.r == 255 && col.g == 59 && col.b == 0))
			RegisterMaterial(vehicle, material, eLightState::Brakelight, col);
		else if ((col.r == 0 && col.g == 16 && col.b == 255)
		|| (col.r == 255 && col.g == 8 && col.b == 128))
			RegisterMaterial(vehicle, material, eLightState::Nightlight, col);
		else if ((col.r == 0 && col.g == 17 && col.b == 255)
		|| (col.r == 255 && col.g == 9 && col.b == 128))
			RegisterMaterial(vehicle, material, eLightState::AllDayLight, col);
		else if ((col.r == 0 && col.g == 18 && col.b == 255)
		|| (col.r == 255 && col.g == 7 && col.b == 128))
			RegisterMaterial(vehicle, material, eLightState::Daylight, col);
		else if ((col.r == 255 && col.g == 174 && col.b == 0) || (col.r == 0 && col.g == 255 && col.b == 199))
			RegisterMaterial(vehicle, material, eLightState::FogLight, col);
		else if ((col.r == 255 && col.g == 175 && col.b == 0)
		|| (col.r == 255 && col.g == 1 && col.b == 128))
			RegisterMaterial(vehicle, material, eLightState::FrontLightLeft, col);
		else if ((col.r == 0 && col.g == 255 && col.b == 200)
			|| (col.r == 255 && col.g == 2 && col.b == 128))
			RegisterMaterial(vehicle, material, eLightState::FrontLightRight, col);
		else if ((col.r == 255 && col.g == 60 && col.b == 0) || col.r == 185 && col.g == 255 && col.b == 0)
			RegisterMaterial(vehicle, material, eLightState::TailLight, col);
		else if (col.r == 255 && col.g == 200 && col.b == 1) {
			RegisterMaterial(vehicle, material, eLightState::SideLightLeft, col);
			pos = eDummyPos::Left;
		}
		else if (col.r == 255 && col.g == 200 && col.b == 2) {
			RegisterMaterial(vehicle, material, eLightState::SideLightRight, col);
			pos = eDummyPos::Right;
		}
		else if (col.r == 255 && col.g == 200 && col.b == 3) {
			RegisterMaterial(vehicle, material, eLightState::STTLightLeft, col);
		}
		else if (col.r == 255 && col.g == 200 && col.b == 4) {
			RegisterMaterial(vehicle, material, eLightState::STTLightRight, col);
		}
		else if (col.r == 255 && col.g == 200 && col.b == 5) {
			RegisterMaterial(vehicle, material, eLightState::NABrakeLightLeft, col);
		}
		else if (col.r == 255 && col.g == 200 && col.b == 6) {
			RegisterMaterial(vehicle, material, eLightState::NABrakeLightRight, col);
		}
		else if (col.r == 255 && col.g == 200 && col.b == 7) {
			RegisterMaterial(vehicle, material, eLightState::SpotLight, col);
		}
		else if (col.r == 255 && col.g == 199) { // 1-255 reserved
			RegisterMaterial(vehicle, material, eLightState::StrobeLight, col);
		}

		// Indicator Lights
		if (col.b == 0) {
			if (col.r == 255) { // Right
				if (col.g >= 56 && col.g <= 58) {
					if (col.g == 58) {
						pos = eDummyPos::FrontRight;
					}
					else if (col.g == 57) {
						pos = eDummyPos::Right;
					}
					else if (col.g == 56) {
						pos = eDummyPos::RearRight;
					}
					RegisterMaterial(vehicle, material, eLightState::IndicatorRight, col, pos);
				}
			}
			else if (col.g == 255) { // Left
				if (col.r >= 181 && col.r <= 183) {
					if (col.r == 183) {
						pos = eDummyPos::FrontLeft;
					}
					else if (col.r == 182) {
						pos = eDummyPos::Left;
					}
					else if (col.r == 181) {
						pos = eDummyPos::RearLeft;
					}
					RegisterMaterial(vehicle, material, eLightState::IndicatorLeft, col, pos);
				}
			}
		}

		if (col.r == 255 && (col.g == 4 || col.g == 5) && col.b == 128
		&& std::string(material->texture->name).rfind("light", 0) == 0) {
			RegisterMaterial(vehicle, material, (col.g == 4) ? eLightState::IndicatorLeft : eLightState::IndicatorRight, col);
		}

		return material; });

	VehicleMaterials::RegisterDummy([](CVehicle *pVeh, RwFrame *frame, std::string name, bool parent)
									{
		eLightState state = eLightState::None;
		RwRGBA col{255, 255, 255, GetCoronaAlphaForDayTime()};
		eDummyPos dummyPos = eDummyPos::None;
		std::smatch match;
		size_t dummyIdx = 0;
		bool directioanlByDef = false;
		if (std::regex_search(name, match, std::regex("^fogl(ight)?_([lr]).*$")))
		{
			state = eLightState::FogLight;
			dummyPos = eDummyPos::Front;
			directioanlByDef = true;
		}
		else if (std::regex_search(name, std::regex(R"(^rev.*\s*_[lr].*$)")))
		{
			state = eLightState::Reverselight;
			dummyPos = eDummyPos::Rear;
			directioanlByDef = true;
		}
		else if (std::regex_search(name, std::regex(R"(^breakl.*\s*_[lr].*$)")))
		{
			state = eLightState::Brakelight;
			dummyPos = eDummyPos::Rear;
			col = {240, 0, 0, GetCoronaAlphaForDayTime()};
			directioanlByDef = true;
		}
		else if (std::regex_search(name, std::regex("^light_day")))
		{
			state = eLightState::Daylight;
			dummyPos = eDummyPos::Front;
		}
		else if (std::regex_search(name, std::regex("^light_nigh")))
		{
			state = eLightState::Nightlight;
			dummyPos = eDummyPos::Front;
		}
		else if (std::regex_search(name, match, std::regex(R"(^strobe_light(\d*)?)")))
		{
			if (match.str(1).size() != 0) {
				dummyIdx = match.str(1)[0] - '0';
			}
			dummyPos = eDummyPos::Front;
			state = eLightState::StrobeLight;
		}
		else if (std::regex_search(name, match, std::regex("^sidelight?_([lr]).*$")))
		{
			if (toupper(match.str(1)[0]) == 'L')
			{
				state = eLightState::SideLightLeft;
				dummyPos = eDummyPos::Left;
			}
			else
			{
				state = eLightState::SideLightRight;
				dummyPos = eDummyPos::Right;
			}
		}
		else if (std::regex_search(name, match, std::regex("^sttlight?_([lr]).*$")))
		{
			if (toupper(match.str(1)[0]) == 'L')
			{
				state = eLightState::STTLightLeft;
				dummyPos = eDummyPos::RearLeft;
			}
			else
			{
				state = eLightState::STTLightRight;
				dummyPos = eDummyPos::RearRight;
			}
			directioanlByDef = true;
			col = {240, 0, 0, GetCoronaAlphaForDayTime()};
		}
		else if (std::regex_search(name, match, std::regex("^nabrakelight?_([lr]).*$")))
		{
			if (toupper(match.str(1)[0]) == 'L')
			{
				state = eLightState::NABrakeLightLeft;
				dummyPos = eDummyPos::RearLeft;
			}
			else
			{
				state = eLightState::NABrakeLightRight;
				dummyPos = eDummyPos::RearRight;
			}
			directioanlByDef = true;
			col = {240, 0, 0, GetCoronaAlphaForDayTime()};
		}
		else if (std::regex_search(name, match, std::regex("^spotlight_light.*$")))
		{
			state = eLightState::SpotLight;
		}
		else if (std::regex_search(name, std::regex("^light_allday")))
		{
			state = eLightState::AllDayLight;
			dummyPos = eDummyPos::Front;
		}
		else if (std::regex_search(name, std::regex("taillights.*$")))
		{
			col = {250, 0, 0, GetCoronaAlphaForDayTime()};
			state = eLightState::TailLight;
			dummyPos = eDummyPos::Rear;
			VehicleDummy *pDummy = new VehicleDummy(pVeh, frame, name, dummyPos, col, dummyIdx, directioanlByDef);
			pDummy->mirroredX = true;
			m_Dummies[pVeh][state].push_back(pDummy);
		}
		else if (std::regex_search(name, match, std::regex("^(turnl_|indicator_)(.{2})")))
		{
			std::string stateStr = match.str(2);
			state = (toupper(stateStr[0]) == 'L') ? eLightState::IndicatorLeft : eLightState::IndicatorRight;
			col = {255, 128, 0, GetCoronaAlphaForDayTime()};

			if (toupper(stateStr[1]) == 'F')
			{
				dummyPos = state == eLightState::IndicatorRight ? eDummyPos::FrontRight : eDummyPos::FrontLeft;
			}
			else if (toupper(stateStr[1]) == 'R')
			{
				dummyPos = state == eLightState::IndicatorRight ? eDummyPos::RearRight : eDummyPos::RearLeft;
			}
			else if (toupper(stateStr[1]) == 'M')
			{
				dummyPos = state == eLightState::IndicatorRight ? eDummyPos::Right : eDummyPos::Left;
			}
			directioanlByDef = true;
		}
		else
		{
			return;
		}

		m_Dummies[pVeh][state].push_back(new VehicleDummy(pVeh, frame, name, dummyPos, col, dummyIdx, directioanlByDef)); });

	// Events::processScriptsEvent += []()
	// {
	// 	if (KeyPressed(VK_J))
	// 	{
	// 		VehicleMaterials::FindDummies(FindPlayerVehicle(0, false), (RwFrame *)FindPlayerVehicle(0, false)->m_pRwClump->object.parent, false, true);
	// 	}
	// };

	Events::processScriptsEvent += []()
	{
		size_t timestamp = CTimer::m_snTimeInMilliseconds;
		if ((timestamp - delay) > 500)
		{
			delay = timestamp;
			indicatorsDelay = !indicatorsDelay;
		}

		CVehicle *pVeh = FindPlayerVehicle(-1, false);
		if (pVeh && pVeh->m_nOverrideLights != eLightOverride::ForceLightsOff)
		{
			static size_t prev = 0;
			static uint32_t fogLightKey = gConfig.ReadInteger("KEYS", "FogLightKey", VK_J);
			if (KeyPressed(fogLightKey) && IsMatAvail(pVeh, eLightState::FogLight))
			{
				size_t now = CTimer::m_snTimeInMilliseconds;
				if (now - prev > 500.0f)
				{
					VehData &data = m_VehData.Get(pVeh);
					data.m_bFogLightsOn = !data.m_bFogLightsOn;
					prev = now;
					AudioMgr::PlayClickSound();
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
					AudioMgr::PlayClickSound();
				}
			}
		}
	};

	VehicleMaterials::RegisterRender([](CVehicle *pControlVeh)
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
		eLightState indState = data.m_nIndicatorState;

		if (pControlVeh->m_fHealth == 0 || IsEngineOff(pControlVeh))
		{
			return;
		}

		CAutomobile *pAutoMobile = reinterpret_cast<CAutomobile *>(pControlVeh);
		RenderLights(pControlVeh, pTowedVeh, eLightState::AllDayLight);
		RenderLights(pControlVeh, pTowedVeh, eLightState::StrobeLight);

		if (IsNightTime())
		{
			RenderLights(pControlVeh, pTowedVeh, eLightState::Nightlight);
		}
		else
		{
			RenderLights(pControlVeh, pTowedVeh, eLightState::Daylight);
		}

		if (data.m_bFogLightsOn)
		{
			RenderLights(pControlVeh, pTowedVeh, eLightState::FogLight, true, "foglight", {1.5f, 2.0f}, {0.0f, 1.0f});
		}

		if (pControlVeh->m_nVehicleFlags.bLightsOn)
		{
			bool leftOk = true;
			bool rightOk = true;
			if (CModelInfo::IsCarModel(pControlVeh->m_nModelIndex))
			{
				leftOk = pAutoMobile->m_renderLights.m_bLeftFront && !pAutoMobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_LEFT);
				rightOk = pAutoMobile->m_renderLights.m_bRightFront && !pAutoMobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_RIGHT);
			}

			if (leftOk)
			{
				RenderLights(pControlVeh, pTowedVeh, eLightState::FrontLightLeft);
			}

			if (rightOk)
			{
				RenderLights(pControlVeh, pTowedVeh, eLightState::FrontLightRight);
			}
		}

		if (SpotLights::IsEnabled(pControlVeh))
		{
			RenderLights(pControlVeh, pTowedVeh, eLightState::SpotLight, false);
		}

		if (IsNightTime())
		{
			RenderLights(pControlVeh, pTowedVeh, eLightState::Nightlight);
			RenderLights(pControlVeh, pTowedVeh, eLightState::SideLightLeft);
			RenderLights(pControlVeh, pTowedVeh, eLightState::SideLightRight);
		}
		else
		{
			RenderLights(pControlVeh, pTowedVeh, eLightState::Daylight);
		}

		RenderLights(pControlVeh, pTowedVeh, eLightState::AllDayLight);

		bool isBike = CModelInfo::IsBikeModel(pControlVeh->m_nModelIndex);
		std::string shdwName = (isBike ? "taillight_bike" : "taillight");
		CVector2D shdwOff = {0.0f, (isBike ? 0.5f : 1.0f)};

		CVector2D shdwSz = {1.0f, 1.5f};
		if (isBike || CModelInfo::IsCarModel(pControlVeh->m_nModelIndex))
		{
			bool isRevlightSupportedByModel = IsMatAvail(pTowedVeh, eLightState::Reverselight);

			bool reverseLightsOn = !isBike && (isRevlightSupportedByModel || gbGlobalReverseLights) && pControlVeh->m_nCurrentGear == 0 && (Util::GetVehicleSpeed(pControlVeh) >= 0.001f) && pControlVeh->m_pDriver;
			if (reverseLightsOn)
			{
				if (isRevlightSupportedByModel)
				{
					RenderLights(pControlVeh, pTowedVeh, eLightState::Reverselight, true, shdwName, shdwSz, shdwOff);
				}
				else
				{
					DrawGlobalLight(pTowedVeh, eDummyPos::RearLeft, {240, 240, 240, 0}, shdwName, shdwSz, shdwOff);
					DrawGlobalLight(pTowedVeh, eDummyPos::RearRight, {240, 240, 240, 0}, shdwName, shdwSz, shdwOff);
				}
			}

			bool sttInstalled = IsMatAvail(pTowedVeh, eLightState::STTLightLeft) || IsMatAvail(pTowedVeh, eLightState::STTLightRight);
			// taillights/ brakelights
			if (pControlVeh->m_fBreakPedal && pControlVeh->m_pDriver)
			{
				if (sttInstalled) {
					RenderLights(pControlVeh, pTowedVeh, eLightState::STTLightLeft, true, shdwName, shdwSz, shdwOff);
					RenderLights(pControlVeh, pTowedVeh, eLightState::STTLightRight, true, shdwName, shdwSz, shdwOff);
				} else {
					if (IsMatAvail(pTowedVeh, eLightState::Brakelight))
					{
						RenderLights(pControlVeh, pTowedVeh, eLightState::Brakelight, true, shdwName, shdwSz, shdwOff);
					}
					else if (IsMatAvail(pTowedVeh, eLightState::TailLight))
					{
						RenderLights(pControlVeh, pTowedVeh, eLightState::TailLight, true, shdwName, shdwSz, shdwOff);
					}
				}

				if (indState != eLightState::IndicatorBoth)
				{
					if (indState != eLightState::IndicatorLeft)
					{
						RenderLights(pControlVeh, pTowedVeh, eLightState::NABrakeLightLeft, true, shdwName, shdwSz, shdwOff);
					}

					if (indState != eLightState::IndicatorRight)
					{
						RenderLights(pControlVeh, pTowedVeh, eLightState::NABrakeLightRight, true, shdwName, shdwSz, shdwOff);
					}
				}
			}

			if (IsTailLightOn(pControlVeh))
			{
				if (sttInstalled) {
					RenderLights(pControlVeh, pTowedVeh, eLightState::STTLightLeft, true, shdwName, shdwSz, shdwOff);
					RenderLights(pControlVeh, pTowedVeh, eLightState::STTLightRight, true, shdwName, shdwSz, shdwOff);
				}
				else {
					if (IsMatAvail(pTowedVeh, eLightState::TailLight))
					{
						RenderLights(pControlVeh, pTowedVeh, eLightState::TailLight, !sttInstalled, shdwName, shdwSz, shdwOff);
					}
					else if (IsMatAvail(pTowedVeh, eLightState::Brakelight))
					{
						RenderLights(pControlVeh, pTowedVeh, eLightState::Brakelight, !sttInstalled, shdwName, shdwSz, shdwOff);
					}
				}
			}
		}

			// Indicator Lights
			if (!gbGlobalIndicatorLights && !IsDummyAvail(pControlVeh, eLightState::IndicatorLeft) && !IsMatAvail(pControlVeh, eLightState::IndicatorLeft))
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
					data.m_nIndicatorState = eLightState::IndicatorNone;
					delay = 0;
					indicatorsDelay = false;
				}

				if (KeyPressed(indicatorLeftKey))
				{
					data.m_nIndicatorState = eLightState::IndicatorLeft;
				}

				if (KeyPressed(indicatorRightKey))
				{
					data.m_nIndicatorState = eLightState::IndicatorRight;
				}

				if (KeyPressed(indicatorBothKey))
				{
					data.m_nIndicatorState = eLightState::IndicatorBoth;
				}
			}
			else if (pControlVeh->m_pDriver)
			{
				data.m_nIndicatorState = eLightState::IndicatorNone;
				CVector2D prevPoint = GetCarPathLinkPosition(pControlVeh->m_autoPilot.m_nPreviousPathNodeInfo);
				CVector2D currPoint = GetCarPathLinkPosition(pControlVeh->m_autoPilot.m_nCurrentPathNodeInfo);
				CVector2D nextPoint = GetCarPathLinkPosition(pControlVeh->m_autoPilot.m_nNextPathNodeInfo);

				float angle = GetZAngleForPoint(nextPoint - currPoint) - GetZAngleForPoint(currPoint - prevPoint);
				while (angle < 0.0f)
					angle += 360.0f;
				while (angle > 360.0f)
					angle -= 360.0f;

				if (angle >= 30.0f && angle < 180.0f)
					data.m_nIndicatorState = eLightState::IndicatorLeft;
				else if (angle <= 330.0f && angle > 180.0f)
					data.m_nIndicatorState = eLightState::IndicatorRight;

				if (data.m_nIndicatorState == eLightState::IndicatorNone)
				{
					if (pControlVeh->m_autoPilot.m_nCurrentLane == 0 && pControlVeh->m_autoPilot.m_nNextLane == 1)
						data.m_nIndicatorState = eLightState::IndicatorRight;
					else if (pControlVeh->m_autoPilot.m_nCurrentLane == 1 && pControlVeh->m_autoPilot.m_nNextLane == 0)
						data.m_nIndicatorState = eLightState::IndicatorLeft;
				}
			}

			if (!indicatorsDelay || indState == eLightState::IndicatorNone)
			{
				return;
			}

			// global turn lights
			if (gbGlobalIndicatorLights &&
				((!IsDummyAvail(pControlVeh, eLightState::IndicatorLeft) && !IsDummyAvail(pControlVeh, eLightState::STTLightLeft)) || (!IsDummyAvail(pControlVeh, eLightState::IndicatorRight) && !IsDummyAvail(pControlVeh, eLightState::STTLightRight))) &&
				((!IsMatAvail(pControlVeh, eLightState::IndicatorLeft) && !IsMatAvail(pControlVeh, eLightState::STTLightLeft)) || (!IsMatAvail(pControlVeh, eLightState::IndicatorRight) && !IsMatAvail(pControlVeh, eLightState::STTLightRight))))
			{
				if ((pControlVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pControlVeh->m_nVehicleSubClass == VEHICLE_BIKE || pControlVeh->m_nVehicleSubClass == VEHICLE_QUAD) &&
					(pControlVeh->GetVehicleAppearance() == VEHICLE_APPEARANCE_AUTOMOBILE || pControlVeh->GetVehicleAppearance() == VEHICLE_APPEARANCE_BIKE) &&
					pControlVeh->m_nVehicleFlags.bEngineOn && pControlVeh->m_fHealth > 0 && !pControlVeh->m_nVehicleFlags.bIsDrowning && !pControlVeh->m_pAttachedTo)
				{
					if (DistanceBetweenPoints(TheCamera.m_vecGameCamPos, pControlVeh->GetPosition()) < 150.0f)
					{
						if (indState == eLightState::IndicatorBoth || indState == eLightState::IndicatorRight)
						{
							DrawGlobalLight(pControlVeh, eDummyPos::FrontRight, {255, 128, 0, 0}, "indicator", {1.0f, 1.0f}, shdwOff);
							DrawGlobalLight(pTowedVeh, eDummyPos::RearRight, {255, 128, 0, 0}, "indicator", {1.0f, 1.0f}, shdwOff);
						}
						if (indState == eLightState::IndicatorBoth || indState == eLightState::IndicatorLeft)
						{
							DrawGlobalLight(pControlVeh, eDummyPos::FrontLeft, {255, 128, 0, 0}, "indicator", {1.0f, 1.0f}, shdwOff);
							DrawGlobalLight(pTowedVeh, eDummyPos::RearLeft, {255, 128, 0, 0}, "indicator", {1.0f, 1.0f}, shdwOff);
						}
					}
				}
			}
			else
			{
				if (indState == eLightState::IndicatorBoth || indState == eLightState::IndicatorLeft)
				{
					RenderLights(pControlVeh, pTowedVeh, eLightState::IndicatorLeft, true, "indicator", {1.0f, 1.0f}, shdwOff);
					RenderLights(pControlVeh, pTowedVeh, eLightState::STTLightLeft, true, shdwName, shdwSz, shdwOff, true);
				}

				if (indState == eLightState::IndicatorBoth || indState == eLightState::IndicatorRight)
				{
					RenderLights(pControlVeh, pTowedVeh, eLightState::IndicatorRight, true, "indicator", {1.0f, 1.0f}, shdwOff);
					RenderLights(pControlVeh, pTowedVeh, eLightState::STTLightRight, true, shdwName, shdwSz, shdwOff, true);
				}
			}
			if (indState == eLightState::IndicatorBoth || indState == eLightState::IndicatorLeft)
			{
				RenderLights(pControlVeh, pTowedVeh, eLightState::NABrakeLightLeft, true, "indicator", {1.0f, 1.0f}, shdwOff);
			}

			if (indState == eLightState::IndicatorBoth || indState == eLightState::IndicatorRight)
			{
				RenderLights(pControlVeh, pTowedVeh, eLightState::NABrakeLightRight, true, "indicator", {1.0f, 1.0f}, shdwOff);
			} });
};

void Lights::RenderLight(CVehicle *pVeh, eLightState state, bool shadows, std::string texture, CVector2D sz, CVector2D offset, bool highlight)
{
	bool FrontDisabled = false;
	bool RearDisabled = false;
	bool MidDisabled = false;
	int id = static_cast<int>(state) * 10000;

	bool lightState[256] = {true};
	if (pVeh->m_pTrailer && (state == eLightState::IndicatorRight || state == eLightState::IndicatorLeft))
	{
		lightState[0] = true;
	}
	if (IsDummyAvail(pVeh, state))
	{
		for (auto e : m_Dummies[pVeh][state])
		{
			if (e->PartType != eParentType::Unknown && IsBumperOrWingDamaged(pVeh, e->PartType))
			{
				if ((e->DummyType == eDummyPos::FrontLeft || e->DummyType == eDummyPos::FrontRight || e->DummyType == eDummyPos::Front))
				{
					FrontDisabled = true;
				}
				if ((e->DummyType == eDummyPos::Left || e->DummyType == eDummyPos::Right))
				{
					MidDisabled = true;
				}
				if ((e->DummyType == eDummyPos::RearLeft || e->DummyType == eDummyPos::RearRight || e->DummyType == eDummyPos::Rear))
				{
					RearDisabled = true;
				}
				continue;
			}

			if (e->DummyType == eDummyPos::RearLeft || e->DummyType == eDummyPos::RearRight || e->DummyType == eDummyPos::Rear)
			{
				if (pVeh->m_pTrailer)
				{
					RearDisabled = true;
					continue;
				}
			}

			if (state == eLightState::StrobeLight)
			{
				size_t timer = CTimer::m_snTimeInMilliseconds;
				if (timer - e->strobeLightTimer > e->strobeDelay)
				{
					e->strobeLightOn = !e->strobeLightOn;
					e->strobeLightTimer = timer;
				}

				if (!e->strobeLightOn)
				{
					lightState[e->DummyIdx] = false;
					continue;
				}
			}

			e->Update(pVeh);

			EnableDummy((int)pVeh + 42 + id++, e, pVeh, highlight ? 1.5f : 1.0f);

			if (shadows)
			{
				texture = (e->shdwTex == "") ? texture : e->shdwTex;
				Common::RegisterShadow(pVeh, e->ShdwPosition, e->Color, e->Angle, e->CurrentAngle, texture, {sz.x * e->shdowSize.x, sz.y * e->shdowSize.y}, {offset.x + e->shdwOffSet.x, offset.y + e->shdwOffSet.y});
			}
		}
	}

	if (IsMatAvail(pVeh, state))
	{
		for (auto e : m_Materials[pVeh->m_nModelIndex][state])
		{
			if ((FrontDisabled && (e->Pos == eDummyPos::FrontLeft || e->Pos == eDummyPos::FrontRight || e->Pos == eDummyPos::Front)) || (RearDisabled && (e->Pos == eDummyPos::RearLeft || e->Pos == eDummyPos::RearRight || e->Pos == eDummyPos::Rear)) || (MidDisabled && (e->Pos == eDummyPos::Left || e->Pos == eDummyPos::Right)))
			{
				continue;
			}

			if (state == eLightState::StrobeLight && !lightState[e->Color.b])
			{
				continue;
			}

			EnableMaterial(e, highlight ? 2.0f : 1.0f);
		}
	}
}

void Lights::RenderLights(CVehicle *pControlVeh, CVehicle *pTowedVeh, eLightState state, bool shadows, std::string texture, CVector2D sz, CVector2D offset, bool highlight)
{
	RenderLight(pControlVeh, state, shadows, texture, sz, offset, highlight);

	if (pControlVeh != pTowedVeh)
	{
		RenderLight(pTowedVeh, state, shadows, texture, sz, offset, highlight);
	}
}

void Lights::RegisterMaterial(CVehicle *pVeh, RpMaterial *material, eLightState state, CRGBA col, eDummyPos pos)
{
	VehicleMaterial *mat = new VehicleMaterial(material, pos);
	m_Materials[pVeh->m_nModelIndex][state].push_back(mat);
	material->color.red = material->color.green = material->color.blue = 255;
};

void Lights::EnableDummy(int id, VehicleDummy *dummy, CVehicle *pVeh, float szMul)
{
	if (gConfig.ReadBoolean("VEHICLE_FEATURES", "LightCoronas", false))
	{
		if (dummy->LightType == eLightType::NonDirectional)
		{
			Common::RegisterCorona(pVeh, (reinterpret_cast<unsigned int>(pVeh) * 255) + 255 + id, dummy->Position, dummy->Color, dummy->coronaSize * szMul);
		}
		else
		{
			Common::RegisterCoronaWithAngle(pVeh, (reinterpret_cast<unsigned int>(pVeh) * 255) + 255 + id, dummy->Position, dummy->Color, dummy->Angle + (dummy->LightType == eLightType::Inversed ? 180.0f : 0.0f), 180.0f, dummy->coronaSize * szMul);
		}
	}
};

void Lights::EnableMaterial(VehicleMaterial *material, float mul)
{
	if (material && material->Material)
	{
		if (material->Material->surfaceProps.ambient >= AMBIENT_ON_VAL && material->Material->texture == material->TextureActive)
		{
			return; // skip if enabled already
		}

		VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int *>(&material->Material->surfaceProps.ambient), *reinterpret_cast<unsigned int *>(&material->Material->surfaceProps.ambient)));
		material->Material->surfaceProps.ambient = AMBIENT_ON_VAL * mul;
		VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int *>(&material->Material->texture), *reinterpret_cast<unsigned int *>(&material->Material->texture)));
		material->Material->texture = material->TextureActive;
	}
};

bool Lights::IsDummyAvail(CVehicle *pVeh, eLightState state)
{
	return m_Dummies[pVeh][state].size() != 0;
}

bool Lights::IsMatAvail(CVehicle *pVeh, eLightState state)
{
	return m_Materials[pVeh->m_nModelIndex][state].size() != 0;
}