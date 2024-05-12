#include "pch.h"
#include "lights.h"
#include <CClock.h>
#include "internals/common.h"
#include "defines.h"
#include <CShadows.h>

void Lights::Initialize() {

	// NOP CVehicle::DoHeadLightBeam
	patch::Nop(0x6A2E9F, 0x58);
	patch::Nop(0x6BDE73, 0x12);

	VehicleMaterials::Register([](CVehicle* vehicle, RpMaterial* material) {
		if (material->color.red == 255 && material->color.green == 173 && material->color.blue == 0)
			registerMaterial(vehicle, material, eLightState::Reverselight);

		else if (material->color.red == 0 && material->color.green == 255 && material->color.blue == 198)
			registerMaterial(vehicle, material, eLightState::Reverselight);

		else if (material->color.red == 184 && material->color.green == 255 && material->color.blue == 0)
			registerMaterial(vehicle, material, eLightState::Brakelight);

		else if (material->color.red == 255 && material->color.green == 59 && material->color.blue == 0)
			registerMaterial(vehicle, material, eLightState::Brakelight);

		else if (material->color.red == 0 && material->color.green == 16 && material->color.blue == 255)
			registerMaterial(vehicle, material, eLightState::Nightlight);

		else if (material->color.red == 0 && material->color.green == 17 && material->color.blue == 255)
			registerMaterial(vehicle, material, eLightState::AllDayLight);
		else if (material->color.red == 0 && material->color.green == 18 && material->color.blue == 255)
			registerMaterial(vehicle, material, eLightState::Daylight);

		else if (material->color.red == 255 && material->color.green == 174 && material->color.blue == 0)
			registerMaterial(vehicle, material, eLightState::FogLight);
		else if (material->color.red == 0 && material->color.green == 255 && material->color.blue == 199)
			registerMaterial(vehicle, material, eLightState::FogLight);
			
		else if (material->color.red == 255 && material->color.green == 175 && material->color.blue == 0)
			registerMaterial(vehicle, material, eLightState::FrontLightLeft);
		else if (material->color.red == 0 && material->color.green == 255 && material->color.blue == 200) 
			registerMaterial(vehicle, material, eLightState::FrontLightRight);
		else if (material->color.red == 255 && material->color.green == 60 && material->color.blue == 0) 
			registerMaterial(vehicle, material, eLightState::TailLightRight);
		else if (material->color.red == 185 && material->color.green == 255 && material->color.blue == 0)
			registerMaterial(vehicle, material, eLightState::TailLightLeft);

		return material;
	});

	VehicleMaterials::RegisterDummy([](CVehicle* vehicle, RwFrame* frame, std::string name, bool parent) {
		eLightState state = eLightState::None;
		eDummyPos rotation = eDummyPos::Rear;
		RwRGBA col{ 255, 255, 255, 128 };

		std::smatch match;
		if (std::regex_search(name, match, std::regex("^fog(light)?_([a-zA-Z])"))) {
			state = (toupper(match.str(2)[0]) == 'L') ? (eLightState::FogLight) : (eLightState::FogLight);
		} else if (std::regex_search(name, std::regex("^revl_")) || std::regex_search(name, std::regex("^reversingl_"))) {
			state = eLightState::Reverselight;
		} else if (std::regex_search(name, std::regex("^breakl(?:ight)?_"))) {
			state = eLightState::Brakelight;
			col = {250, 0, 0, 128};
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
		dummies[index][state].push_back(new VehicleDummy(frame, name, parent, rotation, col));
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
				if (materials[model][eLightState::FogLight].size() == 0)
					return;

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

		renderLights(pVeh, eLightState::AllDayLight, vehicleAngle, cameraAngle);

		if (CClock::GetIsTimeInRange(20, 8))
			renderLights(pVeh, eLightState::Nightlight, vehicleAngle, cameraAngle);
		else
			renderLights(pVeh, eLightState::Daylight, vehicleAngle, cameraAngle);

		if (pVeh->m_nCurrentGear == 0 && pVeh->m_fMovingSpeed != 0 && pVeh->m_pDriver)
			renderLights(pVeh, eLightState::Reverselight, vehicleAngle, cameraAngle);
		else {
			for (auto e: materials[pVeh->m_nModelIndex][eLightState::Reverselight]) {
				e->Color.red = e->Color.green = e->Color.blue = 10;
			}
		}

		if (pVeh->m_fBreakPedal && pVeh->m_pDriver)
			renderLights(pVeh, eLightState::Brakelight, vehicleAngle, cameraAngle);
		
		VehData& data = vehData.Get(pVeh);

		if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_LEFT) && 
		!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_RIGHT)) {
			if (data.m_bFogLightsOn && materials[model][eLightState::FogLight].size() != 0) {
				renderLights(pVeh, eLightState::FogLight, vehicleAngle, cameraAngle);
				CVector posn = reinterpret_cast<CVehicleModelInfo *>(CModelInfo::ms_modelInfoPtrs[pVeh->m_nModelIndex])->m_pVehicleStruct->m_avDummyPos[0];
				posn.x = 0.0f;
				posn.y += 3.35f;

				static RwTexture *pLightTex = nullptr;
				if (!pLightTex) {
					pLightTex = Util::LoadTextureFromFile(MOD_DATA_PATH_S(std::string("textures/foglight.png")), 120);
				}

				CVector center = pVeh->TransformFromObjectSpace(posn);
				float fAngle = pVeh->GetHeading() + (180.0f * 3.14f / 180.0f);
				CVector up = CVector(-sin(fAngle) * 3.0f, cos(fAngle) * 3.0f, 0.0f);
				CVector right = CVector(cos(fAngle) * 2.0f, sin(fAngle) * 2.0f, 0.0f);
				CShadows::StoreShadowToBeRendered(2, pLightTex, &center, up.x, up.y, right.x, right.y, 128, 225, 225, 225, 4.0f, false, 1.0f, 0, true);
			}
		}
		
		if (pVeh->m_nVehicleFlags.bLightsOn) {
			if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_LEFT) 
			&& materials[model][eLightState::FrontLightLeft].size() != 0) {
				renderLights(pVeh, eLightState::FrontLightLeft, vehicleAngle, cameraAngle);
			}

			if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_FRONT_RIGHT)
			&& materials[model][eLightState::FrontLightRight].size() != 0) {
				renderLights(pVeh, eLightState::FrontLightRight, vehicleAngle, cameraAngle);
			}

			bool showTailLights = false;
			if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_REAR_LEFT)
			&& materials[model][eLightState::TailLightLeft].size() != 0) {
				renderLights(pVeh, eLightState::TailLightLeft, vehicleAngle, cameraAngle);
				showTailLights = true;
			}

			if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_REAR_RIGHT)
			&& materials[model][eLightState::TailLightRight].size() != 0) {
				renderLights(pVeh, eLightState::TailLightRight, vehicleAngle, cameraAngle);
				showTailLights = true;
			}

			if (showTailLights) {
				if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_REAR_LEFT)) {
					CVector posn = reinterpret_cast<CVehicleModelInfo *>(CModelInfo::ms_modelInfoPtrs[pVeh->m_nModelIndex])->m_pVehicleStruct->m_avDummyPos[1];
					if (posn.x == 0.0f) posn.x = 0.15f;
					posn.x *= -1.0f;
					Common::RegisterShadow(pVeh, posn, TL_SHADOW_R, TL_SHADOW_G, TL_SHADOW_B, 180.0f, 0.0f);
				}

				if (!automobile->m_damageManager.GetLightStatus(eLights::LIGHT_REAR_RIGHT)) {
					CVector posn = reinterpret_cast<CVehicleModelInfo *>(CModelInfo::ms_modelInfoPtrs[pVeh->m_nModelIndex])->m_pVehicleStruct->m_avDummyPos[1];
					if (posn.x == 0.0f) posn.x = 0.15f;
					Common::RegisterShadow(pVeh, posn, TL_SHADOW_R, TL_SHADOW_G, TL_SHADOW_B, 180.0f, 0.0f);
				}
			}
		}
	});
};

