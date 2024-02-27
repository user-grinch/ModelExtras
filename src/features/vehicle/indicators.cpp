#include "pch.h"
#include "indicators.h"
#include <CCoronas.h>

void RegisterCoronaEx(CEntity* ent, int id, unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha, CVector const& posn, float size) {
	return CCoronas::RegisterCorona(reinterpret_cast<unsigned int>(ent) + 30 + id, ent, red, green, blue, alpha, posn,
		size, 150.0f, CORONATYPE_HEADLIGHT, FLARETYPE_NONE, false, false, 0, 0.0f, false, 0.5f, 0, 50.0f, false, false);
};

CVector2D GetCarPathLinkPosition(CCarPathLinkAddress &address) {
    if (address.m_nAreaId != -1 && address.m_nCarPathLinkId != -1 && ThePaths.m_pPathNodes[address.m_nAreaId]) {
        return CVector2D(static_cast<float>(ThePaths.m_pNaviNodes[address.m_nAreaId][address.m_nCarPathLinkId].m_vecPosn.x) / 8.0f,
            static_cast<float>(ThePaths.m_pNaviNodes[address.m_nAreaId][address.m_nCarPathLinkId].m_vecPosn.y) / 8.0f);
    }
    return CVector2D(0.0f, 0.0f);
}

void DrawTurnlight(CVehicle *vehicle, unsigned int dummyId, bool leftSide) {
    CVector posn =
        reinterpret_cast<CVehicleModelInfo *>(CModelInfo::ms_modelInfoPtrs[vehicle->m_nModelIndex])->m_pVehicleStruct->m_avDummyPos[dummyId];
    if (posn.x == 0.0f) posn.x = 0.15f;
    if (leftSide) posn.x *= -1.0f;
	dummyId += (leftSide ? 0 : 2);
    RegisterCoronaEx(vehicle, dummyId, 255, 128, 0, 255, posn, 0.3f);
}

void DrawVehicleTurnlights(CVehicle *vehicle, eIndicatorState lightsStatus) {
    if (lightsStatus == eIndicatorState::Both || lightsStatus == eIndicatorState::Right) {
        DrawTurnlight(vehicle, 0, false);
        DrawTurnlight(vehicle, 1, false);
    }
    if (lightsStatus == eIndicatorState::Both || lightsStatus == eIndicatorState::Left) {
        DrawTurnlight(vehicle, 0, true);
        DrawTurnlight(vehicle, 1, true);
    }
}

float GetZAngleForPoint(CVector2D const &point) {
    float angle = CGeneral::GetATanOfXY(point.x, point.y) * 57.295776f - 90.0f;
    while (angle < 0.0f) angle += 360.0f;
    return angle;
}

