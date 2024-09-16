#include "pch.h"
#include "lights.h"
#include <CClock.h>
#include "avs/common.h"
#include "defines.h"
#include <CShadows.h>

void Lights::Initialize() {

	// NOP CVehicle::DoHeadLightBeam
	patch::Nop(0x6A2E9F, 0x58);
	patch::Nop(0x6BDE73, 0x12);

	VehicleMaterials::Register([](CVehicle* vehicle, RpMaterial* material) {
		if (material->color.red == 255 && material->color.green == 173 && material->color.blue == 0)
			RegisterMaterial(vehicle, material, eLightState::Reverselight);

		else if (material->color.red == 0 && material->color.green == 255 && material->color.blue == 198)
			RegisterMaterial(vehicle, material, eLightState::Reverselight);

		else if (material->color.red == 184 && material->color.green == 255 && material->color.blue == 0)
			RegisterMaterial(vehicle, material, eLightState::Brakelight);

		else if (material->color.red == 255 && material->color.green == 59 && material->color.blue == 0)
			RegisterMaterial(vehicle, material, eLightState::Brakelight);

		else if (material->color.red == 0 && material->color.green == 16 && material->color.blue == 255)
			RegisterMaterial(vehicle, material, eLightState::Nightlight);

		else if (material->color.red == 0 && material->color.green == 17 && material->color.blue == 255)
			RegisterMaterial(vehicle, material, eLightState::AllDayLight);
		else if (material->color.red == 0 && material->color.green == 18 && material->color.blue == 255)
			RegisterMaterial(vehicle, material, eLightState::Daylight);

		else if (material->color.red == 255 && material->color.green == 174 && material->color.blue == 0)
			RegisterMaterial(vehicle, material, eLightState::FogLight);
		else if (material->color.red == 0 && material->color.green == 255 && material->color.blue == 199)
			RegisterMaterial(vehicle, material, eLightState::FogLight);
			
		else if (material->color.red == 255 && material->color.green == 175 && material->color.blue == 0)
			RegisterMaterial(vehicle, material, eLightState::FrontLightLeft);
		else if (material->color.red == 0 && material->color.green == 255 && material->color.blue == 200) 
			RegisterMaterial(vehicle, material, eLightState::FrontLightRight);
		else if (material->color.red == 255 && material->color.green == 60 && material->color.blue == 0) 
			RegisterMaterial(vehicle, material, eLightState::TailLightRight);
		else if (material->color.red == 185 && material->color.green == 255 && material->color.blue == 0)
			RegisterMaterial(vehicle, material, eLightState::TailLightLeft);

		return material;
	});

	VehicleMaterials::RegisterDummy([](CVehicle* vehicle, RwFrame* frame, std::string name, bool parent) {
		eLightState state = eLightState::None;
		eDummyPos rotation = eDummyPos::Rear;
		RwRGBA col{ 255, 255, 255, 128 };

		std::smatch match;
		if (std::regex_search(name, match, std::regex("^fog(light)?_([a-zA-Z])"))) {
			state = (toupper(match.str(2)[0]) == 'L') ? (eLightState::FogLight) : (eLightState::FogLight);
		} else if (std::regex_search(name, std::regex("^(revl_|reversingl_|revr_|reversingr_|reversinglight_l|reversinglight_r)"))) {
			state = eLightState::Reverselight;
			col = {255, 255, 255, 240};
		} else if (std::regex_search(name, std::regex("^light_day"))) {
			state = eLightState::Daylight;
		} else if (std::regex_search(name, std::regex("^light_night"))) {
			state = eLightState::Nightlight;
		} else if (std::regex_search(name, std::regex("^light_(?!em)"))) {
			state = eLightState::AllDayLight;
		} else {
			return;
		}

		dummies[vehicle->m_nModelIndex][state].push_back(new VehicleDummy(frame, name, parent, rotation, col));
	});
	
	Events::processScriptsEvent += []() {
		CVehicle *pVeh = FindPlayerVehicle(-1, false);
		if (!pVeh) {
			return;
		}

		static size_t prev = 0;
		if (KeyPressed(VK_J)) {
			size_t now = CTimer::m_snTimeInMilliseconds;
			if (now - prev > 500.0f) {
				int model = pVeh->m_nModelIndex;

				VehData& data = vehData.Get(pVeh);
				data.m_bFogLightsOn = !data.m_bFogLightsOn;
				prev = now;
			}
		}
	};

	VehicleMaterials::RegisterRender([](CVehicle* pVeh) {
		int model = pVeh->m_nModelIndex;

		if (pVeh->m_fHealth == 0 || materials[model].size() == 0)
			return;

		CAutomobile* automobile = reinterpret_cast<CAutomobile*>(pVeh);

		float vehicleAngle = (pVeh->GetHeading() * 180.0f) / 3.14f;
		float cameraAngle = (TheCamera.GetHeading() * 180.0f) / 3.14f;

		RenderLights(pVeh, eLightState::AllDayLight, vehicleAngle, cameraAngle);

		if (CClock::GetIsTimeInRange(20, 8)) {
			RenderLights(pVeh, eLightState::Nightlight, vehicleAngle, cameraAngle);
		}
		else {
			RenderLights(pVeh, eLightState::Daylight, vehicleAngle, cameraAngle);
		}

		if (pVeh->m_nCurrentGear == 0 && pVeh->m_fMovingSpeed != 0 && pVeh->m_pDriver) {
			RenderLights(pVeh, eLightState::Reverselight, vehicleAngle, cameraAngle);
		}
		
		VehData& data = vehData.Get(pVeh);
		if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_LEFT) && 
		!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_RIGHT)) {
			if (data.m_bFogLightsOn) {
				RenderLights(pVeh, eLightState::FogLight, vehicleAngle, cameraAngle);
				static RwTexture *pLightTex = Util::LoadTextureFromFile(MOD_DATA_PATH_S(std::string("textures/foglight.png")), 120);
				CVector posn = reinterpret_cast<CVehicleModelInfo *>(CModelInfo__ms_modelInfoPtrs[pVeh->m_nModelIndex])->m_pVehicleStruct->m_avDummyPos[0];
				posn.x = 0.0f;
				posn.y += 2.0f;
				Common::RegisterShadow(pVeh, posn, 225, 225, 225, 128, 0.0f, 0.0f, "", 2.5f, 0.0f, pLightTex);
			}
		}
		
		if (pVeh->m_nVehicleFlags.bLightsOn) {
			if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_LEFT) 
			&& materials[model][eLightState::FrontLightLeft].size() != 0) {
				RenderLights(pVeh, eLightState::FrontLightLeft, vehicleAngle, cameraAngle);
			}

			if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_RIGHT)
			&& materials[model][eLightState::FrontLightRight].size() != 0) {
				RenderLights(pVeh, eLightState::FrontLightRight, vehicleAngle, cameraAngle);
			}

			bool showTailLights = false;
			if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_REAR_LEFT)) {
				RenderLights(pVeh, eLightState::TailLightLeft, vehicleAngle, cameraAngle);
				showTailLights = true;
			}

			if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_REAR_RIGHT)) {
				RenderLights(pVeh, eLightState::TailLightRight, vehicleAngle, cameraAngle);
				showTailLights = true;
			}

			if (pVeh->m_fBreakPedal && pVeh->m_pDriver) {
				RenderLights(pVeh, eLightState::Brakelight, vehicleAngle, cameraAngle);
				static RwTexture *pLightTex = Util::LoadTextureFromFile(MOD_DATA_PATH_S(std::string("textures/taillight.png")), 80);
				CVector posn = reinterpret_cast<CVehicleModelInfo *>(CModelInfo__ms_modelInfoPtrs[pVeh->m_nModelIndex])->m_pVehicleStruct->m_avDummyPos[1];
				posn.x = 0.0f;
				Common::RegisterShadow(pVeh, posn, TL_SHADOW_R, TL_SHADOW_G, TL_SHADOW_B, 128, 180.0f, 0.0f, "", 2.0f, 0.0f, pLightTex);
			}

			if (showTailLights) {
				static RwTexture *pLightTex = Util::LoadTextureFromFile(MOD_DATA_PATH_S(std::string("textures/taillight.png")), 40);
				CVector posn = reinterpret_cast<CVehicleModelInfo *>(CModelInfo__ms_modelInfoPtrs[pVeh->m_nModelIndex])->m_pVehicleStruct->m_avDummyPos[1];
				posn.x = 0.0f;
				Common::RegisterShadow(pVeh, posn, TL_SHADOW_R, TL_SHADOW_G, TL_SHADOW_B, 128, 180.0f, 0.0f, "", 2.0f, 0.0f, pLightTex);
			}
		}
	});
};

