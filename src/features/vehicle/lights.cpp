#include "pch.h"
#include "lights.h"
#include <CClock.h>
#include "avs/common.h"
#include "defines.h"
#include <CShadows.h>

void Lights::Initialize() {

	// NOP CVehicle::DoHeadLightBeam
	patch::Nop(0x6A2E9F, 0x58);
	patch::Nop(0x6BDE73, 0x12);

	VehicleMaterials::Register([](CVehicle* vehicle, RpMaterial* material) {
		if (material->color.red == 255 && material->color.green == 173 && material->color.blue == 0)
			RegisterMaterial(vehicle, material, eLightState::Reverselight);

		else if (material->color.red == 0 && material->color.green == 255 && material->color.blue == 198)
			RegisterMaterial(vehicle, material, eLightState::Reverselight);

		else if (material->color.red == 184 && material->color.green == 255 && material->color.blue == 0)
			RegisterMaterial(vehicle, material, eLightState::Brakelight);

		else if (material->color.red == 255 && material->color.green == 59 && material->color.blue == 0)
			RegisterMaterial(vehicle, material, eLightState::Brakelight);

		else if (material->color.red == 0 && material->color.green == 16 && material->color.blue == 255)
			RegisterMaterial(vehicle, material, eLightState::Nightlight);

		else if (material->color.red == 0 && material->color.green == 17 && material->color.blue == 255)
			RegisterMaterial(vehicle, material, eLightState::AllDayLight);
		else if (material->color.red == 0 && material->color.green == 18 && material->color.blue == 255)
			RegisterMaterial(vehicle, material, eLightState::Daylight);

		else if (material->color.red == 255 && material->color.green == 174 && material->color.blue == 0)
			RegisterMaterial(vehicle, material, eLightState::FogLight);
		else if (material->color.red == 0 && material->color.green == 255 && material->color.blue == 199)
			RegisterMaterial(vehicle, material, eLightState::FogLight);
			
		else if (material->color.red == 255 && material->color.green == 175 && material->color.blue == 0)
			RegisterMaterial(vehicle, material, eLightState::FrontLightLeft);
		else if (material->color.red == 0 && material->color.green == 255 && material->color.blue == 200) 
			RegisterMaterial(vehicle, material, eLightState::FrontLightRight);
		else if (material->color.red == 255 && material->color.green == 60 && material->color.blue == 0) 
			RegisterMaterial(vehicle, material, eLightState::TailLightRight);
		else if (material->color.red == 185 && material->color.green == 255 && material->color.blue == 0)
			RegisterMaterial(vehicle, material, eLightState::TailLightLeft);

		return material;
	});

	VehicleMaterials::RegisterDummy([](CVehicle* vehicle, RwFrame* frame, std::string name, bool parent) {
		eLightState state = eLightState::None;
		eDummyPos rotation = eDummyPos::Rear;
		RwRGBA col{ 255, 255, 255, 128 };

		std::smatch match;
		if (std::regex_search(name, match, std::regex("^fog(light)?_([a-zA-Z])"))) {
			state = (toupper(match.str(2)[0]) == 'L') ? (eLightState::FogLight) : (eLightState::FogLight);
		} else if (std::regex_search(name, std::regex("^(revl_|reversingl_|revr_|reversingr_|reversinglight_l|reversinglight_r)"))) {
			state = eLightState::Reverselight;
			col = {255, 255, 255, 240};
		} else if (std::regex_search(name, std::regex("^light_day"))) {
			state = eLightState::Daylight;
		} else if (std::regex_search(name, std::regex("^light_night"))) {
			state = eLightState::Nightlight;
		} else if (std::regex_search(name, std::regex("^light_(?!em)"))) {
			state = eLightState::AllDayLight;
		} else {
			return;
		}

		dummies[vehicle->m_nModelIndex][state].push_back(new VehicleDummy(frame, name, parent, rotation, col));
	});
	
	Events::processScriptsEvent += []() {
		CVehicle *pVeh = FindPlayerVehicle(-1, false);
		if (!pVeh) {
			return;
		}

		static size_t prev = 0;
		if (KeyPressed(VK_J)) {
			size_t now = CTimer::m_snTimeInMilliseconds;
			if (now - prev > 500.0f) {
				int model = pVeh->m_nModelIndex;

				VehData& data = vehData.Get(pVeh);
				data.m_bFogLightsOn = !data.m_bFogLightsOn;
				prev = now;
			}
		}
	};

	VehicleMaterials::RegisterRender([](CVehicle* pVeh) {
		int model = pVeh->m_nModelIndex;

		if (pVeh->m_fHealth == 0 || materials[model].size() == 0)
			return;

		CAutomobile* automobile = reinterpret_cast<CAutomobile*>(pVeh);

		float vehicleAngle = (pVeh->GetHeading() * 180.0f) / 3.14f;
		float cameraAngle = (TheCamera.GetHeading() * 180.0f) / 3.14f;

		RenderLights(pVeh, eLightState::AllDayLight, vehicleAngle, cameraAngle);

		if (CClock::GetIsTimeInRange(20, 8)) {
			RenderLights(pVeh, eLightState::Nightlight, vehicleAngle, cameraAngle);
		}
		else {
			RenderLights(pVeh, eLightState::Daylight, vehicleAngle, cameraAngle);
		}

		if (pVeh->m_nCurrentGear == 0 && pVeh->m_fMovingSpeed != 0 && pVeh->m_pDriver) {
			RenderLights(pVeh, eLightState::Reverselight, vehicleAngle, cameraAngle);
		}
		
		VehData& data = vehData.Get(pVeh);
		if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_LEFT) && 
		!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_RIGHT)) {
			if (data.m_bFogLightsOn) {
				RenderLights(pVeh, eLightState::FogLight, vehicleAngle, cameraAngle);
				static RwTexture *pLightTex = Util::LoadTextureFromFile(MOD_DATA_PATH_S(std::string("textures/foglight.png")), 120);
				CVector posn = reinterpret_cast<CVehicleModelInfo *>(CModelInfo__ms_modelInfoPtrs[pVeh->m_nModelIndex])->m_pVehicleStruct->m_avDummyPos[0];
				posn.x = 0.0f;
				posn.y += 2.0f;
				Common::RegisterShadow(pVeh, posn, 225, 225, 225, 128, 0.0f, 0.0f, "", 2.5f, 0.0f, pLightTex);
			}
		}
		
		if (pVeh->m_nVehicleFlags.bLightsOn) {
			if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_LEFT) 
			&& materials[model][eLightState::FrontLightLeft].size() != 0) {
				RenderLights(pVeh, eLightState::FrontLightLeft, vehicleAngle, cameraAngle);
			}

			if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_RIGHT)
			&& materials[model][eLightState::FrontLightRight].size() != 0) {
				RenderLights(pVeh, eLightState::FrontLightRight, vehicleAngle, cameraAngle);
			}

			bool showTailLights = false;
			if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_REAR_LEFT)) {
				RenderLights(pVeh, eLightState::TailLightLeft, vehicleAngle, cameraAngle);
				showTailLights = true;
			}

			if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_REAR_RIGHT)) {
				RenderLights(pVeh, eLightState::TailLightRight, vehicleAngle, cameraAngle);
				showTailLights = true;
			}

			if (pVeh->m_fBreakPedal && pVeh->m_pDriver) {
				RenderLights(pVeh, eLightState::Brakelight, vehicleAngle, cameraAngle);
				static RwTexture *pLightTex = Util::LoadTextureFromFile(MOD_DATA_PATH_S(std::string("textures/taillight.png")), 80);
				CVector posn = reinterpret_cast<CVehicleModelInfo *>(CModelInfo__ms_modelInfoPtrs[pVeh->m_nModelIndex])->m_pVehicleStruct->m_avDummyPos[1];
				posn.x = 0.0f;
				Common::RegisterShadow(pVeh, posn, TL_SHADOW_R, TL_SHADOW_G, TL_SHADOW_B, 128, 180.0f, 0.0f, "", 2.0f, 0.0f, pLightTex);
			}

			if (showTailLights) {
				static RwTexture *pLightTex = Util::LoadTextureFromFile(MOD_DATA_PATH_S(std::string("textures/taillight.png")), 40);
				CVector posn = reinterpret_cast<CVehicleModelInfo *>(CModelInfo__ms_modelInfoPtrs[pVeh->m_nModelIndex])->m_pVehicleStruct->m_avDummyPos[1];
				posn.x = 0.0f;
				Common::RegisterShadow(pVeh, posn, TL_SHADOW_R, TL_SHADOW_G, TL_SHADOW_B, 128, 180.0f, 0.0f, "", 2.0f, 0.0f, pLightTex);
			}
		}
	});

	InitIndicators();
};

