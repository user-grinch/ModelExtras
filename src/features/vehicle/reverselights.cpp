#include "pch.h"
#include "reverselights.h"
#include <CCoronas.h>

ReverseLightsFeature ReverseLights;

void ReverseLightsFeature::Initialize() {
	VehicleMaterials::Register((VehicleMaterialFunction)[](CVehicle* vehicle, RpMaterial* material) {
		if (material->color.red == 255 && material->color.blue == 128) {
			if (std::string(material->texture->name).rfind("light", 0) == 0) {
				if (material->color.green == 6)
					ReverseLights.registerMaterial(vehicle, material, eReverseLightstate::Left);

				else if (material->color.green == 7)
					ReverseLights.registerMaterial(vehicle, material, eReverseLightstate::Right);
			}
		}
		else if (material->color.red == 255 && material->color.green == 215 && material->color.blue == 0)
			ReverseLights.registerMaterial(vehicle, material, eReverseLightstate::Left);

		else if (material->color.red == 0 && material->color.green == 255 && material->color.blue == 228)
			ReverseLights.registerMaterial(vehicle, material, eReverseLightstate::Right);
		
		return material;
	});

	VehicleMaterials::RegisterDummy((VehicleDummyFunction)[](CVehicle* vehicle, RwFrame* frame, std::string name, bool parent) {
		int start = -1;

		if (name.rfind("revl_", 0) == 0)
			start = 5;
			
		else if (name.rfind("reversingl_", 0) == 0)
			start = 11;

		if (start == -1)
			return;

		int model = vehicle->m_nModelIndex;

		eReverseLightstate state = (toupper(name[start]) == 'L') ? (eReverseLightstate::Left) : (eReverseLightstate::Right);

		for (int _char = start; _char < (int)name.size(); _char++) {
			if (name[_char] != '_')
				continue;

			start = _char + 1;

			break;
		}

		ReverseLights.dummies[model][state].push_back(new VehicleDummy(frame, name, start, parent, eDummyPos::Backward, { 255, 255, 255, 128 }));
	});

	Events::vehicleRenderEvent += [this](CVehicle *pVeh) {

		static size_t prev = 0;
		// if (KeyPressed(VK_P)) {
			// size_t now = CTimer::m_snTimeInMilliseconds;
			// if (now - prev > 500.0f) {

				int model = pVeh->m_nModelIndex;

				if (materials[model].size() == 0)
					return;

				int index = CPools::ms_pVehiclePool->GetIndex(pVeh);
				if (states.contains(index))
					states.erase(index);
				else
					states[index] = true;
			// 	prev = now;
			// }
		// }
	};

	VehicleMaterials::RegisterRender((VehicleMaterialRender)[](CVehicle* vehicle, int index) {
		int model = vehicle->m_nModelIndex;

		if (ReverseLights.materials[model].size() == 0)
			return;

		if (vehicle->m_nVehicleFlags.bSirenOrAlarm == false) {
			int index = CPools::ms_pVehiclePool->GetIndex(vehicle);

			if (!ReverseLights.states.contains(index))
				return;
		}

		CAutomobile* automobile = reinterpret_cast<CAutomobile*>(vehicle);

		float vehicleAngle = (vehicle->GetHeading() * 180.0f) / 3.14f;

		float cameraAngle = (((CCamera*)0xB6F028)->GetHeading() * 180.0f) / 3.14f;

		if (ReverseLights.materials[model][eReverseLightstate::Left].size() != 0) {
			if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_LEFT)) {
				for (std::vector<VehicleMaterial*>::iterator material = ReverseLights.materials[model][eReverseLightstate::Left].begin(); material != ReverseLights.materials[model][eReverseLightstate::Left].end(); ++material)
					ReverseLights.enableMaterial((*material));

				for (std::vector<VehicleDummy*>::iterator dummy = ReverseLights.dummies[model][eReverseLightstate::Left].begin(); dummy != ReverseLights.dummies[model][eReverseLightstate::Left].end(); ++dummy)
					ReverseLights.enableDummy((*dummy), vehicle, vehicleAngle, cameraAngle);
			}
		}

		if (ReverseLights.materials[model][eReverseLightstate::Right].size() != 0) {
			if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_RIGHT)) {
				for (std::vector<VehicleMaterial*>::iterator material = ReverseLights.materials[model][eReverseLightstate::Right].begin(); material != ReverseLights.materials[model][eReverseLightstate::Right].end(); ++material)
					ReverseLights.enableMaterial((*material));

				for (std::vector<VehicleDummy*>::iterator dummy = ReverseLights.dummies[model][eReverseLightstate::Right].begin(); dummy != ReverseLights.dummies[model][eReverseLightstate::Right].end(); ++dummy)
					ReverseLights.enableDummy((*dummy), vehicle, vehicleAngle, cameraAngle);
			}
		}
	});
};

void ReverseLightsFeature::registerMaterial(CVehicle* vehicle, RpMaterial* material, eReverseLightstate state) {
	material->color.red = material->color.green = material->color.blue = 255;
	materials[vehicle->m_nModelIndex][state].push_back(new VehicleMaterial(material));
};

void ReverseLightsFeature::enableMaterial(VehicleMaterial* material) {
	VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int*>(&material->Material->surfaceProps.ambient), *reinterpret_cast<unsigned int*>(&material->Material->surfaceProps.ambient)));

	material->Material->surfaceProps.ambient = 4.0;

	VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int*>(&material->Material->texture), *reinterpret_cast<unsigned int*>(&material->Material->texture)));

	material->Material->texture = material->TextureActive;
};

void ReverseLightsFeature::enableDummy(VehicleDummy* dummy, CVehicle* vehicle, float vehicleAngle, float cameraAngle) {
	// if (dummy->Type < 2) {
	// 	float dummyAngle = vehicleAngle + dummy->Angle;

	// 	float differenceAngle = ((cameraAngle > dummyAngle) ? (cameraAngle - dummyAngle) : (dummyAngle - cameraAngle));

	// 	if (differenceAngle < 90.0f || differenceAngle > 270.0f)
	// 		return;
	// }

	CCoronas::RegisterCorona(reinterpret_cast<unsigned int>(vehicle) + 40 + reinterpret_cast<unsigned int>(dummy), vehicle, dummy->Color.red, dummy->Color.green, dummy->Color.blue, dummy->Color.alpha, dummy->Position,
		dummy->Size, 300.0f, eCoronaType::CORONATYPE_HEADLIGHT, eCoronaFlareType::FLARETYPE_NONE, false, false, 0, 0.0f, false, 0.5f, 0, 50.0f, false, true);
};