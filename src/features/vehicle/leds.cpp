#include "pch.h"
#include "leds.h"
#include <CClock.h>
#include "defines.h"
#include <CShadows.h>
#include <eVehicleClass.h>
#include <rwcore.h>
#include <rpworld.h>
#include "spotlights.h"
#include <CWeather.h>
#include <CCoronas.h>
#include "enums/vehdummy.h"
#include "datamgr.h"
#include "core/colors.h"
#include <CPointLights.h>

#include "roof.h"
#include "lights.h"

void DashboardLEDs::Initialize()
{
	bEnabled = true;

	ModelInfoMgr::RegisterMaterial([](CVehicle *pVeh, RpMaterial *pMat){
		if (!bEnabled) {
			return eMaterialType::UnknownMaterial;
		}

		CRGBA matCol = *reinterpret_cast<CRGBA *>(RpMaterialGetColor(pMat));
		matCol.a = 255;

		if (matCol == VEHCOL_LED_ENGINE_ON) {
			return eMaterialType::EngineOnLed;
		} else if (matCol == VEHCOL_LED_ENGINE_BROKEN) {
			return eMaterialType::EngineBrokenLed;
		} else if (matCol == VEHCOL_LED_FOG_LIGHT) {
			return eMaterialType::FogLightLed;
		} else if (matCol == VEHCOL_LED_HIGH_BEAM) {
			return eMaterialType::HighBeamLed;
		} else if (matCol == VEHCOL_LED_LOW_BEAM) {
			return eMaterialType::LowBeamLed;
		} else if (matCol == VEHCOL_LED_INDICATOR_LEFT) {
			return eMaterialType::IndicatorLeftLed;
		} else if (matCol == VEHCOL_LED_INDICATOR_RIGHT) {
			return eMaterialType::IndicatorRightLed;
		} else if (matCol == VEHCOL_LED_SIREN_LIGHTS) {
			return eMaterialType::SirenLed;
		} else if (matCol == VEHCOL_LED_BOOT_OPEN) {
			return eMaterialType::BootOpenLed;
		} else if (matCol == VEHCOL_LED_BONNET_OPEN) {
			return eMaterialType::BonnetOpenLed;
		} else if (matCol == VEHCOL_LED_DOOR_OPEN) {
			return eMaterialType::DoorOpenLed;
		} else if (matCol == VEHCOL_LED_ROOF_OPEN) {
			return eMaterialType::RoofOpenLed;
		}
		
		return eMaterialType::UnknownMaterial;
	});

	ModelInfoMgr::RegisterRender([](CVehicle *pControlVeh) {
		int model = pControlVeh->m_nModelIndex;

		if (pControlVeh->bEngineOn) {
			EnableLED(pControlVeh, eMaterialType::EngineOnLed);
		}

		if (pControlVeh->bEngineBroken) {
			EnableLED(pControlVeh, eMaterialType::EngineBrokenLed);
		}

		if (pControlVeh->bSirenOrAlarm) {
			EnableLED(pControlVeh, eMaterialType::SirenLed);
		}

		if (pControlVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE) {
			CAutomobile *pAutomobile = static_cast<CAutomobile*>(pControlVeh);
			bool isAnyDoorOpen = false;

			for (int i= eDoors::DOOR_FRONT_LEFT; i <= eDoors::DOOR_REAR_RIGHT; i++) {
				if (!pAutomobile->m_doors[i].IsClosed()) {
					isAnyDoorOpen = true;
					break;
				}
			}

			if (isAnyDoorOpen) {
				EnableLED(pControlVeh, eMaterialType::DoorOpenLed);
			}

			if (!pAutomobile->m_doors[eDoors::BONNET].IsClosed()) {
				EnableLED(pControlVeh, eMaterialType::BonnetOpenLed);
			}

			if (!pAutomobile->m_doors[eDoors::BOOT].IsClosed()) {
				EnableLED(pControlVeh, eMaterialType::BootOpenLed);
			}
		}

		auto data = Lights::GetVehicleData(pControlVeh);
		if (data.m_bFogLightsOn) {
			EnableLED(pControlVeh, eMaterialType::FogLightLed);
		}
		
		if (data.m_bLongLightsOn) {
			EnableLED(pControlVeh, eMaterialType::HighBeamLed);
		} else {
			EnableLED(pControlVeh, eMaterialType::LowBeamLed);
		}
		
		if (data.m_bFogLightsOn) {
			EnableLED(pControlVeh, eMaterialType::FogLightLed);
		}

		if (Lights::IsIndicatorOn(pControlVeh)) {
			if (data.m_nIndicatorState == eIndicatorState::LeftOn) {
				EnableLED(pControlVeh, eMaterialType::IndicatorLeftLed);
			} else {
				EnableLED(pControlVeh, eMaterialType::IndicatorRightLed);
			}
		}

		if (ConvertibleRoof::IsRoofOpen(pControlVeh)) {
			EnableLED(pControlVeh, eMaterialType::RoofOpenLed);
		}
	});
}

void DashboardLEDs::EnableLED(CVehicle *pVeh, eMaterialType type)
{
	auto& data = VehData.Get(pVeh);
	if (ModelInfoMgr::IsMaterialAvailable(pVeh, type) && data.bLEDStates[type]) {
		ModelInfoMgr::EnableMaterial(pVeh, type);
	}
}