void Lights::RenderLights(CVehicle* pVeh, eLightState state, float vehicleAngle, float cameraAngle) {
	for (auto &e: materials[pVeh->m_nModelIndex][state]) {
		EnableMaterial(e);
	}

	int id = 0;
	for (auto e: dummies[pVeh->m_nModelIndex][state]) {
		EnableDummy((int)pVeh + (int)state + id++, e, pVeh);
		Common::RegisterShadow(pVeh, e->ShdwPosition, e->Color.red, e->Color.green, e->Color.blue, 128,  e->Angle, e->CurrentAngle, "indicator");
	}
};

void Lights::RegisterMaterial(CVehicle* vehicle, RpMaterial* material, eLightState state) {
	material->color.red = material->color.green = material->color.blue = 255;
	materials[vehicle->m_nModelIndex][state].push_back(new VehicleMaterial(material));
};

void Lights::EnableDummy(int id, VehicleDummy* dummy, CVehicle* vehicle) {
	if (gConfig.ReadBoolean("FEATURES", "RenderCoronas", false)) {
		float cameraAngle = (TheCamera.GetHeading() * 180.0f) / 3.14f;
		Common::RegisterCoronaWithAngle(vehicle, dummy->Position, dummy->Color.red, dummy->Color.green, dummy->Color.blue, 
			CORONA_A, id, cameraAngle, dummy->CurrentAngle, 1.0f,  0.5f);
	}
};

