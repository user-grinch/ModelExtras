#include "pch.h"
#include "foglights.h"
#include "internals/common.h"

FogLightsFeature FogLights;

void FogLightsFeature::Initialize() {
	VehicleMaterials::Register((VehicleMaterialFunction)[](CVehicle* vehicle, RpMaterial* material) {
		if (material->color.red == 255 && material->color.blue == 128) {
			if (std::string(material->texture->name).rfind("light", 0) == 0) {
				if (material->color.green == 6)
					FogLights.registerMaterial(vehicle, material, eFogLightState::Left);

				else if (material->color.green == 7)
					FogLights.registerMaterial(vehicle, material, eFogLightState::Right);
			}
		}
		else if (material->color.red == 255 && material->color.green == 174 && material->color.blue == 0)
			FogLights.registerMaterial(vehicle, material, eFogLightState::Left);

		else if (material->color.red == 0 && material->color.green == 255 && material->color.blue == 199)
			FogLights.registerMaterial(vehicle, material, eFogLightState::Right);
		
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

		eFogLightState state = (toupper(name[start]) == 'L') ? (eFogLightState::Left) : (eFogLightState::Right);

		for (int _char = start; _char < (int)name.size(); _char++) {
			if (name[_char] != '_')
				continue;

			start = _char + 1;

			break;
		}

		FogLights.dummies[model][state].push_back(new VehicleDummy(frame, name, start, parent, eDummyPos::Backward, { 255, 255, 255, 128 }));
	});

	Events::processScriptsEvent += [this]() {
		CVehicle *vehicle = FindPlayerVehicle(-1, false);
		if (!vehicle) {
			return;
		}

		static size_t prev = 0;
		if (KeyPressed(VK_J)) {
			size_t now = CTimer::m_snTimeInMilliseconds;
			if (now - prev > 500.0f) {

				int model = vehicle->m_nModelIndex;

				if (materials[model].size() == 0)
					return;

				int index = CPools::ms_pVehiclePool->GetIndex(vehicle);

				if (states.contains(index))
					states.erase(index);
				else
					states[index] = true;
				prev = now;
			}
		}
	};

	VehicleMaterials::RegisterRender((VehicleMaterialRender)[](CVehicle* vehicle, int index) {
		int model = vehicle->m_nModelIndex;

		if (FogLights.materials[model].size() == 0)
			return;

		if (vehicle->m_nVehicleFlags.bSirenOrAlarm == false) {
			int index = CPools::ms_pVehiclePool->GetIndex(vehicle);

			if (!FogLights.states.contains(index))
				return;
		}

		CAutomobile* automobile = reinterpret_cast<CAutomobile*>(vehicle);

		float vehicleAngle = (vehicle->GetHeading() * 180.0f) / 3.14f;

		float cameraAngle = (((CCamera*)0xB6F028)->GetHeading() * 180.0f) / 3.14f;

		if (FogLights.materials[model][eFogLightState::Left].size() != 0) {
			if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_LEFT)) {
				for (std::vector<VehicleMaterial*>::iterator material = FogLights.materials[model][eFogLightState::Left].begin(); material != FogLights.materials[model][eFogLightState::Left].end(); ++material)
					FogLights.enableMaterial((*material));

				for (std::vector<VehicleDummy*>::iterator dummy = FogLights.dummies[model][eFogLightState::Left].begin(); dummy != FogLights.dummies[model][eFogLightState::Left].end(); ++dummy)
					FogLights.enableDummy((*dummy), vehicle, vehicleAngle, cameraAngle);
			}
		}

		if (FogLights.materials[model][eFogLightState::Right].size() != 0) {
			if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_RIGHT)) {
				for (std::vector<VehicleMaterial*>::iterator material = FogLights.materials[model][eFogLightState::Right].begin(); material != FogLights.materials[model][eFogLightState::Right].end(); ++material)
					FogLights.enableMaterial((*material));

				for (std::vector<VehicleDummy*>::iterator dummy = FogLights.dummies[model][eFogLightState::Right].begin(); dummy != FogLights.dummies[model][eFogLightState::Right].end(); ++dummy)
					FogLights.enableDummy((*dummy), vehicle, vehicleAngle, cameraAngle);
			}
		}
	});
};

void FogLightsFeature::registerMaterial(CVehicle* vehicle, RpMaterial* material, eFogLightState state) {
	material->color.red = material->color.green = material->color.blue = 255;
	materials[vehicle->m_nModelIndex][state].push_back(new VehicleMaterial(material));
};

void FogLightsFeature::enableMaterial(VehicleMaterial* material) {
	VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int*>(&material->Material->surfaceProps.ambient), *reinterpret_cast<unsigned int*>(&material->Material->surfaceProps.ambient)));

	material->Material->surfaceProps.ambient = 4.0;

	VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int*>(&material->Material->texture), *reinterpret_cast<unsigned int*>(&material->Material->texture)));

	material->Material->texture = material->TextureActive;
};

void FogLightsFeature::enableDummy(VehicleDummy* dummy, CVehicle* vehicle, float vehicleAngle, float cameraAngle) {
	// if (dummy->Type < 2) {
	// 	float dummyAngle = vehicleAngle + dummy->Angle;

	// 	float differenceAngle = ((cameraAngle > dummyAngle) ? (cameraAngle - dummyAngle) : (dummyAngle - cameraAngle));

	// 	if (differenceAngle < 90.0f || differenceAngle > 270.0f)
	// 		return;
	// }

	Common::RegisterShadow(vehicle, dummy->Position, dummy->Color.red, dummy->Color.green, dummy->Color.blue, dummy->Angle, dummy->CurrentAngle);
	Common::RegisterCorona(vehicle, dummy->Position, dummy->Color.red, dummy->Color.green, dummy->Color.blue, dummy->Color.alpha, 20, dummy->Size);
};