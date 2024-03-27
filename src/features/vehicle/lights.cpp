#include "pch.h"
#include "lights.h"
#include <CClock.h>
#include "internals/common.h"
#include "defines.h"

LightsFeature Lights;

void DrawTailLightShadows(CVehicle *pVeh, bool leftSide) {
    CVector posn = reinterpret_cast<CVehicleModelInfo *>(CModelInfo::ms_modelInfoPtrs[pVeh->m_nModelIndex])->m_pVehicleStruct->m_avDummyPos[1];
    if (posn.x == 0.0f) posn.x = 0.15f;
    if (leftSide) posn.x *= -1.0f;
	Common::RegisterShadow(pVeh, posn, SHADOW_RED, SHADOW_GREEN, SHADOW_BLUE, 180.0f, 0.0f);
}

void LightsFeature::Initialize() {
	VehicleMaterials::Register([](CVehicle* vehicle, RpMaterial* material, bool* clearMats) {
		*clearMats = true;
		if (material->color.red == 255 && material->color.blue == 128) {
			// This section is kinda sus, remove it ltr?
			// if (material->color.green == 1)
			// 	Lights.registerMaterial(vehicle, material, eLightState::FrontLightLeft);

			// else if (material->color.green == 2)
			// 	Lights.registerMaterial(vehicle, material, eLightState::FrontLightRight);

			// else if (material->color.green == 3)
			// 	Lights.registerMaterial(vehicle, material, eLightState::TailLight);
			// // else if (material->color.green == 6)
			// // 	Lights.registerMaterial(vehicle, material, eLightState::FogLightLeft);

			// else if (material->color.green == 7) {
			// 	// Lights.registerMaterial(vehicle, material, eLightState::FogLightRight);
			// 	Lights.registerMaterial(vehicle, material, eLightState::Daylight);
			// }
			// else if (material->color.green == 8)
			// 	Lights.registerMaterial(vehicle, material, eLightState::Nightlight);

			// else if (material->color.green == 9)
			// 	Lights.registerMaterial(vehicle, material, eLightState::AllDayLight);
			// else
			// 	*clearMats = false;

		} else if (material->color.red == 255 && material->color.green == 173 && material->color.blue == 0)
			Lights.registerMaterial(vehicle, material, eLightState::Reverselight);

		else if (material->color.red == 0 && material->color.green == 255 && material->color.blue == 198)
			Lights.registerMaterial(vehicle, material, eLightState::Reverselight);

		else if (material->color.red == 184 && material->color.green == 255 && material->color.blue == 0)
			Lights.registerMaterial(vehicle, material, eLightState::Brakelight);

		else if (material->color.red == 255 && material->color.green == 59 && material->color.blue == 0)
			Lights.registerMaterial(vehicle, material, eLightState::Brakelight);

		else if (material->color.red == 0 && material->color.green == 16 && material->color.blue == 255)
			Lights.registerMaterial(vehicle, material, eLightState::Nightlight);

		else if (material->color.red == 0 && material->color.green == 17 && material->color.blue == 255)
			Lights.registerMaterial(vehicle, material, eLightState::AllDayLight);
		else if (material->color.red == 0 && material->color.green == 18 && material->color.blue == 255)
			Lights.registerMaterial(vehicle, material, eLightState::Daylight);

		else if (material->color.red == 255 && material->color.green == 174 && material->color.blue == 0)
			Lights.registerMaterial(vehicle, material, eLightState::FogLightLeft);
		else if (material->color.red == 0 && material->color.green == 255 && material->color.blue == 199)
			Lights.registerMaterial(vehicle, material, eLightState::FogLightRight);
			
		else if (material->color.red == 255 && material->color.green == 175 && material->color.blue == 0)
			Lights.registerMaterial(vehicle, material, eLightState::FrontLightLeft);
		else if (material->color.red == 0 && material->color.green == 255 && material->color.blue == 200) 
			Lights.registerMaterial(vehicle, material, eLightState::FrontLightRight);
		else if (material->color.red == 255 && material->color.green == 60 && material->color.blue == 0) 
			Lights.registerMaterial(vehicle, material, eLightState::TailLightRight);
		else if ((material->color.red == 255 && material->color.green == 60 && material->color.blue == 0) 
		|| (material->color.red == 185 && material->color.green == 255 && material->color.blue == 0))
			Lights.registerMaterial(vehicle, material, eLightState::TailLightLeft);
		else 
			*clearMats = false;

		return material;
	});

	VehicleMaterials::RegisterDummy([](CVehicle* vehicle, RwFrame* frame, std::string name, bool parent) {
		eLightState state = eLightState::None;
		eDummyPos rotation = eDummyPos::Rear;

		std::smatch match;
		if (std::regex_search(name, match, std::regex("^fog(light)?_([a-zA-Z])"))) {
			state = (toupper(match.str(2)[0]) == 'L') ? (eLightState::FogLightLeft) : (eLightState::FogLightRight);
		} else if (std::regex_search(name, std::regex("^revl_")) || std::regex_search(name, std::regex("^reversingl_"))) {
			state = eLightState::Reverselight;
		} else if (std::regex_search(name, std::regex("^breakl(?:ight)?_"))) {
			state = eLightState::Brakelight;
		} else if (std::regex_search(name, std::regex("^light_day"))) {
			state = eLightState::Daylight;
		} else if (std::regex_search(name, std::regex("^light_night"))) {
			state = eLightState::Nightlight;
		} else if (std::regex_search(name, std::regex("^light_(?!em)"))) {
			state = eLightState::AllDayLight;
		} else {
			return;
		}

		int index = CPools::ms_pVehiclePool->GetIndex(vehicle);
		Lights.dummies[index][state].push_back(new VehicleDummy(frame, name, parent, rotation, { 255, 255, 255, 128 }));
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
		float cameraAngle = (TheCamera.GetHeading() * 180.0f) / 3.14f;

		Lights.renderLights(pVeh, eLightState::AllDayLight, vehicleAngle, cameraAngle);

		if (CClock::GetIsTimeInRange(20, 8))
			Lights.renderLights(pVeh, eLightState::Nightlight, vehicleAngle, cameraAngle);
		else
			Lights.renderLights(pVeh, eLightState::Daylight, vehicleAngle, cameraAngle);

		if (pVeh->m_nCurrentGear == 0 && pVeh->m_fMovingSpeed != 0 && pVeh->m_pDriver)
			Lights.renderLights(pVeh, eLightState::Reverselight, vehicleAngle, cameraAngle);

		if (pVeh->m_fBreakPedal && pVeh->m_pDriver)
			Lights.renderLights(pVeh, eLightState::Brakelight, vehicleAngle, cameraAngle);
		
		if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_LEFT)) {
			if (pVeh->m_nVehicleFlags.bLightsOn && Lights.materials[model][eLightState::FrontLightLeft].size() != 0) {
				Lights.renderLights(pVeh, eLightState::FrontLightLeft, vehicleAngle, cameraAngle);
			}

			VehData& data = vehData.Get(pVeh);
			if (data.m_bFogLightsOn && Lights.materials[model][eLightState::FogLightLeft].size() != 0) {
				Lights.renderLights(pVeh, eLightState::FogLightLeft, vehicleAngle, cameraAngle);
				CVector posn = reinterpret_cast<CVehicleModelInfo *>(CModelInfo::ms_modelInfoPtrs[pVeh->m_nModelIndex])->m_pVehicleStruct->m_avDummyPos[0];
				if (posn.x == 0.0f) posn.x = 0.15f;
				Common::RegisterShadow(pVeh, posn, 225, 225, 225, 0.0f, 0.0f);
				posn.x *= -1.0f;
				Common::RegisterShadow(pVeh, posn, 225, 225, 225, 0.0f, 0.0f);
			}
		}

		if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_RIGHT)) {
			if (pVeh->m_nVehicleFlags.bLightsOn && Lights.materials[model][eLightState::FrontLightRight].size() != 0) {
				Lights.renderLights(pVeh, eLightState::FrontLightRight, vehicleAngle, cameraAngle);
			}

			VehData& data = vehData.Get(pVeh);
			if (data.m_bFogLightsOn && Lights.materials[model][eLightState::FogLightRight].size() != 0) {
				Lights.renderLights(pVeh, eLightState::FogLightRight, vehicleAngle, cameraAngle);
			}
		}

		if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_REAR_LEFT)) {
			if (pVeh->m_nVehicleFlags.bLightsOn && Lights.materials[model][eLightState::TailLightLeft].size() != 0) {
				Lights.renderLights(pVeh, eLightState::TailLightLeft, vehicleAngle, cameraAngle);
				CVector posn = reinterpret_cast<CVehicleModelInfo *>(CModelInfo::ms_modelInfoPtrs[pVeh->m_nModelIndex])->m_pVehicleStruct->m_avDummyPos[1];
				if (posn.x == 0.0f) posn.x = 0.15f;
				posn.x *= -1.0f;
				Common::RegisterShadow(pVeh, posn, 225, 0, 0, 180.0f, 0.0f);
			}
		}

		if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_REAR_RIGHT)) {
			if (pVeh->m_nVehicleFlags.bLightsOn && Lights.materials[model][eLightState::TailLightRight].size() != 0) {
				Lights.renderLights(pVeh, eLightState::TailLightRight, vehicleAngle, cameraAngle);
				CVector posn = reinterpret_cast<CVehicleModelInfo *>(CModelInfo::ms_modelInfoPtrs[pVeh->m_nModelIndex])->m_pVehicleStruct->m_avDummyPos[1];
				if (posn.x == 0.0f) posn.x = 0.15f;
				Common::RegisterShadow(pVeh, posn, 225, 0, 0, 180.0f, 0.0f);
			}
		}
	});
};