void Lights::EnableMaterial(VehicleMaterial* material) {
	VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int*>(&material->Material->surfaceProps.ambient), *reinterpret_cast<unsigned int*>(&material->Material->surfaceProps.ambient)));
	material->Material->surfaceProps.ambient = 4.0;
	VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int*>(&material->Material->texture), *reinterpret_cast<unsigned int*>(&material->Material->texture)));
	material->Material->texture = material->TextureActive;
};

static uint64_t delay;
static bool delayState;

CVector2D GetCarPathLinkPosition(CCarPathLinkAddress &address) {
    if (address.m_nAreaId != -1 && address.m_nCarPathLinkId != -1 && ThePaths.m_pPathNodes[address.m_nAreaId]) {
        return CVector2D(static_cast<float>(ThePaths.m_pNaviNodes[address.m_nAreaId][address.m_nCarPathLinkId].m_vecPosn.x) / 8.0f,
            static_cast<float>(ThePaths.m_pNaviNodes[address.m_nAreaId][address.m_nCarPathLinkId].m_vecPosn.y) / 8.0f);
    }
    return CVector2D(0.0f, 0.0f);
}

void DrawTurnlight(CVehicle *pVeh, eDummyPos pos) {
	int idx = (pos == eDummyPos::RearLeft) || (pos == eDummyPos::RearRight);
	bool leftSide = (pos == eDummyPos::RearLeft) || (pos == eDummyPos::FrontLeft);

    CVector posn =
        reinterpret_cast<CVehicleModelInfo*>(CModelInfo__ms_modelInfoPtrs[pVeh->m_nModelIndex])->m_pVehicleStruct->m_avDummyPos[idx];
	
    if (posn.x == 0.0f) posn.x = 0.15f;
    if (leftSide) posn.x *= -1.0f;
	int dummyId = static_cast<int>(idx) + (leftSide ? 0 : 2);
	float dummyAngle = (pos == eDummyPos::RearLeft || pos == eDummyPos::RearRight) ? 180.0f : 0.0f;
	float cameraAngle = (TheCamera.GetHeading() * 180.0f) / 3.14f;
	Common::RegisterShadow(pVeh, posn, SHADOW_R, SHADOW_G, SHADOW_B, 128, dummyAngle, 0.0f, "indicator");
    Common::RegisterCoronaWithAngle(pVeh, posn, 255, 128, 0, CORONA_A, dummyId, cameraAngle, dummyAngle, 2.0f, 0.5f);
}

void DrawVehicleTurnlights(CVehicle *vehicle, eLightState lightsStatus) {
    if (lightsStatus == eLightState::IndicatorBoth || lightsStatus == eLightState::IndicatorRight) {
        DrawTurnlight(vehicle, eDummyPos::FrontRight);
        DrawTurnlight(vehicle, eDummyPos::RearRight);
    }
    if (lightsStatus == eLightState::IndicatorBoth || lightsStatus == eLightState::IndicatorLeft) {
        DrawTurnlight(vehicle, eDummyPos::FrontLeft);
        DrawTurnlight(vehicle, eDummyPos::RearLeft);
    }
}