void Lights::renderLights(CVehicle* pVeh, eLightState state, float vehicleAngle, float cameraAngle) {
	for (auto e: materials[pVeh->m_nModelIndex][state]) {
		enableMaterial(e);
	}

	if (gConfig.ReadBoolean("FEATURES", "RenderShadows", false)) {
		for (auto e: dummies[CPools::ms_pVehiclePool->GetIndex(pVeh)][state]) {
			Common::RegisterShadow(pVeh, e->Frame->modelling.pos, e->Color.red, e->Color.green, e->Color.blue, e->Angle, e->CurrentAngle);
		}
	}
};

void Lights::registerMaterial(CVehicle* vehicle, RpMaterial* material, eLightState state) {
	material->color.red = material->color.green = material->color.blue = 255;
	materials[vehicle->m_nModelIndex][state].push_back(new VehicleMaterial(material));
};

void Lights::enableMaterial(VehicleMaterial* material) {
	VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int*>(&material->Material->surfaceProps.ambient), *reinterpret_cast<unsigned int*>(&material->Material->surfaceProps.ambient)));
	material->Material->surfaceProps.ambient = 4.0;

	material->Color.red = material->Color.green = material->Color.blue = 255;
	VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int*>(&material->Material->texture), *reinterpret_cast<unsigned int*>(&material->Material->texture)));
	material->Material->texture = material->TextureActive;
};