#include "pch.h"
#include "indicators.h"
#include "internals/common.h"
#include "defines.h"

CVector2D GetCarPathLinkPosition(CCarPathLinkAddress &address) {
    if (address.m_nAreaId != -1 && address.m_nCarPathLinkId != -1 && ThePaths.m_pPathNodes[address.m_nAreaId]) {
        return CVector2D(static_cast<float>(ThePaths.m_pNaviNodes[address.m_nAreaId][address.m_nCarPathLinkId].m_vecPosn.x) / 8.0f,
            static_cast<float>(ThePaths.m_pNaviNodes[address.m_nAreaId][address.m_nCarPathLinkId].m_vecPosn.y) / 8.0f);
    }
    return CVector2D(0.0f, 0.0f);
}

void DrawTurnlight(CVehicle *pVeh, eDummyPos pos, bool leftSide) {
	int idx = pos == eDummyPos::Front ? 0 : 1;
    CVector posn =
        reinterpret_cast<CVehicleModelInfo*>(CModelInfo::ms_modelInfoPtrs[pVeh->m_nModelIndex])->m_pVehicleStruct->m_avDummyPos[idx];
	
    if (posn.x == 0.0f) posn.x = 0.15f;
    if (leftSide) posn.x *= -1.0f;
	int dummyId = static_cast<int>(idx) + (leftSide ? 0 : 2);
	float dummyAngle = (pos == eDummyPos::Rear) ? 180.0f : 0.0f;
	float cameraAngle = (TheCamera.GetHeading() * 180.0f) / 3.14f;
	Common::RegisterShadow(pVeh, posn, SHADOW_R, SHADOW_G, SHADOW_B, dummyAngle, 0.0f);
    Common::RegisterCoronaWithAngle(pVeh, posn, 255, 128, 0, CORONA_A, dummyId, cameraAngle, dummyAngle, 2.0f, 0.5f);
}

