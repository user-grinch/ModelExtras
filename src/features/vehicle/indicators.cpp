#include "pch.h"
#include "indicators.h"
#include "internals/common.h"

CVector2D GetCarPathLinkPosition(CCarPathLinkAddress &address) {
    if (address.m_nAreaId != -1 && address.m_nCarPathLinkId != -1 && ThePaths.m_pPathNodes[address.m_nAreaId]) {
        return CVector2D(static_cast<float>(ThePaths.m_pNaviNodes[address.m_nAreaId][address.m_nCarPathLinkId].m_vecPosn.x) / 8.0f,
            static_cast<float>(ThePaths.m_pNaviNodes[address.m_nAreaId][address.m_nCarPathLinkId].m_vecPosn.y) / 8.0f);
    }
    return CVector2D(0.0f, 0.0f);
}

void DrawTurnlight(CVehicle *pVeh, eDummyRotation indicatorPos, bool leftSide) {
    CVector posn =
        reinterpret_cast<CVehicleModelInfo *>(CModelInfo::ms_modelInfoPtrs[pVeh->m_nModelIndex])->m_pVehicleStruct->m_avDummyPos[static_cast<int>(indicatorPos)];
	
    if (posn.x == 0.0f) posn.x = 0.15f;
    if (leftSide) posn.x *= -1.0f;
	int dummyId = static_cast<int>(indicatorPos) + (leftSide ? 0 : 2);
	float dummyAngle = (indicatorPos == eDummyRotation::Backward) ? 180.0f : 0.0f;
	Common::RegisterShadow(pVeh, posn, 255, 128, 0, dummyAngle, 0.0f);
    Common::RegisterCorona(pVeh, posn, 255, 128, 0, 255, dummyId, 0.3f, dummyAngle);
}

void DrawVehicleTurnlights(CVehicle *vehicle, eIndicatorState lightsStatus) {
    if (lightsStatus == eIndicatorState::Both || lightsStatus == eIndicatorState::Right) {
        DrawTurnlight(vehicle, eDummyRotation::Forward, false);
        DrawTurnlight(vehicle, eDummyRotation::Backward, false);
    }
    if (lightsStatus == eIndicatorState::Both || lightsStatus == eIndicatorState::Left) {
        DrawTurnlight(vehicle, eDummyRotation::Forward, true);
        DrawTurnlight(vehicle, eDummyRotation::Backward, true);
    }
}

float GetZAngleForPoint(CVector2D const &point) {
    float angle = CGeneral::GetATanOfXY(point.x, point.y) * 57.295776f - 90.0f;
    while (angle < 0.0f) angle += 360.0f;
    return angle;
}