float GetZAngleForPoint(CVector2D const &point) {
    float angle = CGeneral::GetATanOfXY(point.x, point.y) * 57.295776f - 90.0f;
    while (angle < 0.0f) angle += 360.0f;
    return angle;
}

void Lights::InitIndicators() {
	VehicleMaterials::Register([](CVehicle* vehicle, RpMaterial* material) {
		if (material->color.blue == 0) {
			if (material->color.red == 255) {
				if (material->color.green > 55 && material->color.green < 59) {
					RegisterMaterial(vehicle, material, eLightState::IndicatorRight);
				}
			}
			else if (material->color.green == 255) {
				if (material->color.red > 180 && material->color.red < 184) {
					RegisterMaterial(vehicle, material, eLightState::IndicatorLeft);
				}
			}
		}

		if (material->color.red == 255 
		&& (material->color.green == 4 ||  material->color.green == 5) 
		&& material->color.blue == 128 
		&& std::string(material->texture->name).rfind("light", 0) == 0) {
			RegisterMaterial(vehicle, material, (material->color.green == 4) ? eLightState::IndicatorLeft : eLightState::IndicatorRight);
		}
		return material;
	});

	VehicleMaterials::RegisterDummy([](CVehicle* pVeh, RwFrame* pFrame, std::string name, bool parent) {
		std::smatch match;
		if (std::regex_search(name, match, std::regex("^(turnl_|indicator_)(.{2})"))) {
			std::string stateStr = match.str(2);
			eLightState state = (toupper(stateStr[0]) == 'L') ? eLightState::IndicatorLeft : eLightState::IndicatorRight;
			eDummyPos rot = eDummyPos::None;
			
			if (toupper(stateStr[1]) == 'F') {
				rot = state == eLightState::IndicatorRight ? eDummyPos::FrontRight : eDummyPos::FrontLeft;
			} else if (toupper(stateStr[1]) == 'R') {
				rot = state == eLightState::IndicatorRight ? eDummyPos::RearRight : eDummyPos::RearLeft;
			} else if (toupper(stateStr[1]) == 'M') {
				rot = state == eLightState::IndicatorRight ? eDummyPos::MiddleRight : eDummyPos::MiddleLeft;
			}

			if (rot != eDummyPos::None) {
				bool exists = false;
				for (auto e: Lights::dummies[pVeh->m_nModelIndex][state]) {
					if (e->Position.y == pFrame->modelling.pos.y
					&& e->Position.z == pFrame->modelling.pos.z) {
						exists = true;
						break;
					}
				}

				if (!exists) {
					Lights::dummies[pVeh->m_nModelIndex][state].push_back(new VehicleDummy(pFrame, name, parent, rot, { 255, 128, 0, 128 }));
				}
			}
		}
	});

	VehicleMaterials::RegisterRender([](CVehicle* pVeh) {
		if (pVeh->m_fHealth == 0) {
			return;
		}

		Lights::VehData &data = Lights::vehData.Get(pVeh);
		int model = pVeh->m_nModelIndex;
		eLightState state = data.indicatorState;

		if (gConfig.ReadBoolean("FEATURES", "GlobalIndicators", false) == false && 
		Lights::dummies[model].size() == 0 && Lights::materials[model][state].size() == 0) {
			return;
		}
		
		if (pVeh->m_pDriver == FindPlayerPed()) {
			if (KeyPressed(VK_SHIFT)) {
				data.indicatorState = eLightState::IndicatorNone;
				delay = 0;
				delayState = false;
			}

			if (KeyPressed(VK_Z)) {
				data.indicatorState = eLightState::IndicatorLeft;
			}

			if (KeyPressed(VK_C)) { 
				data.indicatorState = eLightState::IndicatorRight;
			}

			if (KeyPressed(VK_X)) {
				data.indicatorState = eLightState::IndicatorBoth;
			}
		} else if (pVeh->m_pDriver) {
			data.indicatorState = eLightState::IndicatorNone;
			CVector2D prevPoint = GetCarPathLinkPosition(pVeh->m_autoPilot.m_nPreviousPathNodeInfo);
			CVector2D currPoint = GetCarPathLinkPosition(pVeh->m_autoPilot.m_nCurrentPathNodeInfo);
			CVector2D nextPoint = GetCarPathLinkPosition(pVeh->m_autoPilot.m_nNextPathNodeInfo);

			float angle = GetZAngleForPoint(nextPoint - currPoint) - GetZAngleForPoint(currPoint - prevPoint);
			while (angle < 0.0f) angle += 360.0f;
			while (angle > 360.0f) angle -= 360.0f;

			if (angle >= 30.0f && angle < 180.0f)
				data.indicatorState = eLightState::IndicatorLeft;
			else if (angle <= 330.0f && angle > 180.0f)
				data.indicatorState = eLightState::IndicatorRight;

			if (data.indicatorState == eLightState::IndicatorNone) {
				if (pVeh->m_autoPilot.m_nCurrentLane == 0 && pVeh->m_autoPilot.m_nNextLane == 1)
					data.indicatorState = eLightState::IndicatorRight;
				else if (pVeh->m_autoPilot.m_nCurrentLane == 1 && pVeh->m_autoPilot.m_nNextLane == 0)
					data.indicatorState = eLightState::IndicatorLeft;
			}
		}

		if (state == eLightState::IndicatorNone) {
			return;
		}

		if (!delayState)
			return;

		// global turn lights
		if (gConfig.ReadBoolean("FEATURES", "GlobalIndicators", false) &&
			Lights::dummies[model].size() == 0 && Lights::materials[model][state].size() == 0)
		{
			if ((pVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pVeh->m_nVehicleSubClass == VEHICLE_BIKE) &&
				(pVeh->GetVehicleAppearance() == VEHICLE_APPEARANCE_AUTOMOBILE || pVeh->GetVehicleAppearance() == VEHICLE_APPEARANCE_BIKE) &&
				pVeh->m_nVehicleFlags.bEngineOn && pVeh->m_fHealth > 0 && !pVeh->m_nVehicleFlags.bIsDrowning && !pVeh->m_pAttachedTo )
			{
				if (DistanceBetweenPoints(TheCamera.m_vecGameCamPos, pVeh->GetPosition()) < 150.0f) {
					DrawVehicleTurnlights(pVeh, state);
					if (pVeh->m_pTractor)
						DrawVehicleTurnlights(pVeh->m_pTractor, state);
				}
			}
		} else {
			int id = 0;
			if (state == eLightState::IndicatorBoth || state == eLightState::IndicatorLeft) {
				for (auto e: Lights::materials[model][eLightState::IndicatorLeft]){
					EnableMaterial(e);
				}

				for (auto e: Lights::dummies[model][eLightState::IndicatorLeft]) {
					RwRGBA color = e->Color;
					// TODO: Use RwTexture color instead
					// if (Lights::materials[model][eLightState::IndicatorLeft].size() > 0) {
					// 	color = GetColorFromTexture(Lights::materials[model][eLightState::IndicatorLeft][0]->TextureActive, 50, 50);
					// }
					EnableDummy((int)pVeh + id++, e, pVeh);
					Common::RegisterShadow(pVeh, e->ShdwPosition, color.red, color.green, color.blue, color.alpha, e->Angle, e->CurrentAngle, "indicator");
				}
			}

			if (state == eLightState::IndicatorBoth || state == eLightState::IndicatorRight) {
				for (auto &e: Lights::materials[model][eLightState::IndicatorRight]){
					EnableMaterial(e);
				}

				for (auto e: Lights::dummies[model][eLightState::IndicatorRight]) {
					RwRGBA color = e->Color;
					// TODO: Use RwTexture color instead
					// if (Lights::materials[model][eLightState::IndicatorRight].size() > 0) {
					// 	color = GetColorFromTexture(Lights::materials[model][eLightState::IndicatorLeft][0]->TextureActive, 50, 50);
					// }
					EnableDummy((int)pVeh + id++, e, pVeh);
					Common::RegisterShadow(pVeh, e->ShdwPosition, color.red, color.green, color.blue, color.alpha, e->Angle, e->CurrentAngle, "indicator");
				}
			}
		}
	});

	Events::drawingEvent += []() {
		size_t timestamp = CTimer::m_snTimeInMilliseconds;
		
		if ((timestamp - delay) < 500)
			return;

		delay = timestamp;
		delayState = !delayState;
	};
};