#include "pch.h"
#include "indicators.h"
#include "avs/common.h"
#include "defines.h"

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

void Indicator::Initialize() {
	VehicleMaterials::Register([](CVehicle* vehicle, RpMaterial* material) {
		if (material->color.blue == 0) {
			if (material->color.red == 255) {
				if (material->color.green > 55 && material->color.green < 59) {
					Lights::RegisterMaterial(vehicle, material, eLightState::IndicatorRight);
				}
			}
			else if (material->color.green == 255) {
				if (material->color.red > 180 && material->color.red < 184) {
					Lights::RegisterMaterial(vehicle, material, eLightState::IndicatorLeft);
				}
			}
		}

		if (material->color.red == 255 
		&& (material->color.green == 4 ||  material->color.green == 5) 
		&& material->color.blue == 128 
		&& std::string(material->texture->name).rfind("light", 0) == 0) {
			Lights::RegisterMaterial(vehicle, material, (material->color.green == 4) ? eLightState::IndicatorLeft : eLightState::IndicatorRight);
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
				for (auto e: dummies[pVeh->m_nModelIndex][state]) {
					if (e->Position.y == pFrame->modelling.pos.y
					&& e->Position.z == pFrame->modelling.pos.z) {
						exists = true;
						break;
					}
				}

				if (!exists) {
					dummies[pVeh->m_nModelIndex][state].push_back(new VehicleDummy(pFrame, name, parent, rot, { 255, 128, 0, 128 }));
				}
			}
		}
	});

	VehicleMaterials::RegisterRender([](CVehicle* pVeh) {
		if (pVeh->m_fHealth == 0) {
			return;
		}

		VehData &data = vehData.Get(pVeh);
		int model = pVeh->m_nModelIndex;
		eLightState state = data.indicatorState;

		if (gConfig.ReadBoolean("FEATURES", "GlobalIndicators", false) == false && 
		dummies[model].size() == 0 && materials[model][state].size() == 0) {
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
			dummies[model].size() == 0 && materials[model][state].size() == 0)
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
				for (auto e: materials[model][eLightState::IndicatorLeft]){
					Lights::EnableMaterial(e);
				}

				for (auto e: dummies[model][eLightState::IndicatorLeft]) {
					RwRGBA color = e->Color;
					// TODO: Use RwTexture color instead
					// if (materials[model][eLightState::IndicatorLeft].size() > 0) {
					// 	color = GetColorFromTexture(materials[model][eLightState::IndicatorLeft][0]->TextureActive, 50, 50);
					// }
					Lights::EnableDummy((int)pVeh + id++, e, pVeh);
					Common::RegisterShadow(pVeh, e->ShdwPosition, color.red, color.green, color.blue, color.alpha, e->Angle, e->CurrentAngle, "indicator");
				}
			}

			if (state == eLightState::IndicatorBoth || state == eLightState::IndicatorRight) {
				for (auto &e: materials[model][eLightState::IndicatorRight]){
					Lights::EnableMaterial(e);
				}

				for (auto e: dummies[model][eLightState::IndicatorRight]) {
					RwRGBA color = e->Color;
					// TODO: Use RwTexture color instead
					// if (materials[model][eLightState::IndicatorRight].size() > 0) {
					// 	color = GetColorFromTexture(materials[model][eLightState::IndicatorLeft][0]->TextureActive, 50, 50);
					// }
					Lights::EnableDummy((int)pVeh + id++, e, pVeh);
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