void Lights::RenderLights(CVehicle* pVeh, eLightState state, float vehicleAngle, float cameraAngle) {
	for (auto &e: materials[pVeh->m_nModelIndex][state]) {
		EnableMaterial(e);
	}

	int id = 0;
	for (auto e: dummies[pVeh->m_nModelIndex][state]) {
		EnableDummy((int)pVeh + (int)state + id++, e, pVeh);
		Common::RegisterShadow(pVeh, e->ShdwPosition, e->Color.red, e->Color.green, e->Color.blue, 128,  e->Angle, e->CurrentAngle, "indicator");
	}
};

void Lights::RegisterMaterial(CVehicle* vehicle, RpMaterial* material, eLightState state) {
	material->color.red = material->color.green = material->color.blue = 255;
	materials[vehicle->m_nModelIndex][state].push_back(new VehicleMaterial(material));
};

void Lights::EnableDummy(int id, VehicleDummy* dummy, CVehicle* vehicle) {
	if (gConfig.ReadBoolean("FEATURES", "RenderCoronas", false)) {
		float cameraAngle = (TheCamera.GetHeading() * 180.0f) / 3.14f;
		Common::RegisterCoronaWithAngle(vehicle, dummy->Position, dummy->Color.red, dummy->Color.green, dummy->Color.blue, 
			CORONA_A, id, cameraAngle, dummy->CurrentAngle, 1.0f,  0.5f);
	}
};

void Lights::EnableMaterial(VehicleMaterial* material) {
	VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int*>(&material->Material->surfaceProps.ambient), *reinterpret_cast<unsigned int*>(&material->Material->surfaceProps.ambient)));
	material->Material->surfaceProps.ambient = 4.0;
	VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int*>(&material->Material->texture), *reinterpret_cast<unsigned int*>(&material->Material->texture)));
	material->Material->texture = material->TextureActive;
};