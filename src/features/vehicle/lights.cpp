#include "pch.h"
#include "lights.h"
#include <CClock.h>
#include "internals/common.h"

LightsFeature Lights;
#include <CHud.h>
void LightsFeature::Initialize() {
	VehicleMaterials::Register([](CVehicle* vehicle, RpMaterial* material) {
		if (material->color.red == 255 && material->color.blue == 128) {
			if (material->color.green == 1)
				Lights.registerMaterial(vehicle, material, eLightState::LightLeft);

			else if (material->color.green == 2)
				Lights.registerMaterial(vehicle, material, eLightState::LightRight);

			else if (material->color.green == 3)
				Lights.registerMaterial(vehicle, material, eLightState::TailLight);
			else if (material->color.green == 6)
				Lights.registerMaterial(vehicle, material, eLightState::FogLightLeft);

			else if (material->color.green == 7) {
				Lights.registerMaterial(vehicle, material, eLightState::FogLightRight);
				Lights.registerMaterial(vehicle, material, eLightState::Daylight);
			}
			else if (material->color.green == 8)
				Lights.registerMaterial(vehicle, material, eLightState::Nightlight);

			else if (material->color.green == 9)
				Lights.registerMaterial(vehicle, material, eLightState::Light);
		}
		else if (material->color.red == 255 && material->color.green == 173 && material->color.blue == 0)
			Lights.registerMaterial(vehicle, material, eLightState::Reverselight);

		else if (material->color.red == 0 && material->color.green == 255 && material->color.blue == 198)
			Lights.registerMaterial(vehicle, material, eLightState::Reverselight);

		else if (material->color.red == 184 && material->color.green == 255 && material->color.blue == 0)
			Lights.registerMaterial(vehicle, material, eLightState::Brakelight);

		else if (material->color.red == 255 && material->color.green == 59 && material->color.blue == 0)
			Lights.registerMaterial(vehicle, material, eLightState::Brakelight);

		else if (material->color.red == 0 && material->color.green == 18 && material->color.blue == 255)
			Lights.registerMaterial(vehicle, material, eLightState::Daylight);

		else if (material->color.red == 0 && material->color.green == 16 && material->color.blue == 255)
			Lights.registerMaterial(vehicle, material, eLightState::Nightlight);

		else if (material->color.red == 0 && material->color.green == 17 && material->color.blue == 255)
			Lights.registerMaterial(vehicle, material, eLightState::Light);
		else if (material->color.red == 255 && material->color.green == 174 && material->color.blue == 0)
			Lights.registerMaterial(vehicle, material, eLightState::FogLightLeft);

		else if (material->color.red == 0 && material->color.green == 255 && material->color.blue == 199)
			Lights.registerMaterial(vehicle, material, eLightState::FogLightRight);

		return material;
	});

	VehicleMaterials::RegisterDummy([](CVehicle* vehicle, RwFrame* frame, std::string name, bool parent) {
		int start = -1;

		eLightState state = eLightState::None;

		if (name.rfind("fogl_", 0) == 0) {
			start = 4;
			state = (toupper(name[start]) == 'L') ? (eLightState::FogLightLeft) : (eLightState::FogLightRight);
		} else if (name.rfind("foglight_", 0) == 0) {
			start = 8;
			state = (toupper(name[start]) == 'L') ? (eLightState::FogLightLeft) : (eLightState::FogLightRight);
		} else if (name.rfind("revl_", 0) == 0){
			start = 4;
			state = eLightState::Reverselight;
		}
		else if (name.rfind("reversingl_", 0) == 0) {
			start = 10;
			state = eLightState::Reverselight;
		}
		else if (name.rfind("breakl_", 0) == 0) {
			start = 6;
			state = eLightState::Brakelight;
		}
		else if (name.rfind("breaklight_", 0) == 0) {
			start = 10;

			state = eLightState::Brakelight;
		}
		else if (name.rfind("light_day", 0) == 0) {
			start = 9;

			state = eLightState::Daylight;
		}
		else if (name.rfind("light_night", 0) == 0) {
			start = 10;

			state = eLightState::Nightlight;
		}
		else if (name.rfind("light_", 0) == 0 && name.rfind("light_em", 0) != 0) {
			start = 5;

			state = eLightState::Light;
		}
		else {
			return;
		}

		for (int _char = start; _char < (int)name.size(); _char++) {
			if (name[_char] != '_')
				continue;

			start = _char + 1;

			break;
		}

		int index = CPools::ms_pVehiclePool->GetIndex(vehicle);

		Lights.dummies[index][state].push_back(new VehicleDummy(frame, name, start, parent, eDummyPos::Backward, { 255, 255, 255, 128 }));
	});

	
	Events::processScriptsEvent += [this]() {
		CVehicle *pVeh = FindPlayerVehicle(-1, false);
		if (!pVeh) {
			return;
		}

		static size_t prev = 0;
		if (KeyPressed(VK_J)) {
			size_t now = CTimer::m_snTimeInMilliseconds;
			if (now - prev > 500.0f) {
				int model = pVeh->m_nModelIndex;
				if (materials[model][eLightState::FogLightLeft].size() == 0)
					return;

				VehData& data = vehData.Get(pVeh);
				data.m_bFogLightsOn = !data.m_bFogLightsOn;
				prev = now;
			}
		}
	};

	VehicleMaterials::RegisterRender([this](CVehicle* pVeh) {
		int model = pVeh->m_nModelIndex;

		if (Lights.materials[model].size() == 0)
			return;

		CAutomobile* automobile = reinterpret_cast<CAutomobile*>(pVeh);

		float vehicleAngle = (pVeh->GetHeading() * 180.0f) / 3.14f;

		float cameraAngle = (((CCamera*)0xB6F028)->GetHeading() * 180.0f) / 3.14f;

		Lights.renderLights(pVeh, eLightState::Light, vehicleAngle, cameraAngle);

		if (CClock::GetIsTimeInRange(20, 8))
			Lights.renderLights(pVeh, eLightState::Nightlight, vehicleAngle, cameraAngle);
		else
			Lights.renderLights(pVeh, eLightState::Daylight, vehicleAngle, cameraAngle);

		if (pVeh->m_nCurrentGear == 0 && pVeh->m_fMovingSpeed != 0)
			Lights.renderLights(pVeh, eLightState::Reverselight, vehicleAngle, cameraAngle);

		if (pVeh->m_fBreakPedal)
			Lights.renderLights(pVeh, eLightState::Brakelight, vehicleAngle, cameraAngle);
		
		if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_LEFT)) {
			if (pVeh->m_nVehicleFlags.bLightsOn && Lights.materials[model][eLightState::LightLeft].size() != 0) {
				Lights.renderLights(pVeh, eLightState::LightLeft, vehicleAngle, cameraAngle);
			}

			VehData& data = vehData.Get(pVeh);
			if (data.m_bFogLightsOn && Lights.materials[model][eLightState::FogLightLeft].size() != 0) {
				Lights.renderLights(pVeh, eLightState::FogLightLeft, vehicleAngle, cameraAngle);
			}
		}

		if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_RIGHT)) {
			if (pVeh->m_nVehicleFlags.bLightsOn && Lights.materials[model][eLightState::LightRight].size() != 0) {
				Lights.renderLights(pVeh, eLightState::LightRight, vehicleAngle, cameraAngle);
			}

			VehData& data = vehData.Get(pVeh);
			if (data.m_bFogLightsOn && Lights.materials[model][eLightState::FogLightRight].size() != 0) {
				Lights.renderLights(pVeh, eLightState::FogLightRight, vehicleAngle, cameraAngle);
			}
		}
	});
};