IndicatorFeature Indicator;
void IndicatorFeature::Initialize() {
	VehicleMaterials::Register([](CVehicle* vehicle, RpMaterial* material) {
		if (material->color.blue == 0) {
			if (material->color.red == 255) {
				if (material->color.green > 55 && material->color.green < 59) {
					Indicator.registerMaterial(vehicle, material, eIndicatorState::Right);
				
					return material;
				}
				else return material;
			}
			else if (material->color.green == 255) {
				if (material->color.red > 180 && material->color.red < 184) {
					Indicator.registerMaterial(vehicle, material, eIndicatorState::Left);

					return material;
				}
				else return material;
			}
			else return material;
		}

		if (material->color.red != 255 || ((material->color.green < 4) || (material->color.green > 5)) || material->color.blue != 128 || std::string(material->texture->name).rfind("light", 0) != 0)
			return material;

		Indicator.registerMaterial(vehicle, material, (material->color.green == 4) ? eIndicatorState::Left : eIndicatorState::Right);
	
		return material;
	});

	VehicleMaterials::RegisterDummy([](CVehicle* pVeh, RwFrame* pFrame, std::string name, bool parent) {
		std::smatch match;
		if (std::regex_search(name, match, std::regex("^(turnl_|indicator_)(.{2})"))) {
			std::string stateStr = match.str(2);
			eIndicatorState state = (toupper(stateStr[0]) == 'L') ? eIndicatorState::Left : eIndicatorState::Right;
			eDummyRotation rot = (toupper(stateStr[1]) == 'F') ? eDummyRotation::Forward : eDummyRotation::Backward;
			Indicator.registerDummy(pVeh, pFrame, name, parent, state, rot);
		}
	});


	VehicleMaterials::RegisterRender([this](CVehicle* pVeh) {
		VehData &data = vehData.Get(pVeh);

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
		} else {
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

		if (!Indicator.delayState)
			return;

		int model = pVeh->m_nModelIndex;
		eIndicatorState state = data.indicatorState;
		if (state == eIndicatorState::None) {
			return;
		}

		// global turn lights
		if (gConfig.ReadBoolean("FEATURES", "GlobalIndicators", false) &&
			Indicator.dummies[model].size() == 0 && Indicator.materials[model][state].size() == 0)
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
			return;
		}
			
		

		int id = 0;

		if (state != eIndicatorState::Both) {
			if (Indicator.materials[model][state].size() == 0)
				return;

			for (std::vector<RpMaterial*>::iterator material = Indicator.materials[model][state].begin(); material != Indicator.materials[model][state].end(); ++material)
				Indicator.enableMaterial((*material));

			if (gConfig.ReadBoolean("FEATURES", "RenderShadows", false) || gConfig.ReadBoolean("FEATURES", "RenderCoronas", false)) {
				float vehicleAngle = (pVeh->GetHeading() * 180.0f) / 3.14f;
				float cameraAngle = (((CCamera*)0xB6F028)->GetHeading() * 180.0f) / 3.14f;

				for (std::vector<VehicleDummy*>::iterator dummy = Indicator.dummies[model][state].begin(); dummy != Indicator.dummies[model][state].end(); ++dummy) {
					id++;

					Indicator.enableDummy(id, (*dummy), pVeh, vehicleAngle, cameraAngle);
				}
			}
		}
		else {
			for (std::vector<RpMaterial*>::iterator material = Indicator.materials[model][eIndicatorState::Left].begin(); material != Indicator.materials[model][eIndicatorState::Left].end(); ++material)
				Indicator.enableMaterial((*material));

			for (std::vector<RpMaterial*>::iterator material = Indicator.materials[model][eIndicatorState::Right].begin(); material != Indicator.materials[model][eIndicatorState::Right].end(); ++material)
				Indicator.enableMaterial((*material));

			float vehicleAngle = (pVeh->GetHeading() * 180.0f) / 3.14f;
			float cameraAngle = (TheCamera.GetHeading() * 180.0f) / 3.14f;

			if (gConfig.ReadBoolean("FEATURES", "RenderShadows", false) || gConfig.ReadBoolean("FEATURES", "RenderCoronas", false)) {
				for (std::vector<VehicleDummy*>::iterator dummy = Indicator.dummies[model][eIndicatorState::Left].begin(); dummy != Indicator.dummies[model][eIndicatorState::Left].end(); ++dummy) {
					id++;

					Indicator.enableDummy(id, (*dummy), pVeh, vehicleAngle, cameraAngle);
				}

				for (std::vector<VehicleDummy*>::iterator dummy = Indicator.dummies[model][eIndicatorState::Right].begin(); dummy != Indicator.dummies[model][eIndicatorState::Right].end(); ++dummy) {
					id++;

					Indicator.enableDummy(id, (*dummy), pVeh, vehicleAngle, cameraAngle);
				}
			}
		}
	});

	Events::drawingEvent += []() {
		size_t timestamp = CTimer::m_snTimeInMilliseconds;
		
		if ((timestamp - Indicator.delay) < 500)
			return;

		Indicator.delay = timestamp;
		Indicator.delayState = !Indicator.delayState;
	};
};

void IndicatorFeature::registerMaterial(CVehicle* pVeh, RpMaterial* &material, eIndicatorState state) {
	material->color.red = material->color.green = material->color.blue = 255;
	materials[pVeh->m_nModelIndex][state].push_back(material);
};

void IndicatorFeature::registerDummy(CVehicle* pVeh, RwFrame* pFrame, std::string name, bool parent, eIndicatorState state, eDummyRotation rot) {
	dummies[pVeh->m_nModelIndex][state].push_back(new VehicleDummy(pFrame, name, parent, rot, { 255, 98, 0, 128 }));
};

void IndicatorFeature::enableMaterial(RpMaterial* material) {
	VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int*>(&material->surfaceProps.ambient), *reinterpret_cast<unsigned int*>(&material->surfaceProps.ambient)));
	material->surfaceProps.ambient = 4.0;
};

void IndicatorFeature::enableDummy(int id, VehicleDummy* dummy, CVehicle* vehicle, float vehicleAngle, float cameraAngle) {
	if (gConfig.ReadBoolean("FEATURES", "RenderShadows", false)) {
		Common::RegisterShadow(vehicle, dummy->Position, dummy->Color.red, dummy->Color.green, dummy->Color.blue, 
			dummy->Angle, dummy->CurrentAngle);
	}

	if (gConfig.ReadBoolean("FEATURES", "RenderCoronas", false)) {
		Common::RegisterCorona(vehicle, dummy->Position, dummy->Color.red, dummy->Color.green, dummy->Color.blue, 
			80, id, dummy->Size, dummy->CurrentAngle);
	}
};