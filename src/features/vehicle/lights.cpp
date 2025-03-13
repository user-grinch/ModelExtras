#include "pch.h"
#include "lights.h"
#include <CClock.h>
#include "avs/common.h"
#include "defines.h"
#include <CShadows.h>
#include <eVehicleClass.h>
#include <CCutsceneMgr.h>
#include <CAudioEngine.h>
#include <rwcore.h>
#include <rpworld.h>

bool IsNightTime() {
	return CClock::GetIsTimeInRange(20, 6);
}

unsigned int GetShadowAlphaForDayTime() {
	if (IsNightTime()) {
		return 250;
	}
	else {
		return 240;
	}
}

unsigned int GetCoronaAlphaForDayTime() {
	if (IsNightTime()) {
		return 250;
	}
	else {
		return 240;
	}
}

// Indicator lights
static uint64_t delay;
static bool delayState;

CVector2D GetCarPathLinkPosition(CCarPathLinkAddress& address) {
	if (address.m_nAreaId != -1 && address.m_nCarPathLinkId != -1 && ThePaths.m_pPathNodes[address.m_nAreaId]) {
		return CVector2D(static_cast<float>(ThePaths.m_pNaviNodes[address.m_nAreaId][address.m_nCarPathLinkId].m_vecPosn.x) / 8.0f,
			static_cast<float>(ThePaths.m_pNaviNodes[address.m_nAreaId][address.m_nCarPathLinkId].m_vecPosn.y) / 8.0f);
	}
	return CVector2D(0.0f, 0.0f);
}

void DrawGlobalLight(CVehicle* pVeh, eDummyPos pos, CRGBA col) {
	if (pVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE) {
		CAutomobile* ptr = reinterpret_cast<CAutomobile*>(pVeh);
		if ((pos == eDummyPos::FrontLeft && ptr->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_LEFT))
			|| (pos == eDummyPos::FrontRight && ptr->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_RIGHT))
			|| (pos == eDummyPos::RearLeft && ptr->m_damageManager.GetLightStatus(eLights::LIGHT_REAR_LEFT))
			|| (pos == eDummyPos::RearRight && ptr->m_damageManager.GetLightStatus(eLights::LIGHT_REAR_RIGHT))) {
			return;
		}
	}

	int idx = (pos == eDummyPos::RearLeft) || (pos == eDummyPos::RearRight);
	bool leftSide = (pos == eDummyPos::RearLeft) || (pos == eDummyPos::FrontLeft);

	CVector posn =
		reinterpret_cast<CVehicleModelInfo*>(CModelInfo__ms_modelInfoPtrs[pVeh->m_nModelIndex])->m_pVehicleStruct->m_avDummyPos[idx];

	if (posn.x == 0.0f) posn.x = 0.15f;
	if (leftSide) posn.x *= -1.0f;
	int dummyId = static_cast<int>(idx) + (leftSide ? 0 : 2);
	float dummyAngle = (pos == eDummyPos::RearLeft || pos == eDummyPos::RearRight) ? 180.0f : 0.0f;
	Common::RegisterShadow(pVeh, posn, col.r, col.g, col.b, GetShadowAlphaForDayTime(), dummyAngle, 0.0f, "indicator");
	Common::RegisterCoronaWithAngle(pVeh, (reinterpret_cast<unsigned int>(pVeh) * 255) + 255 + int(pos), posn, col.r, col.g, col.b, GetCoronaAlphaForDayTime(), dummyAngle, 0.3f, 0.9f);
}

inline float GetZAngleForPoint(CVector2D const& point) {
	float angle = CGeneral::GetATanOfXY(point.x, point.y) * 57.295776f - 90.0f;
	while (angle < 0.0f) angle += 360.0f;
	return angle;
}

inline bool IsBumperOrWingDamaged(CVehicle* pVeh, eParentType part) {
	if (pVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE) {
		CAutomobile* ptr = reinterpret_cast<CAutomobile*>(pVeh);
		return ptr->m_damageManager.GetPanelStatus((int)part);
	}
	return false;
}

