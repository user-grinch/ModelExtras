#pragma once
#include <map>
#include <plugin.h>
#include "avs/dummy.h"
#include "avs/materials.h"
#include "enums/lighttype.h"

enum class eLightState
{
	None,
	FrontLightLeft,
	FrontLightRight,
	TailLightLeft,
	TailLightRight,
	Reverselight,
	Brakelight,
	AllDayLight,
	Daylight,
	Nightlight,
	FogLight,
	SideLightLeft,
	SideLightRight,
	STTLightLeft,
	STTLightRight,
	NABrakeLightLeft,
	NABrakeLightRight,
	SpotLight,

	IndicatorLeft,
	IndicatorRight,
	IndicatorBoth,
	IndicatorNone,
	Total,
};

enum eLightOverride
{
	NoOverride,
	ForceLightsOff,
	ForceLightsOn,
};

class Lights
{
private:
	struct VehData
	{
		bool m_bFogLightsOn = false;
		bool m_bLongLightsOn = false;
		eLightState m_nIndicatorState = eLightState::IndicatorNone;

		VehData(CVehicle *pVeh) {}
		~VehData() {}
	};

	/*
	 *	Note: Material data need to be model based
	 *		  Dummy data should be entity based
	 *		  Don't change it
	 */
	static inline std::map<int, std::map<eLightState, std::vector<VehicleMaterial *>>> m_Materials;
	static inline std::map<CVehicle *, std::map<eLightState, std::vector<VehicleDummy *>>> m_Dummies;
	static inline VehicleExtendedData<VehData> m_VehData;

	static void RegisterMaterial(CVehicle *vehicle, RpMaterial *material, eLightState state, CRGBA col, eDummyPos pos = eDummyPos::None);
	static void EnableMaterial(VehicleMaterial *material);
	static void EnableDummy(int id, VehicleDummy *dummy, CVehicle *vehicle);
	static void RenderLights(CVehicle *pControlVeh, CVehicle *pTowedVeh, eLightState state, bool shadows = true, std::string texture = "indicator", CVector2D sz = {1.0f, 1.0f}, CVector2D offset = {0.0f, 0.0f});

	// Helper functions
	static bool IsDummyAvail(CVehicle *pVeh, eLightState state);
	static bool IsMatAvail(CVehicle *pVeh, eLightState state);

public:
	static inline bool indicatorsDelay;

	static void Initialize();
	static bool IsIndicatorOn(CVehicle *pVeh)
	{
		return (pVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pVeh->m_nVehicleSubClass == VEHICLE_BIKE) && indicatorsDelay && m_VehData.Get(pVeh).m_nIndicatorState != eLightState::IndicatorNone;
	}
};