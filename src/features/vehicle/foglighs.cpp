#include "pch.h"
#include "foglights.h"
#include <CCoronas.h>

void VehicleFoglights::Initialize() {
	VehicleMaterials::Register((VehicleMaterialFunction)[](CVehicle* vehicle, RpMaterial* material) {
		if (material->color.red == 255 && material->color.blue == 128) {
			if (std::string(material->texture->name).rfind("light", 0) == 0) {
				if (material->color.green == 6)
					registerMaterial(vehicle, material, VehicleFoglightState::FoglightLeft);

				else if (material->color.green == 7)
					registerMaterial(vehicle, material, VehicleFoglightState::FoglightRight);
			}
		}
		else if (material->color.red == 255 && material->color.green == 174 && material->color.blue == 0)
			registerMaterial(vehicle, material, VehicleFoglightState::FoglightLeft);

		else if (material->color.red == 0 && material->color.green == 255 && material->color.blue == 199)
			registerMaterial(vehicle, material, VehicleFoglightState::FoglightRight);
		
		return material;
	});

	VehicleMaterials::RegisterDummy((VehicleDummyFunction)[](CVehicle* vehicle, RwFrame* frame, std::string name, bool parent) {
		int start = -1;

		if (name.rfind("fogl_", 0) == 0)
			start = 5;
		else if (name.rfind("foglight_", 0) == 0)
			start = 9;

		if (start == -1)
			return;

		int model = vehicle->m_nModelIndex;

		VehicleFoglightState state = (toupper(name[start]) == 'L') ? (VehicleFoglightState::FoglightLeft) : (VehicleFoglightState::FoglightRight);

		for (int _char = start; _char < (int)name.size(); _char++) {
			if (name[_char] != '_')
				continue;

			start = _char + 1;

			break;
		}

		dummies[model][state].push_back(new VehicleDummy(frame, name, start, parent, 0, { 255, 255, 255, 128 }));
	});

	VehicleMaterials::RegisterRender((VehicleMaterialRender)[](CVehicle* vehicle, int index) {
		int model = vehicle->m_nModelIndex;
		if (KeyPressed(VK_J)) {
			if (materials[model].size() == 0)
				return;

			int index = CPools::ms_pVehiclePool->GetIndex(vehicle);

			if (states.contains(index))
				states.erase(index);
			else
				states[index] = true;
		}

		if (materials[model].size() == 0)
			return;

		if (vehicle->m_nVehicleFlags.bSirenOrAlarm == false) {
			int index = CPools::ms_pVehiclePool->GetIndex(vehicle);

			if (!states.contains(index))
				return;
		}

		CAutomobile* automobile = reinterpret_cast<CAutomobile*>(vehicle);

		float vehicleAngle = (vehicle->GetHeading() * 180.0f) / 3.14f;

		float cameraAngle = (((CCamera*)0xB6F028)->GetHeading() * 180.0f) / 3.14f;

		if (materials[model][VehicleFoglightState::FoglightLeft].size() != 0) {
			if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_LEFT)) {
				for (std::vector<VehicleMaterial*>::iterator material = materials[model][VehicleFoglightState::FoglightLeft].begin(); material != materials[model][VehicleFoglightState::FoglightLeft].end(); ++material)
					enableMaterial((*material));

				for (std::vector<VehicleDummy*>::iterator dummy = dummies[model][VehicleFoglightState::FoglightLeft].begin(); dummy != dummies[model][VehicleFoglightState::FoglightLeft].end(); ++dummy)
					enableDummy((*dummy), vehicle, vehicleAngle, cameraAngle);
			}
		}

		if (materials[model][VehicleFoglightState::FoglightRight].size() != 0) {
			if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_RIGHT)) {
				for (std::vector<VehicleMaterial*>::iterator material = materials[model][VehicleFoglightState::FoglightRight].begin(); material != materials[model][VehicleFoglightState::FoglightRight].end(); ++material)
					enableMaterial((*material));

				for (std::vector<VehicleDummy*>::iterator dummy = dummies[model][VehicleFoglightState::FoglightRight].begin(); dummy != dummies[model][VehicleFoglightState::FoglightRight].end(); ++dummy)
					enableDummy((*dummy), vehicle, vehicleAngle, cameraAngle);
			}
		}
	});
};

void VehicleFoglights::registerMaterial(CVehicle* vehicle, RpMaterial* material, VehicleFoglightState state) {
	material->color.red = material->color.green = material->color.blue = 255;
	materials[vehicle->m_nModelIndex][state].push_back(new VehicleMaterial(material));
};

void VehicleFoglights::enableMaterial(VehicleMaterial* material) {
	VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int*>(&material->Material->surfaceProps.ambient), *reinterpret_cast<unsigned int*>(&material->Material->surfaceProps.ambient)));

	material->Material->surfaceProps.ambient = 4.0;

	VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int*>(&material->Material->texture), *reinterpret_cast<unsigned int*>(&material->Material->texture)));

	material->Material->texture = material->TextureActive;
};

void VehicleFoglights::enableDummy(VehicleDummy* dummy, CVehicle* vehicle, float vehicleAngle, float cameraAngle) {
	if (dummy->Type < 2) {
		float dummyAngle = vehicleAngle + dummy->Angle;

		float differenceAngle = ((cameraAngle > dummyAngle) ? (cameraAngle - dummyAngle) : (dummyAngle - cameraAngle));

		if (differenceAngle < 90.0f || differenceAngle > 270.0f)
			return;
	}

	CCoronas::RegisterCorona(reinterpret_cast<unsigned int>(vehicle) + 40 + reinterpret_cast<unsigned int>(dummy), vehicle, dummy->Color.red, dummy->Color.green, dummy->Color.blue, dummy->Color.alpha, dummy->Position,
		dummy->Size, 260.0f, eCoronaType::CORONATYPE_HEADLIGHT, eCoronaFlareType::FLARETYPE_NONE, false, false, 0, 0.0f, false, 0.5f, 0, 50.0f, false, true);
};
