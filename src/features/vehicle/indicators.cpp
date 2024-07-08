#include "pch.h"
#include "indicators.h"
#include "avs/common.h"
#include <CCoronas.h>

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

void VehicleIndicators::Initialize() {
	VehicleMaterials::Register((VehicleMaterialFunction)[](CVehicle* vehicle, RpMaterial* material) {
		if (material->color.blue == 0) {
			if (material->color.red == 255) {
				if (material->color.green > 55 && material->color.green < 59) {
					registerMaterial(vehicle, material, Right);
				
					return material;
				}
				else return material;
			}
			else if (material->color.green == 255) {
				if (material->color.red > 180 && material->color.red < 184) {
					registerMaterial(vehicle, material, Left);

					return material;
				}
				else return material;
			}
			else return material;
		}

		if (material->color.red != 255 || ((material->color.green < 4) || (material->color.green > 5)) || material->color.blue != 128 || std::string(material->texture->name).rfind("light", 0) != 0)
			return material;

		// indicator
		if (material->color.green == 4)
			registerMaterial(vehicle, material, Left);
		else
			registerMaterial(vehicle, material, Right);
	
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

		VehicleIndicatorState state = (toupper(name[start]) == 'L') ? (Left) : (Right);

		char position = tolower(name[start + 1]);

		int type = (position == 'f') ? (0) : ((position == 'r') ? (0) : (2));

		dummies[model][state].push_back(new VehicleDummy(frame, name, start + 3, parent, type, { 255, 98, 0, 128 }));
	});

	VehicleMaterials::RegisterRender((VehicleMaterialRender)[](CVehicle* vehicle, int index) {
		if (KeyPressed(VK_SHIFT)) {
			int model = vehicle->m_nModelIndex;

			if (materials[model].size() == 0)
				return;

			int index = CPools::ms_pVehiclePool->GetIndex(vehicle);

			if (!states.contains(index))
				return;

			states.erase(index);

			if (states.size() == 0) {
				delay = 0;

				delayState = false;
			}
		}

		if (KeyPressed(VK_Z)) {
			int model = vehicle->m_nModelIndex;

			if (materials[model].size() == 0)
				return;

			int index = CPools::ms_pVehiclePool->GetIndex(vehicle);

			states[index] = Left;
		};

		if (KeyPressed(VK_C)) { 
			int model = vehicle->m_nModelIndex;

			if (materials[model].size() == 0)
				return;

			int index = CPools::ms_pVehiclePool->GetIndex(vehicle);

			states[index] = Right;
		};

		if (KeyPressed(VK_X)) {
			int model = vehicle->m_nModelIndex;

			if (materials[model].size() == 0)
				return;

			int index = CPools::ms_pVehiclePool->GetIndex(vehicle);

			states[index] = Both;
		}

		if (!delayState)
			return;

		int model = vehicle->m_nModelIndex;

		if (materials[model].size() == 0)
			return;

		if (!states.contains(index))
			return;

		VehicleIndicatorState state = states[index];

		int id = 0;

		if (state != VehicleIndicatorState::Both) {
			if (materials[model][state].size() == 0)
				return;

			for (std::vector<RpMaterial*>::iterator material = materials[model][state].begin(); material != materials[model][state].end(); ++material)
				enableMaterial((*material));

			float vehicleAngle = (vehicle->GetHeading() * 180.0f) / 3.14f;

			float cameraAngle = (((CCamera*)0xB6F028)->GetHeading() * 180.0f) / 3.14f;

			for (std::vector<VehicleDummy*>::iterator dummy = dummies[model][state].begin(); dummy != dummies[model][state].end(); ++dummy) {
				id++;

				enableDummy(id, (*dummy), vehicle, vehicleAngle, cameraAngle);
			}
		}
		else {
			float vehicleAngle = (vehicle->GetHeading() * 180.0f) / 3.14f;

			float cameraAngle = (((CCamera*)0xB6F028)->GetHeading() * 180.0f) / 3.14f;

			for (std::vector<RpMaterial*>::iterator material = materials[model][VehicleIndicatorState::Left].begin(); material != materials[model][VehicleIndicatorState::Left].end(); ++material)
				enableMaterial((*material));

			for (std::vector<VehicleDummy*>::iterator dummy = dummies[model][VehicleIndicatorState::Left].begin(); dummy != dummies[model][VehicleIndicatorState::Left].end(); ++dummy) {
				id++;

				enableDummy(id, (*dummy), vehicle, vehicleAngle, cameraAngle);
			}

			for (std::vector<RpMaterial*>::iterator material = materials[model][VehicleIndicatorState::Right].begin(); material != materials[model][VehicleIndicatorState::Right].end(); ++material)
				enableMaterial((*material));

			for (std::vector<VehicleDummy*>::iterator dummy = dummies[model][VehicleIndicatorState::Right].begin(); dummy != dummies[model][VehicleIndicatorState::Right].end(); ++dummy) {
				id++;

				enableDummy(id, (*dummy), vehicle, vehicleAngle, cameraAngle);
			}
		}
	});

	plugin::Events::drawingEvent += []() {
		if (states.size() == 0)
			return;

		uint64_t timestamp = Common::TimeSinceEpochMillisec();

		if ((timestamp - delay) < 500)
			return;

		delay = timestamp;
		delayState = !delayState;
	};
};

void VehicleIndicators::registerMaterial(CVehicle* vehicle, RpMaterial* &material, VehicleIndicatorState state) {
	material->color.red = material->color.green = material->color.blue = 255;
	materials[vehicle->m_nModelIndex][state].push_back(material);
};

void VehicleIndicators::enableMaterial(RpMaterial* material) {
	VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int*>(&material->surfaceProps.ambient), *reinterpret_cast<unsigned int*>(&material->surfaceProps.ambient)));

	material->surfaceProps.ambient = 4.0;
};

void VehicleIndicators::enableDummy(int id, VehicleDummy* dummy, CVehicle* vehicle, float vehicleAngle, float cameraAngle) {
	if (dummy->Type < 2) {
		float dummyAngle = vehicleAngle + dummy->Angle;

		float differenceAngle = ((cameraAngle > dummyAngle) ? (cameraAngle - dummyAngle) : (dummyAngle - cameraAngle));

		if (differenceAngle < 90.0f || differenceAngle > 270.0f)
			return;
	}

	CCoronas::RegisterCorona(reinterpret_cast<unsigned int>(vehicle) + 30 + id, vehicle, dummy->Color.red, dummy->Color.green, dummy->Color.blue, dummy->Color.alpha, dummy->Position,
		dummy->Size, 260.0f, eCoronaType::CORONATYPE_HEADLIGHT, eCoronaFlareType::FLARETYPE_NONE, false, false, 0, 0.0f, false, 0.5f, 0, 50.0f, false, true);
};