void LightsFeature::renderLights(CVehicle* vehicle, eLightState state, float vehicleAngle, float cameraAngle) {
	for (std::vector<VehicleMaterial*>::iterator material = materials[vehicle->m_nModelIndex][state].begin(); material != materials[vehicle->m_nModelIndex][state].end(); ++material)
		enableMaterial((*material));

	if (gConfig.ReadBoolean("FEATURES", "RenderShadows", false) || gConfig.ReadBoolean("FEATURES", "RenderCoronas", false)) {
		int id = (int)state * 100;
		int index = CPools::ms_pVehiclePool->GetIndex(vehicle);
		for (std::vector<VehicleDummy*>::iterator dummy = dummies[index][state].begin(); dummy != dummies[index][state].end(); ++dummy) {
			id++;

			enableDummy(id, (*dummy), vehicle, vehicleAngle, cameraAngle);
		}
	}
};

void LightsFeature::registerMaterial(CVehicle* vehicle, RpMaterial* material, eLightState state) {
	material->color.red = material->color.green = material->color.blue = 255;
	materials[vehicle->m_nModelIndex][state].push_back(new VehicleMaterial(material));
};

void LightsFeature::enableMaterial(VehicleMaterial* material) {
	VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int*>(&material->Material->surfaceProps.ambient), *reinterpret_cast<unsigned int*>(&material->Material->surfaceProps.ambient)));
	material->Material->surfaceProps.ambient = 4.0;

	material->Color.red = material->Color.green = material->Color.blue = 255;
	VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int*>(&material->Material->texture), *reinterpret_cast<unsigned int*>(&material->Material->texture)));
	material->Material->texture = material->TextureActive;
};

void LightsFeature::enableDummy(int id, VehicleDummy* dummy, CVehicle* vehicle, float vehicleAngle, float cameraAngle) {
	if (gConfig.ReadBoolean("FEATURES", "RenderShadows", false)) {
		Common::RegisterShadow(vehicle, dummy->Frame->modelling.pos, dummy->Color.red, dummy->Color.green, 
			dummy->Color.blue, dummy->Angle, dummy->CurrentAngle);
	}

	// if (gConfig.ReadBoolean("FEATURES", "RenderCoronas", false)) {
	// 	Common::RegisterCorona(vehicle, dummy->Frame->modelling.pos, dummy->Color.red, dummy->Color.green, 
	// 		dummy->Color.blue, CORONA_ALPHA, (CPools::ms_pVehiclePool->GetIndex(vehicle) * 255) + id, dummy->Size, 
	// 		dummy->CurrentAngle);
	// }
};