// Shadows
unsigned int HEADLIGHT_SHADOW_ALPHA = 240;
float HEADLIGHT_SHADOW_WIDTH_BIKE = 2.75f;
float HEADLIGHT_SHADOW_WIDTH = 240;
float HEADLIGHT_SHADOW_WIDTH_SHORT = 8.0f;
float HEADLIGHT_SHADOW_WIDTH_LONG = 8.0f;

// Coronas
float HEADLIGHT_CORONA_SIZE_SHORT = 0.075f;
float HEADLIGHT_CORONA_SIZE_LONG = 0.115f;

float HEADLIGHT_CORONA_ALPHA_SHORT = 128;
float HEADLIGHT_CORONA_ALPHA_LONG = 255;


void Lights::Initialize() {
	static float headlightTexWidth = HEADLIGHT_SHADOW_WIDTH_SHORT;

	patch::SetPointer(0x6E16A3, &headlightTexWidth);
	patch::SetPointer(0x6E1537, &headlightTexWidth);
	patch::SetPointer(0x6E1548, &HEADLIGHT_SHADOW_WIDTH_BIKE);
	patch::SetPointer(0x70C6CB, &HEADLIGHT_SHADOW_ALPHA);
	patch::SetPointer(0x70C72D, &HEADLIGHT_SHADOW_ALPHA);
	patch::SetUInt(0x6E0CF8, 0xC0); // Decrease inner corona alpha a bit

	static RwTexture* hss = nullptr;
	static RwTexture* hsl = nullptr;
	static RwTexture* hts = nullptr;
	static RwTexture* htl = nullptr;

	plugin::Events::initGameEvent += []() {
		hss = Util::LoadTextureFromFile(MOD_DATA_PATH_S(std::string("textures/headlight_single_short.png")), 255);
		hsl = Util::LoadTextureFromFile(MOD_DATA_PATH_S(std::string("textures/headlight_single_long.png")), 255);
		hts = Util::LoadTextureFromFile(MOD_DATA_PATH_S(std::string("textures/headlight_twin_short.png")), 255);
		htl = Util::LoadTextureFromFile(MOD_DATA_PATH_S(std::string("textures/headlight_twin_long.png")), 255);
		};

	static FastcallEvent <AddressList<0x6E1A76, H_CALL>, PRIORITY_BEFORE,
		ArgPick2N<CVehicle*, 0, int, 1>, void(CVehicle*, int)> DoHeadLightEvent;

	DoHeadLightEvent += [](CVehicle* pVeh, int b) {
		VehData& data = m_VehData.Get(pVeh);
		if (data.m_bLongLightsOn) {
			plugin::patch::SetPointer(0x6E1693, htl); // Twin
			plugin::patch::SetPointer(0x6E151D, hsl); // Single
			headlightTexWidth = HEADLIGHT_SHADOW_WIDTH_LONG;
			patch::SetFloat(0x6E0CA6, HEADLIGHT_CORONA_SIZE_LONG); // HeadLightCoronaSize
			patch::SetUInt(0x6E0DEE, HEADLIGHT_CORONA_ALPHA_LONG); // HeadLightCoronaAlpha
		}
		else {
			plugin::patch::SetPointer(0x6E1693, hts); // Twin
			plugin::patch::SetPointer(0x6E151D, hss); // Single
			headlightTexWidth = HEADLIGHT_SHADOW_WIDTH_SHORT;
			patch::SetFloat(0x6E0CA6, HEADLIGHT_CORONA_SIZE_SHORT);
			patch::SetUInt(0x6E0DEE, HEADLIGHT_CORONA_ALPHA_SHORT); // HeadLightCoronaAlpha
		}
		};

	Events::vehicleDtorEvent += [](CVehicle* pVeh) {
		m_Dummies.erase(pVeh);
		};

	VehicleMaterials::Register([](CVehicle* vehicle, RpMaterial* material, CRGBA col) {
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
		else if (col.r == 255 && col.g == 174 && col.b == 0)
			RegisterMaterial(vehicle, material, eLightState::FogLightLeft, col);
		else if (col.r == 0 && col.g == 255 && col.b == 199)
			RegisterMaterial(vehicle, material, eLightState::FogLightRight, col);
		else if ((col.r == 255 && col.g == 175 && col.b == 0)
		|| (col.r == 255 && col.g == 1 && col.b == 128))
			RegisterMaterial(vehicle, material, eLightState::FrontLightLeft, col);
		else if ((col.r == 0 && col.g == 255 && col.b == 200)
			|| (col.r == 255 && col.g == 2 && col.b == 128))
			RegisterMaterial(vehicle, material, eLightState::FrontLightRight, col);
		else if (col.r == 255 && col.g == 60 && col.b == 0)
			RegisterMaterial(vehicle, material, eLightState::TailLightRight, col);
		else if (col.r == 185 && col.g == 255 && col.b == 0)
			RegisterMaterial(vehicle, material, eLightState::TailLightLeft, col);

		// Indicator Lights
		eDummyPos pos = eDummyPos::None;
		if (col.b == 0) {
			if (col.r == 255) { // Right
				if (col.g >= 56 && col.g <= 59) {
					if (col.g == 58) {
						pos = eDummyPos::FrontRight;
					}
					else if (col.g == 57) {
						pos = eDummyPos::MiddleRight;
					}
					else if (col.g == 56) {
						pos = eDummyPos::RearRight;
					}
					RegisterMaterial(vehicle, material, eLightState::IndicatorRight, col, pos);
				}
			}
			else if (col.g == 255) { // Left
				if (col.r >= 181 && col.r <= 184) {
					if (col.r == 183) {
						pos = eDummyPos::FrontLeft;
					}
					else if (col.r == 182) {
						pos = eDummyPos::MiddleLeft;
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

		return material;
		});

	VehicleMaterials::RegisterDummy([](CVehicle* pVeh, RwFrame* frame, std::string name, bool parent) {
		eLightState state = eLightState::None;
		RwRGBA col{ 255, 255, 255, 128 };

		std::smatch match;
		if (std::regex_search(name, match, std::regex("^fogl(ight)?_([lr]).*$"))) {
			state = (toupper(match.str(2)[0]) == 'L') ? (eLightState::FogLightLeft) : (eLightState::FogLightRight);
		}
		else if (std::regex_search(name, std::regex("^rev.*\s*_[lr].*$"))) {
			state = eLightState::Reverselight;
		}
		else if (std::regex_search(name, std::regex("^light_day"))) {
			state = eLightState::Daylight;
		}
		else if (std::regex_search(name, std::regex("^light_night"))) {
			state = eLightState::Nightlight;
		}
		else if (std::regex_search(name, std::regex("^light_em"))) {
			state = eLightState::AllDayLight;
			col = { 0, 0, 0, 0 }; // Make invisible
		}
		else if (std::regex_search(name, match, std::regex("^(turnl_|indicator_)(.{2})"))) { // Indicator Lights
			std::string stateStr = match.str(2);
			eLightState state = (toupper(stateStr[0]) == 'L') ? eLightState::IndicatorLeft : eLightState::IndicatorRight;
			eDummyPos rot = eDummyPos::None;

			if (toupper(stateStr[1]) == 'F') {
				rot = state == eLightState::IndicatorRight ? eDummyPos::FrontRight : eDummyPos::FrontLeft;
			}
			else if (toupper(stateStr[1]) == 'R') {
				rot = state == eLightState::IndicatorRight ? eDummyPos::RearRight : eDummyPos::RearLeft;
			}
			else if (toupper(stateStr[1]) == 'M') {
				rot = state == eLightState::IndicatorRight ? eDummyPos::MiddleRight : eDummyPos::MiddleLeft;
			}

			if (rot != eDummyPos::None) {
				bool exists = false;
				for (auto e : m_Dummies[pVeh][state]) {
					if (e->Position.y == frame->modelling.pos.y
					&& e->Position.z == frame->modelling.pos.z) {
						exists = true;
						break;
					}
				}

				if (!exists) {
					LOG_VERBOSE("Registering {} for {}", name, pVeh->m_nModelIndex);
					m_Dummies[pVeh][state].push_back(new VehicleDummy(pVeh, frame, name, parent, rot, { 255, 128, 0, 128 }));
					return;
				}
			}
		}

		if (state != eLightState::None) {
			m_Dummies[pVeh][state].push_back(new VehicleDummy(pVeh, frame, name, parent, eDummyPos::Rear, col));
		}
	});

	Events::processScriptsEvent += []() {
		size_t timestamp = CTimer::m_snTimeInMilliseconds;
		if ((timestamp - delay) > 500) {
			delay = timestamp;
			delayState = !delayState;
		}

		CVehicle* pVeh = FindPlayerVehicle(-1, false);
		if (pVeh) {
			static size_t prev = 0;
			if (KeyPressed(VK_J)
			&& (!m_Dummies[pVeh][eLightState::FogLightLeft].empty() || !m_Dummies[pVeh][eLightState::FogLightRight].empty())) {
				size_t now = CTimer::m_snTimeInMilliseconds;
				if (now - prev > 500.0f) {
					VehData& data = m_VehData.Get(pVeh);
					data.m_bFogLightsOn = !data.m_bFogLightsOn;
					prev = now;
					AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RADIO_CLICK_ON, 10.0, 1.0);
				}
			}

			if (KeyPressed(VK_G)) {
				size_t now = CTimer::m_snTimeInMilliseconds;
				if (now - prev > 500.0f) {
					VehData& data = m_VehData.Get(pVeh);
					data.m_bLongLightsOn = !data.m_bLongLightsOn;
					prev = now;
					AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RADIO_CLICK_ON, 10.0, 1.0);
				}
			}
		}
		};

	VehicleMaterials::RegisterRender([](CVehicle* pControlVeh) {
		int model = pControlVeh->m_nModelIndex;
		CVehicle* pTowedVeh = pControlVeh;
		if (pControlVeh->m_pTrailer) {
			pTowedVeh = pControlVeh->m_pTrailer;
		}

		if (pControlVeh->m_pTractor) {
			pTowedVeh = pControlVeh->m_pTractor;
		}
		VehData& data = m_VehData.Get(pControlVeh);

		if (pControlVeh->m_fHealth == 0) { // TODO
			return;
		}


		if (!m_Materials[pControlVeh->m_nModelIndex].empty()) {
			CAutomobile* automobile = reinterpret_cast<CAutomobile*>(pControlVeh);

			float vehicleAngle = (pControlVeh->GetHeading() * 180.0f) / 3.14f;
			float cameraAngle = (TheCamera.GetHeading() * 180.0f) / 3.14f;

			RenderLights(pControlVeh, eLightState::AllDayLight, vehicleAngle, cameraAngle);

			if (IsNightTime()) {
				RenderLights(pControlVeh, eLightState::Nightlight, vehicleAngle, cameraAngle);
			}
			else {
				RenderLights(pControlVeh, eLightState::Daylight, vehicleAngle, cameraAngle);
			}

			bool leftOk = true;
			bool rightOk = true;
			if (CModelInfo::IsCarModel(pControlVeh->m_nModelIndex)) {
				bool leftOn = automobile->m_renderLights.m_bLeftFront;
				bool rightOn = automobile->m_renderLights.m_bRightFront;
				leftOk = leftOn && !automobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_LEFT);
				rightOk = rightOn && !automobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_RIGHT);
			}

			if (data.m_bFogLightsOn) {
				CVector posn = reinterpret_cast<CVehicleModelInfo*>(CModelInfo__ms_modelInfoPtrs[pControlVeh->m_nModelIndex])->m_pVehicleStruct->m_avDummyPos[0];
				if (leftOk && rightOk) {
					posn.x = 0.0f;
					posn.y += 4.2f;
					Common::RegisterShadow(pControlVeh, posn, 225, 225, 225, GetShadowAlphaForDayTime(), 180.0f, 0.0f, "foglight_twin", 2.0f);
					RenderLights(pControlVeh, eLightState::FogLightLeft, vehicleAngle, cameraAngle, false, "foglight_single", 1.0f);
					RenderLights(pControlVeh, eLightState::FogLightRight, vehicleAngle, cameraAngle, false, "foglight_single", 1.0f);
				}
				else if (leftOk || rightOk) {
					posn.x = leftOk ? -0.5f : 0.5f;
					posn.y += 3.2f;
					RenderLights(pControlVeh, leftOk ? eLightState::FogLightLeft : eLightState::FogLightRight, vehicleAngle, cameraAngle, false, "foglight_single", 1.0f);
					Common::RegisterShadow(pControlVeh, posn, 225, 225, 225, GetShadowAlphaForDayTime(), 180.0f, 0.0f, "foglight_single", 1.2f);
				}
			}

			if (pControlVeh->m_nVehicleFlags.bLightsOn || !pControlVeh->ms_forceVehicleLightsOff) {
				VehData& data = m_VehData.Get(pControlVeh);
				if (leftOk && m_Materials[pControlVeh->m_nModelIndex][eLightState::FrontLightLeft].size() != 0) {
					RenderLights(pControlVeh, eLightState::FrontLightLeft, vehicleAngle, cameraAngle);
				}

				if (rightOk && m_Materials[pControlVeh->m_nModelIndex][eLightState::FrontLightRight].size() != 0) {
					RenderLights(pControlVeh, eLightState::FrontLightRight, vehicleAngle, cameraAngle);
				}
			}

			if (IsNightTime()) {
				if (m_Materials[pControlVeh->m_nModelIndex][eLightState::Nightlight].size() != 0) {
					RenderLights(pControlVeh, eLightState::Nightlight, vehicleAngle, cameraAngle);
				}

				if (m_Materials[pTowedVeh->m_nModelIndex][eLightState::Nightlight].size() != 0) {
					RenderLights(pTowedVeh, eLightState::Nightlight, vehicleAngle, cameraAngle);
				}
			}
			else {
				if (m_Materials[pControlVeh->m_nModelIndex][eLightState::Daylight].size() != 0) {
					RenderLights(pControlVeh, eLightState::Daylight, vehicleAngle, cameraAngle);
				}

				if (m_Materials[pTowedVeh->m_nModelIndex][eLightState::Daylight].size() != 0) {
					RenderLights(pTowedVeh, eLightState::Daylight, vehicleAngle, cameraAngle);
				}
			}

			if (m_Materials[pControlVeh->m_nModelIndex][eLightState::AllDayLight].size() != 0) {
				RenderLights(pControlVeh, eLightState::AllDayLight, vehicleAngle, cameraAngle);
			}

			if (m_Materials[pTowedVeh->m_nModelIndex][eLightState::AllDayLight].size() != 0) {
				RenderLights(pTowedVeh, eLightState::AllDayLight, vehicleAngle, cameraAngle);
			}

			bool isBike = CModelInfo::IsBikeModel(pControlVeh->m_nModelIndex);
			if (isBike || CModelInfo::IsCarModel(pControlVeh->m_nModelIndex)) {
				bool isRevlightSupportedByModel = !m_Dummies[pTowedVeh][eLightState::Reverselight].empty();

				bool globalRevlights = gConfig.ReadBoolean("VEHICLE_FEATURES", "StandardLights_GlobalReverseLights", false);
				bool reverseLightsOn = !isBike && (isRevlightSupportedByModel || globalRevlights)
					&& pControlVeh->m_nCurrentGear == 0 && (pControlVeh->m_fMovingSpeed >= 0.01f) && pControlVeh->m_pDriver;

				if (reverseLightsOn) {
					if (isRevlightSupportedByModel) {
						RenderLights(pTowedVeh, eLightState::Reverselight, vehicleAngle, cameraAngle);
					}
					else if (globalRevlights) {
						DrawGlobalLight(pTowedVeh, eDummyPos::RearLeft, { 240, 240, 240, 128 });
						DrawGlobalLight(pTowedVeh, eDummyPos::RearRight, { 240, 240, 240, 128 });
					}
				}

				// taillights/ brakelights
				if (pControlVeh->m_pDriver && (pControlVeh->m_nRenderLightsFlags || pControlVeh->m_fBreakPedal)) {
					CVector posn = reinterpret_cast<CVehicleModelInfo*>(CModelInfo__ms_modelInfoPtrs[pTowedVeh->m_nModelIndex])->m_pVehicleStruct->m_avDummyPos[1];
					posn.x = 0.0f;
					posn.y += 0.2f;
					int r = 250;
					int g = 0;
					int b = 0;
					Common::RegisterShadow(pTowedVeh, posn, r, g, b, GetShadowAlphaForDayTime(), 180.0f, 0.0f, isBike ? "taillight_bike" : "taillight", 1.75f);
					if (pControlVeh->m_nRenderLightsFlags && pControlVeh->m_fBreakPedal) {
						RenderLights(pTowedVeh, eLightState::Brakelight, vehicleAngle, cameraAngle, false);
						Common::RegisterShadow(pTowedVeh, posn, r, g, b, GetShadowAlphaForDayTime(), 180.0f, 0.0f, isBike ? "taillight_bike" : "taillight", 1.75f);
					}
					RenderLights(pTowedVeh, eLightState::TailLightLeft, vehicleAngle, cameraAngle);
					RenderLights(pTowedVeh, eLightState::TailLightRight, vehicleAngle, cameraAngle);
				}
			}
		}

		// Indicator Lights
		eLightState state = data.m_nIndicatorState;
		if (!gConfig.ReadBoolean("VEHICLE_FEATURES", "StandardLights_GlobalIndicatorLights", false) && m_Dummies[pControlVeh].size() == 0 && m_Materials[pControlVeh->m_nModelIndex][state].size() == 0) {
			return;
		}

		if (CCutsceneMgr::ms_running || TheCamera.m_bWideScreenOn) {
			return;
		}

		if (pControlVeh->m_pDriver == FindPlayerPed()) {
			if (KeyPressed(VK_SHIFT)) {
				data.m_nIndicatorState = eLightState::IndicatorNone;
				delay = 0;
				delayState = false;
			}

			if (KeyPressed(VK_Z)) {
				data.m_nIndicatorState = eLightState::IndicatorLeft;
			}

			if (KeyPressed(VK_C)) {
				data.m_nIndicatorState = eLightState::IndicatorRight;
			}

			if (KeyPressed(VK_X)) {
				data.m_nIndicatorState = eLightState::IndicatorBoth;
			}
		}
		else if (pControlVeh->m_pDriver) {
			data.m_nIndicatorState = eLightState::IndicatorNone;
			CVector2D prevPoint = GetCarPathLinkPosition(pControlVeh->m_autoPilot.m_nPreviousPathNodeInfo);
			CVector2D currPoint = GetCarPathLinkPosition(pControlVeh->m_autoPilot.m_nCurrentPathNodeInfo);
			CVector2D nextPoint = GetCarPathLinkPosition(pControlVeh->m_autoPilot.m_nNextPathNodeInfo);

			float angle = GetZAngleForPoint(nextPoint - currPoint) - GetZAngleForPoint(currPoint - prevPoint);
			while (angle < 0.0f) angle += 360.0f;
			while (angle > 360.0f) angle -= 360.0f;

			if (angle >= 30.0f && angle < 180.0f)
				data.m_nIndicatorState = eLightState::IndicatorLeft;
			else if (angle <= 330.0f && angle > 180.0f)
				data.m_nIndicatorState = eLightState::IndicatorRight;

			if (data.m_nIndicatorState == eLightState::IndicatorNone) {
				if (pControlVeh->m_autoPilot.m_nCurrentLane == 0 && pControlVeh->m_autoPilot.m_nNextLane == 1)
					data.m_nIndicatorState = eLightState::IndicatorRight;
				else if (pControlVeh->m_autoPilot.m_nCurrentLane == 1 && pControlVeh->m_autoPilot.m_nNextLane == 0)
					data.m_nIndicatorState = eLightState::IndicatorLeft;
			}
		}

		if (pControlVeh->m_pTrailer) {
			Lights::VehData& trailer = Lights::m_VehData.Get(pControlVeh->m_pTrailer);
			trailer.m_nIndicatorState = data.m_nIndicatorState;
			data.m_nIndicatorState = eLightState::IndicatorNone;
		}

		if (pControlVeh->m_pTractor) {
			Lights::VehData& trailer = Lights::m_VehData.Get(pControlVeh->m_pTractor);
			trailer.m_nIndicatorState = data.m_nIndicatorState;
			data.m_nIndicatorState = eLightState::IndicatorNone;
		}

		if (!delayState || state == eLightState::IndicatorNone) {
			return;
		}

		// global turn lights
		if (gConfig.ReadBoolean("VEHICLE_FEATURES", "StandardLights_GlobalIndicatorLights", false) &&
			(m_Dummies[pControlVeh][eLightState::IndicatorLeft].size() == 0 || m_Dummies[pControlVeh][eLightState::IndicatorRight].size() == 0)
			 && (m_Materials[pControlVeh->m_nModelIndex][eLightState::IndicatorLeft].size() == 0 || m_Materials[pControlVeh->m_nModelIndex][eLightState::IndicatorRight].size() == 0))
		{
			if ((pControlVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pControlVeh->m_nVehicleSubClass == VEHICLE_BIKE) &&
				(pControlVeh->GetVehicleAppearance() == VEHICLE_APPEARANCE_AUTOMOBILE || pControlVeh->GetVehicleAppearance() == VEHICLE_APPEARANCE_BIKE) &&
				pControlVeh->m_nVehicleFlags.bEngineOn && pControlVeh->m_fHealth > 0 && !pControlVeh->m_nVehicleFlags.bIsDrowning && !pControlVeh->m_pAttachedTo)
			{
				if (DistanceBetweenPoints(TheCamera.m_vecGameCamPos, pControlVeh->GetPosition()) < 150.0f) {
					if (state == eLightState::IndicatorBoth || state == eLightState::IndicatorRight) {
						DrawGlobalLight(pControlVeh, eDummyPos::FrontRight, { 255, 128, 0, 0 });
						DrawGlobalLight(pTowedVeh, eDummyPos::RearRight, { 255, 128, 0, 0 });
					}
					if (state == eLightState::IndicatorBoth || state == eLightState::IndicatorLeft) {
						DrawGlobalLight(pControlVeh, eDummyPos::FrontLeft, { 255, 128, 0, 0 });
						DrawGlobalLight(pTowedVeh, eDummyPos::RearLeft, { 255, 128, 0, 0 });
					}
				}
			}
		}
		else {
			int id = 0;
			if (state == eLightState::IndicatorBoth || state == eLightState::IndicatorLeft) {
				bool FrontDisabled = false;
				bool RearDisabled = false;
				bool MidDisabled = false;

				for (auto e : m_Dummies[pControlVeh][eLightState::IndicatorLeft]) {
					if (e->PartType != eParentType::Unknown && IsBumperOrWingDamaged(pControlVeh, e->PartType)) {
						if (e->Type == eDummyPos::FrontLeft) {
							FrontDisabled = true;
						}
						if (e->Type == eDummyPos::MiddleLeft) {
							MidDisabled = true;
						}
						if (e->Type == eDummyPos::RearLeft) {
							RearDisabled = true;
						}
						continue;
					}

					bool isRear = (e->Type == eDummyPos::RearLeft || e->Type == eDummyPos::RearRight);
					EnableDummy((int)pControlVeh + 42 + id++, e, isRear ? pTowedVeh : pControlVeh);
					Common::RegisterShadow(isRear ? pTowedVeh : pControlVeh, e->ShdwPosition, e->Color.r, e->Color.g, e->Color.b, GetShadowAlphaForDayTime(), e->Angle, e->CurrentAngle, "indicator");
				}

				for (auto e : m_Materials[pControlVeh->m_nModelIndex][eLightState::IndicatorLeft]) {
					if (e->Pos == eDummyPos::RearLeft) continue; // Skip rear
					if ((FrontDisabled && e->Pos == eDummyPos::FrontLeft)
					|| RearDisabled && e->Pos == eDummyPos::RearLeft
					|| MidDisabled && e->Pos == eDummyPos::MiddleLeft) {
						continue;
					}
					EnableMaterial(e);
				}

				if (!RearDisabled) {
					for (auto e : m_Materials[pTowedVeh->m_nModelIndex][eLightState::IndicatorLeft]) {
						if (e->Pos == eDummyPos::RearLeft && !RearDisabled) {
							EnableMaterial(e);
						}
					}
				}
			}

			if (state == eLightState::IndicatorBoth || state == eLightState::IndicatorRight) {
				bool FrontDisabled = false;
				bool RearDisabled = false;
				bool MidDisabled = false;

				for (auto e : m_Dummies[pControlVeh][eLightState::IndicatorRight]) {
					if (e->PartType != eParentType::Unknown && IsBumperOrWingDamaged(pControlVeh, e->PartType)) {
						if (e->Type == eDummyPos::FrontRight) {
							FrontDisabled = true;
						}
						if (e->Type == eDummyPos::MiddleRight) {
							MidDisabled = true;
						}
						if (e->Type == eDummyPos::RearRight) {
							RearDisabled = true;
						}
						continue;
					}
					bool isRear = (e->Type == eDummyPos::RearLeft || e->Type == eDummyPos::RearRight);
					EnableDummy((int)pControlVeh + 44 + id++, e, isRear ? pTowedVeh : pControlVeh);
					Common::RegisterShadow(isRear ? pTowedVeh : pControlVeh, e->ShdwPosition, e->Color.r, e->Color.g, e->Color.b, GetShadowAlphaForDayTime(), e->Angle, e->CurrentAngle, "indicator");
				}

				for (auto& e : m_Materials[pControlVeh->m_nModelIndex][eLightState::IndicatorRight]) {
					if (e->Pos == eDummyPos::RearRight) continue; // Skip rear
					if ((FrontDisabled && e->Pos == eDummyPos::FrontRight)
					|| MidDisabled && e->Pos == eDummyPos::MiddleRight) {
						continue;
					}
					EnableMaterial(e);
				}

				if (!RearDisabled) {
					for (auto e : m_Materials[pTowedVeh->m_nModelIndex][eLightState::IndicatorRight]) {
						if (e->Pos == eDummyPos::RearRight) {
							EnableMaterial(e);
						}
					}
				}
			}
		}
});
};