IndicatorFeature Indicator;
void IndicatorFeature::Initialize() {
	VehicleMaterials::Register((VehicleMaterialFunction)[](CVehicle* vehicle, RpMaterial* material) {
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

	VehicleMaterials::RegisterDummy((VehicleDummyFunction)[](CVehicle* vehicle, RwFrame* frame, std::string name, bool parent) {
		int start = -1;

		if (name.rfind("turnl_", 0) == 0)
			start = 6;
		else if (name.rfind("indicator_", 0) == 0)
			start = 10;

		if (start == -1)
			return;

		int model = vehicle->m_nModelIndex;

		eIndicatorState state = (toupper(name[start]) == 'L') ? (eIndicatorState::Left) : (eIndicatorState::Right);

		char position = tolower(name[start + 1]);

		int type = (position == 'f') ? (0) : ((position == 'r') ? (0) : (2));

		Indicator.dummies[model][state].push_back(new VehicleDummy(frame, name, start + 3, parent, type, { 255, 98, 0, 128 }));
	});
	
	Events::processScriptsEvent += [this]() {
		CVehicle *vehicle = FindPlayerVehicle(-1, false);

		if (!vehicle) {
			return;
		}

		VehData &data = vehData.Get(vehicle);

		if (vehicle->m_pDriver == FindPlayerPed()) {
			if (KeyPressed(90)) { // Z
				data.indicatorState = eIndicatorState::None;
				delay = 0;
				delayState = false;
			}

			if (KeyPressed(88)) { // X
				data.indicatorState = eIndicatorState::Left;
			}

			if (KeyPressed(67)) { // C
				data.indicatorState = eIndicatorState::Right;
			}

			if (KeyPressed(VK_SHIFT)) {
				data.indicatorState = eIndicatorState::Both;
			}
		} else {
			data.indicatorState = eIndicatorState::None;
			CVector2D prevPoint = GetCarPathLinkPosition(vehicle->m_autoPilot.m_nPreviousPathNodeInfo);
			CVector2D currPoint = GetCarPathLinkPosition(vehicle->m_autoPilot.m_nCurrentPathNodeInfo);
			CVector2D nextPoint = GetCarPathLinkPosition(vehicle->m_autoPilot.m_nNextPathNodeInfo);

			float angle = GetZAngleForPoint(nextPoint - currPoint) - GetZAngleForPoint(currPoint - prevPoint);
			while (angle < 0.0f) angle += 360.0f;
			while (angle > 360.0f) angle -= 360.0f;

			if (angle >= 30.0f && angle < 180.0f)
				data.indicatorState = eIndicatorState::Left;
			else if (angle <= 330.0f && angle > 180.0f)
				data.indicatorState = eIndicatorState::Right;

			if (data.indicatorState == eIndicatorState::None) {
				if (vehicle->m_autoPilot.m_nCurrentLane == 0 && vehicle->m_autoPilot.m_nNextLane == 1)
					data.indicatorState = eIndicatorState::Right;
				else if (vehicle->m_autoPilot.m_nCurrentLane == 1 && vehicle->m_autoPilot.m_nNextLane == 0)
					data.indicatorState = eIndicatorState::Left;
			}
		}
	};

	VehicleMaterials::RegisterRender((VehicleMaterialRender)[](CVehicle* vehicle, int index) {
		if (!Indicator.delayState)
			return;

		int model = vehicle->m_nModelIndex;
		VehData &data = Indicator.vehData.Get(vehicle);
		eIndicatorState state = data.indicatorState;
		if (state == eIndicatorState::None) {
			return;
		}

		// global turn lights
		if (Indicator.dummies[model].size() == 0)
		{
			if ((vehicle->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || vehicle->m_nVehicleSubClass == VEHICLE_BIKE) &&
				(vehicle->GetVehicleAppearance() == VEHICLE_APPEARANCE_AUTOMOBILE || vehicle->GetVehicleAppearance() == VEHICLE_APPEARANCE_BIKE) &&
				vehicle->m_nVehicleFlags.bEngineOn && vehicle->m_fHealth > 0 && !vehicle->m_nVehicleFlags.bIsDrowning && !vehicle->m_pAttachedTo )
			{
				if (DistanceBetweenPoints(TheCamera.m_vecGameCamPos, vehicle->GetPosition()) < 150.0f) {
					DrawVehicleTurnlights(vehicle, state);
					if (vehicle->m_pTractor)
						DrawVehicleTurnlights(vehicle->m_pTractor, state);
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

			float vehicleAngle = (vehicle->GetHeading() * 180.0f) / 3.14f;

			float cameraAngle = (((CCamera*)0xB6F028)->GetHeading() * 180.0f) / 3.14f;

			for (std::vector<VehicleDummy*>::iterator dummy = Indicator.dummies[model][state].begin(); dummy != Indicator.dummies[model][state].end(); ++dummy) {
				id++;

				Indicator.enableDummy(id, (*dummy), vehicle, vehicleAngle, cameraAngle);
			}
		}
		else {
			float vehicleAngle = (vehicle->GetHeading() * 180.0f) / 3.14f;

			float cameraAngle = (((CCamera*)0xB6F028)->GetHeading() * 180.0f) / 3.14f;

			for (std::vector<RpMaterial*>::iterator material = Indicator.materials[model][eIndicatorState::Left].begin(); material != Indicator.materials[model][eIndicatorState::Left].end(); ++material)
				Indicator.enableMaterial((*material));

			for (std::vector<VehicleDummy*>::iterator dummy = Indicator.dummies[model][eIndicatorState::Left].begin(); dummy != Indicator.dummies[model][eIndicatorState::Left].end(); ++dummy) {
				id++;

				Indicator.enableDummy(id, (*dummy), vehicle, vehicleAngle, cameraAngle);
			}

			for (std::vector<RpMaterial*>::iterator material = Indicator.materials[model][eIndicatorState::Right].begin(); material != Indicator.materials[model][eIndicatorState::Right].end(); ++material)
				Indicator.enableMaterial((*material));

			for (std::vector<VehicleDummy*>::iterator dummy = Indicator.dummies[model][eIndicatorState::Right].begin(); dummy != Indicator.dummies[model][eIndicatorState::Right].end(); ++dummy) {
				id++;

				Indicator.enableDummy(id, (*dummy), vehicle, vehicleAngle, cameraAngle);
			}
		}
	});

	plugin::Events::drawingEvent += []() {
		size_t timestamp = CTimer::m_snTimeInMilliseconds;
		
		if ((timestamp - Indicator.delay) < 500)
			return;

		Indicator.delay = timestamp;
		Indicator.delayState = !Indicator.delayState;
	};
};

void IndicatorFeature::registerMaterial(CVehicle* vehicle, RpMaterial* &material, eIndicatorState state) {
	material->color.red = material->color.green = material->color.blue = 255;
	materials[vehicle->m_nModelIndex][state].push_back(material);
};

void IndicatorFeature::enableMaterial(RpMaterial* material) {
	VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int*>(&material->surfaceProps.ambient), *reinterpret_cast<unsigned int*>(&material->surfaceProps.ambient)));
	material->surfaceProps.ambient = 4.0;
};

void IndicatorFeature::enableDummy(int id, VehicleDummy* dummy, CVehicle* vehicle, float vehicleAngle, float cameraAngle) {
	if (dummy->Type < 2) {
		float dummyAngle = vehicleAngle + dummy->Angle;

		float differenceAngle = ((cameraAngle > dummyAngle) ? (cameraAngle - dummyAngle) : (dummyAngle - cameraAngle));

		if (differenceAngle < 90.0f || differenceAngle > 270.0f)
			return;
	}

	RegisterCoronaEx(vehicle, id, dummy->Color.red, dummy->Color.green, dummy->Color.blue, dummy->Color.alpha, dummy->Position, dummy->Size);
};