void DrawVehicleTurnlights(CVehicle *vehicle, eIndicatorState lightsStatus) {
    if (lightsStatus == eIndicatorState::Both || lightsStatus == eIndicatorState::Right) {
        DrawTurnlight(vehicle, eDummyPos::Front, false);
        DrawTurnlight(vehicle, eDummyPos::Rear, false);
    }
    if (lightsStatus == eIndicatorState::Both || lightsStatus == eIndicatorState::Left) {
        DrawTurnlight(vehicle, eDummyPos::Front, true);
        DrawTurnlight(vehicle, eDummyPos::Rear, true);
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
					registerMaterial(vehicle, material, eIndicatorState::Right);
				}
			}
			else if (material->color.green == 255) {
				if (material->color.red > 180 && material->color.red < 184) {
					registerMaterial(vehicle, material, eIndicatorState::Left);
				}
			}
		}

		if (material->color.red == 255 
		&& (material->color.green == 4 ||  material->color.green == 5) 
		&& material->color.blue == 128 
		&& std::string(material->texture->name).rfind("light", 0) == 0) {
			registerMaterial(vehicle, material, (material->color.green == 4) ? eIndicatorState::Left : eIndicatorState::Right);
		}
		return material;
	});

	VehicleMaterials::RegisterDummy([](CVehicle* pVeh, RwFrame* pFrame, std::string name, bool parent) {
		std::smatch match;
		if (std::regex_search(name, match, std::regex("^(turnl_|indicator_)(.{2})"))) {
			std::string stateStr = match.str(2);
			eIndicatorState state = (toupper(stateStr[0]) == 'L') ? eIndicatorState::Left : eIndicatorState::Right;
			eDummyPos rot = eDummyPos::None;
			
			if (toupper(stateStr[1]) == 'F') {
				rot = eDummyPos::Front;
			} else if (toupper(stateStr[1]) == 'R') {
				rot = eDummyPos::Rear;
			} else if (toupper(stateStr[1]) == 'M') {
				if (state == eIndicatorState::Left) {
					rot = eDummyPos::Left;
				} else if (state == eIndicatorState::Right) {
					rot = eDummyPos::Right;
				}
			}

			if (rot != eDummyPos::None) {
				registerDummy(pVeh, pFrame, name, parent, state, rot);
			}
		}
	});


	VehicleMaterials::RegisterRender([](CVehicle* pVeh) {
		if (pVeh->m_fHealth == 0) {
			return;
		}

		VehData &data = vehData.Get(pVeh);
		int model = pVeh->m_nModelIndex;
		eIndicatorState state = data.indicatorState;

		if (gConfig.ReadBoolean("FEATURES", "GlobalIndicators", false) == false && 
		dummies[model].size() == 0 && materials[model][state].size() == 0) {
			return;
		}
		
		if (pVeh->m_pDriver == FindPlayerPed()) {
			if (KeyPressed(VK_SHIFT)) {
				data.indicatorState = eIndicatorState::None;
				delay = 0;
				delayState = false;
			}

			if (KeyPressed(VK_Z)) {
				data.indicatorState = eIndicatorState::Left;
			}

			if (KeyPressed(VK_C)) { 
				data.indicatorState = eIndicatorState::Right;
			}

			if (KeyPressed(VK_X)) {
				data.indicatorState = eIndicatorState::Both;
			}
		} else if (pVeh->m_pDriver) {
			data.indicatorState = eIndicatorState::None;
			CVector2D prevPoint = GetCarPathLinkPosition(pVeh->m_autoPilot.m_nPreviousPathNodeInfo);
			CVector2D currPoint = GetCarPathLinkPosition(pVeh->m_autoPilot.m_nCurrentPathNodeInfo);
			CVector2D nextPoint = GetCarPathLinkPosition(pVeh->m_autoPilot.m_nNextPathNodeInfo);

			float angle = GetZAngleForPoint(nextPoint - currPoint) - GetZAngleForPoint(currPoint - prevPoint);
			while (angle < 0.0f) angle += 360.0f;
			while (angle > 360.0f) angle -= 360.0f;

			if (angle >= 30.0f && angle < 180.0f)
				data.indicatorState = eIndicatorState::Left;
			else if (angle <= 330.0f && angle > 180.0f)
				data.indicatorState = eIndicatorState::Right;

			if (data.indicatorState == eIndicatorState::None) {
				if (pVeh->m_autoPilot.m_nCurrentLane == 0 && pVeh->m_autoPilot.m_nNextLane == 1)
					data.indicatorState = eIndicatorState::Right;
				else if (pVeh->m_autoPilot.m_nCurrentLane == 1 && pVeh->m_autoPilot.m_nNextLane == 0)
					data.indicatorState = eIndicatorState::Left;
			}
		}

		if (state == eIndicatorState::None) {
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
			if (state != eIndicatorState::Both) {
				for (auto e: materials[model][state]){
					enableMaterial(e);
				}

				if (gConfig.ReadBoolean("FEATURES", "RenderShadows", false)) {
					for (auto e: dummies[model][state]) {
						Common::RegisterShadow(pVeh, e->Position, e->Color.red, e->Color.green, e->Color.blue, e->Angle, e->CurrentAngle);
					}
				}
			} else {
				for (auto k: materials[model]) {
					for (auto e: k.second) {
						enableMaterial(e);
					}
				}

				if (gConfig.ReadBoolean("FEATURES", "RenderShadows", false)) {
					for (auto k: dummies[model]) {
						for (auto e: k.second) {
							Common::RegisterShadow(pVeh, e->Position, e->Color.red, e->Color.green, e->Color.blue, e->Angle, e->CurrentAngle);
						}
					}
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

void Indicator::registerMaterial(CVehicle* pVeh, RpMaterial* material, eIndicatorState state) {
	materials[pVeh->m_nModelIndex][state].push_back(new VehicleMaterial(material));
};

void Indicator::registerDummy(CVehicle* pVeh, RwFrame* pFrame, std::string name, bool parent, eIndicatorState state, eDummyPos rot) {
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
};

void Indicator::enableMaterial(VehicleMaterial* material) {
	VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int*>(&material->Material->surfaceProps.ambient), *reinterpret_cast<unsigned int*>(&material->Material->surfaceProps.ambient)));
	material->Material->surfaceProps.ambient = 4.0;
	VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int*>(&material->Material->texture), *reinterpret_cast<unsigned int*>(&material->Material->texture)));
	material->Material->texture = material->TextureActive;
};

void Indicator::enableDummy(int id, VehicleDummy* dummy, CVehicle* vehicle, float vehicleAngle, float cameraAngle) {
	if (gConfig.ReadBoolean("FEATURES", "RenderCoronas", false)) {
		float cameraAngle = (TheCamera.GetHeading() * 180.0f) / 3.14f;
		Common::RegisterCoronaWithAngle(vehicle, dummy->Position, dummy->Color.red, dummy->Color.green, dummy->Color.blue, 
			CORONA_A, id, cameraAngle, dummy->CurrentAngle, 2.0f,  0.5f);
	}
};