void Lights::RenderLights(CVehicle* pVeh, eLightState state, float vehicleAngle, float cameraAngle, bool shadows, std::string texture, float sz) {
	bool flag = true;
	int id = 0;
	for (auto e : m_Dummies[pVeh][state]) {
		if (CModelInfo::IsCarModel(pVeh->m_nModelIndex)) {
			if (e->PartType != eParentType::Unknown && IsBumperOrWingDamaged(pVeh, e->PartType)) {
				flag = false;
				if (state == eLightState::FogLightLeft || state == eLightState::FogLightRight) {
					m_VehData.Get(pVeh).m_bFogLightsOn = false;
				}
				continue;
			}
		}
		EnableDummy((int)pVeh + (int)state + 30 + id++, e, pVeh);

		if (shadows) {
			Common::RegisterShadow(pVeh, e->ShdwPosition, e->Color.r, e->Color.g, e->Color.b, GetShadowAlphaForDayTime(), e->Angle, e->CurrentAngle, texture, sz);
		}
	}

	if (flag) {
		for (auto& e : m_Materials[pVeh->m_nModelIndex][state]) {
			EnableMaterial(e);
		}
	}
};

void Lights::RegisterMaterial(CVehicle* pVeh, RpMaterial* material, eLightState state, CRGBA col, eDummyPos pos) {
	VehicleMaterial* mat = new VehicleMaterial(material, pos);
	m_Materials[pVeh->m_nModelIndex][state].push_back(mat);
	material->color.red = material->color.green = material->color.blue = 255;
};

void Lights::EnableDummy(int id, VehicleDummy* dummy, CVehicle* pVeh) {
	if (gConfig.ReadBoolean("VEHICLE_FEATURES", "LightCoronas", false)) {
		Common::RegisterCoronaWithAngle(pVeh, (reinterpret_cast<unsigned int>(pVeh) * 255) + 255 + id, *(CVector*)&dummy->Position, dummy->Color.r, dummy->Color.g, dummy->Color.b,
		dummy->Color.a, dummy->Angle, 0.3f, dummy->Size);
	}
};

void Lights::EnableMaterial(VehicleMaterial* material) {
	if (material && material->Material) {
		VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int*>(&material->Material->surfaceProps.ambient), *reinterpret_cast<unsigned int*>(&material->Material->surfaceProps.ambient)));
		material->Material->surfaceProps.ambient = AMBIENT_ON_VAL;
		VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int*>(&material->Material->texture), *reinterpret_cast<unsigned int*>(&material->Material->texture)));
		material->Material->texture = material->TextureActive;
	}
};