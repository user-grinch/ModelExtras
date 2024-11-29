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
	static void InitIndicators();

	struct VehData {
        bool m_bFogLightsOn = false;
		eLightState m_nIndicatorState = eLightState::IndicatorNone;

        VehData(CVehicle *pVeh) {}
        ~VehData() {}
    };

	static inline std::map<int, std::map<eLightState, std::vector<VehicleMaterial*>>> m_Materials;
	static inline std::map<int, std::map<eLightState, std::vector<VehicleDummy*>>> m_Dummies;
    static inline VehicleExtendedData<VehData> m_VehData;

	static void RegisterMaterial(CVehicle* vehicle, RpMaterial* material, eLightState state, eDummyPos pos = eDummyPos::None);
	static void RenderLights(CVehicle* vehicle, eLightState state, float vehicleAngle, float cameraAngle, bool skipShadows = true);
	static void EnableMaterial(VehicleMaterial* material);
	static void EnableDummy(int id, VehicleDummy* dummy, CVehicle* vehicle);

public:
	static void Initialize();
};