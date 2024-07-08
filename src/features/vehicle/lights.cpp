#include "pch.h"
#include "lights.h"
#include <CClock.h>
#include <CCoronas.h>

void VehicleLights::Initialize() {
	VehicleMaterials::Register((VehicleMaterialFunction)[](CVehicle* vehicle, RpMaterial* material) {
		if (material->color.red == 255 && material->color.blue == 128) {
			if (material->color.green == 1)
				registerMaterial(vehicle, material, VehicleLightState::LightLeft);

			else if (material->color.green == 2)
				registerMaterial(vehicle, material, VehicleLightState::LightRight);

			else if (material->color.green == 3)
				registerMaterial(vehicle, material, VehicleLightState::TailLight);

			else if (material->color.green == 7)
				registerMaterial(vehicle, material, VehicleLightState::Daylight);

			else if (material->color.green == 8)
				registerMaterial(vehicle, material, VehicleLightState::Nightlight);

			else if (material->color.green == 9)
				registerMaterial(vehicle, material, VehicleLightState::Light);
		}
		else if (material->color.red == 255 && material->color.green == 173 && material->color.blue == 0)
			registerMaterial(vehicle, material, VehicleLightState::Reverselight);

		else if (material->color.red == 0 && material->color.green == 255 && material->color.blue == 198)
			registerMaterial(vehicle, material, VehicleLightState::Reverselight);

		else if (material->color.red == 184 && material->color.green == 255 && material->color.blue == 0)
			registerMaterial(vehicle, material, VehicleLightState::Brakelight);

		else if (material->color.red == 255 && material->color.green == 59 && material->color.blue == 0)
			registerMaterial(vehicle, material, VehicleLightState::Brakelight);

		else if (material->color.red == 0 && material->color.green == 18 && material->color.blue == 255)
			registerMaterial(vehicle, material, VehicleLightState::Daylight);

		else if (material->color.red == 0 && material->color.green == 16 && material->color.blue == 255)
			registerMaterial(vehicle, material, VehicleLightState::Nightlight);

		else if (material->color.red == 0 && material->color.green == 17 && material->color.blue == 255)
			registerMaterial(vehicle, material, VehicleLightState::Light);

		return material;
	});

	VehicleMaterials::RegisterDummy((VehicleDummyFunction)[](CVehicle* vehicle, RwFrame* frame, std::string name, bool parent) {
		int start = -1;

		VehicleLightState state = VehicleLightState::Something;

		if (name.rfind("breakl_", 0) == 0) {
			start = 6;

			state = VehicleLightState::Brakelight;
		}
		else if (name.rfind("breaklight_", 0) == 0) {
			start = 10;

			state = VehicleLightState::Brakelight;
		}
		else if (name.rfind("light_day", 0) == 0) {
			start = 9;

			state = VehicleLightState::Daylight;
		}
		else if (name.rfind("light_night", 0) == 0) {
			start = 10;

			state = VehicleLightState::Nightlight;
		}
		else if (name.rfind("light_", 0) == 0 && name.rfind("light_em", 0) != 0) {
			start = 5;

			state = VehicleLightState::Light;
		}
		else
			return;

		for (int _char = start; _char < (int)name.size(); _char++) {
			if (name[_char] != '_')
				continue;

			start = _char + 1;

			break;
		}

		int index = CPools::ms_pVehiclePool->GetIndex(vehicle);

		dummies[index][state].push_back(new VehicleDummy(frame, name, start, parent, 0, { 255, 255, 255, 128 }));
	});

	VehicleMaterials::RegisterRender((VehicleMaterialRender)[](CVehicle* vehicle, int index) {
		int model = vehicle->m_nModelIndex;

		if (materials[model].size() == 0)
			return;

		CAutomobile* automobile = reinterpret_cast<CAutomobile*>(vehicle);

		float vehicleAngle = (vehicle->GetHeading() * 180.0f) / 3.14f;

		float cameraAngle = (((CCamera*)0xB6F028)->GetHeading() * 180.0f) / 3.14f;

		renderLights(vehicle, VehicleLightState::Light, vehicleAngle, cameraAngle);

		if (CClock::GetIsTimeInRange(20, 8))
			renderLights(vehicle, VehicleLightState::Nightlight, vehicleAngle, cameraAngle);
		else
			renderLights(vehicle, VehicleLightState::Daylight, vehicleAngle, cameraAngle);

		if (vehicle->m_fBreakPedal)
			renderLights(vehicle, VehicleLightState::Brakelight, vehicleAngle, cameraAngle);

		if (vehicle->m_nVehicleFlags.bLightsOn) {
			if (materials[model][VehicleLightState::LightLeft].size() != 0) {
				if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_LEFT)) {
					renderLights(vehicle, VehicleLightState::LightLeft, vehicleAngle, cameraAngle);
				}
			}

			if (materials[model][VehicleLightState::LightRight].size() != 0) {
				if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_RIGHT)) {
					renderLights(vehicle, VehicleLightState::LightRight, vehicleAngle, cameraAngle);
				}
			}
		}
	});
};

void VehicleLights::renderLights(CVehicle* vehicle, VehicleLightState state, float vehicleAngle, float cameraAngle) {
	for (std::vector<VehicleMaterial*>::iterator material = materials[vehicle->m_nModelIndex][state].begin(); material != materials[vehicle->m_nModelIndex][state].end(); ++material)
		enableMaterial((*material));

	int id = (int)state * 100;

	int index = CPools::ms_pVehiclePool->GetIndex(vehicle);

	for (std::vector<VehicleDummy*>::iterator dummy = dummies[index][state].begin(); dummy != dummies[index][state].end(); ++dummy) {
		id++;

		enableDummy(id, (*dummy), vehicle, vehicleAngle, cameraAngle);
	}
};

void VehicleLights::registerMaterial(CVehicle* vehicle, RpMaterial* material, VehicleLightState state) {
	material->color.red = material->color.green = material->color.blue = 255;

	materials[vehicle->m_nModelIndex][state].push_back(new VehicleMaterial(material));
};

void VehicleLights::enableMaterial(VehicleMaterial* material) {
	VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int*>(&material->Material->surfaceProps.ambient), *reinterpret_cast<unsigned int*>(&material->Material->surfaceProps.ambient)));

	material->Material->surfaceProps.ambient = 4.0;

	VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int*>(&material->Material->texture), *reinterpret_cast<unsigned int*>(&material->Material->texture)));

	material->Material->texture = material->TextureActive;
};

void VehicleLights::enableDummy(int id, VehicleDummy* dummy, CVehicle* vehicle, float vehicleAngle, float cameraAngle) {
	if (dummy->Type < 2) {
		float dummyAngle = vehicleAngle + dummy->Angle;

		float differenceAngle = ((cameraAngle > dummyAngle) ? (cameraAngle - dummyAngle) : (dummyAngle - cameraAngle));

		if (differenceAngle < 90.0f || differenceAngle > 270.0f)
			return;
	}

	// (CPools::ms_pVehiclePool->GetIndex(vehicle) * 255) + id

	CCoronas::RegisterCorona((CPools::ms_pVehiclePool->GetIndex(vehicle) * 255) + id, vehicle, dummy->Color.red, dummy->Color.green, dummy->Color.blue, dummy->Color.alpha, dummy->Frame->modelling.pos,
	 	dummy->Size, 260.0f, eCoronaType::CORONATYPE_HEADLIGHT, eCoronaFlareType::FLARETYPE_NONE, false, false, 0, 0.0f, false, 0.5f, 0, 50.0f, false, true);
};