void LightsFeature::renderLights(CVehicle* vehicle, eLightState state, float vehicleAngle, float cameraAngle) {
	for (std::vector<VehicleMaterial*>::iterator material = materials[vehicle->m_nModelIndex][state].begin(); material != materials[vehicle->m_nModelIndex][state].end(); ++material)
		enableMaterial((*material));

	int id = (int)state * 100;

	int index = CPools::ms_pVehiclePool->GetIndex(vehicle);

	for (std::vector<VehicleDummy*>::iterator dummy = dummies[index][state].begin(); dummy != dummies[index][state].end(); ++dummy) {
		id++;

		enableDummy(id, (*dummy), vehicle, vehicleAngle, cameraAngle);
	}
};

void LightsFeature::registerMaterial(CVehicle* vehicle, RpMaterial* material, eLightState state) {
	material->color.red = material->color.green = material->color.blue = 255;
	materials[vehicle->m_nModelIndex][state].push_back(new VehicleMaterial(material));
};

void LightsFeature::enableMaterial(VehicleMaterial* material) {
	VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int*>(&material->Material->surfaceProps.ambient), *reinterpret_cast<unsigned int*>(&material->Material->surfaceProps.ambient)));

	material->Material->surfaceProps.ambient = 4.0;

	VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int*>(&material->Material->texture), *reinterpret_cast<unsigned int*>(&material->Material->texture)));

	material->Material->texture = material->TextureActive;
};

void LightsFeature::enableDummy(int id, VehicleDummy* dummy, CVehicle* vehicle, float vehicleAngle, float cameraAngle) {
	// if (dummy->Type < 2) {
	// 	float dummyAngle = vehicleAngle + dummy->Angle;

	// 	float differenceAngle = ((cameraAngle > dummyAngle) ? (cameraAngle - dummyAngle) : (dummyAngle - cameraAngle));

	// 	if (differenceAngle < 90.0f || differenceAngle > 270.0f)
	// 		return;
	// }
	Common::RegisterCorona(vehicle, dummy->Frame->modelling.pos, dummy->Color.red, dummy->Color.green, dummy->Color.blue, 115, (CPools::ms_pVehiclePool->GetIndex(vehicle) * 255) + id, dummy->Size);
	Common::RegisterShadow(vehicle, dummy->Frame->modelling.pos, dummy->Color.red, dummy->Color.green, dummy->Color.blue, dummy->Angle, dummy->CurrentAngle);
};
