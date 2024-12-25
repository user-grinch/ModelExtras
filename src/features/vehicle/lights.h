#pragma once
#include <map>
#include <plugin.h>
#include "avs/dummy.h"
#include "avs/materials.h"

enum class eLightState {
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

	IndicatorLeft,
	IndicatorRight,
	IndicatorBoth,
	IndicatorNone,
	Total,
};

class Lights {
private:
	struct VehData {
		bool m_bFogLightsOn = false;
		bool m_bLongLightsOn = false;
		eLightState m_nIndicatorState = eLightState::IndicatorNone;

		VehData(CVehicle* pVeh) {}
		~VehData() {}
	};

	/*
	*	Note: Material data need to be model based
	*		  Dummy data should be entity based
	*		  Don't change it
	*/
	static inline std::map<int, std::map<eLightState, std::vector<VehicleMaterial*>>> m_Materials;
	static inline std::map<CVehicle*, std::map<eLightState, std::vector<VehicleDummy*>>> m_Dummies;
	static inline VehicleExtendedData<VehData> m_VehData;

	static void RegisterMaterial(CVehicle* vehicle, RpMaterial* material, eLightState state, eDummyPos pos = eDummyPos::None);
	static void RenderLights(CVehicle* vehicle, eLightState state, float vehicleAngle, float cameraAngle, bool shadows = true, std::string texture = "indicator", float sz = 1.0f);
	static void EnableMaterial(VehicleMaterial* material);
	static void EnableDummy(int id, VehicleDummy* dummy, CVehicle* vehicle);

public:
	static void Initialize();
};