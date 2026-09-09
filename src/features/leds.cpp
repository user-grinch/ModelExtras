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
#include "utils/datamgr.h"
#include "core/colors.h"
#include <CPointLights.h>

#include "roof.h"
#include "lights/lights.h"

void DashboardLEDs::ReloadConfig()
{
	CBaseFeature::ReloadConfig();
	bEnabled = m_bActive;
}

void DashboardLEDs::Init()
{
	ReloadConfig();

	ModelInfoMgr::RegisterMaterial([](CVehicle *pVeh, RpMaterial *pMat){
		if (!bEnabled) {
			return eMaterialType::UnknownMaterial;
		}

		CRGBA matCol = *reinterpret_cast<CRGBA *>(RpMaterialGetColor(pMat));
		if (matCol.r == 255 && matCol.g == 200) {
			switch (matCol.b) {
			case 100: return eMaterialType::EngineOnLed;
			case 101: return eMaterialType::EngineBrokenLed;
			case 102: return eMaterialType::FogLightLed;
			case 103: return eMaterialType::HighBeamLed;
			case 104: return eMaterialType::LowBeamLed;
			case 105: return eMaterialType::IndicatorLeftLed;
			case 106: return eMaterialType::IndicatorRightLed;
			case 107: return eMaterialType::SirenLed;
			case 108: return eMaterialType::BootOpenLed;
			case 109: return eMaterialType::BonnetOpenLed;
			case 110: return eMaterialType::DoorOpenLed;
			case 111: return eMaterialType::RoofOpenLed;
			default: break;
			}
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

		const auto& data = Lights::GetVehicleData(pControlVeh);
		static bool foglightTiedtoHeadlight = gConfig.ReadBoolean("TWEAKS", "FoglightTiedToHeadlight", true);
		bool isHeadlightsActive = (pControlVeh->bLightsOn || CarUtil::IsLightsForcedOn(pControlVeh) || (Util::IsNightTime() && !Util::IsEngineOff(pControlVeh))) && !CarUtil::IsLightsForcedOff(pControlVeh);
		bool isFoggy = Util::IsFoggy();
		bool shouldShowFog = isFoggy || !foglightTiedtoHeadlight || isHeadlightsActive;
		bool isFogLightOn = (data.bFogLightsOn || isFoggy) && !CarUtil::IsLightsForcedOff(pControlVeh);
		if (isFogLightOn && shouldShowFog) {
			EnableLED(pControlVeh, eMaterialType::FogLightLed);
		}
		bool headlightsOn = (pControlVeh->bLightsOn || CarUtil::IsLightsForcedOn(pControlVeh) || (Util::IsNightTime() && !Util::IsEngineOff(pControlVeh))) && !CarUtil::IsLightsForcedOff(pControlVeh);
		if (headlightsOn) {
			if (data.bLongLightsOn) {
				EnableLED(pControlVeh, eMaterialType::HighBeamLed);
			} else {
				EnableLED(pControlVeh, eMaterialType::LowBeamLed);
			}
		}

		if (Lights::IsIndicatorOn(pControlVeh)) {
			if (data.nIndicatorState == eIndicatorState::LeftOn || data.nIndicatorState == eIndicatorState::BothOn) {
				EnableLED(pControlVeh, eMaterialType::IndicatorLeftLed);
			}
			if (data.nIndicatorState == eIndicatorState::RightOn || data.nIndicatorState == eIndicatorState::BothOn) {
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
	ModelInfoMgr::EnableMaterial(pVeh, type);
}