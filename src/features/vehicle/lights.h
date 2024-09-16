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

	// Indicator state 
	IndicatorLeft, 
	IndicatorRight, 
	IndicatorBoth,
	IndicatorNone, 
};

class Lights {
private: 
	static void InitIndicators();

	struct VehData {
        bool m_bFogLightsOn = false;
		eLightState indicatorState = eLightState::IndicatorNone;

        VehData(CVehicle *pVeh) {}
        ~VehData() {}
    };

    static inline VehicleExtendedData<VehData> vehData;
	static inline std::map<int, std::map<eLightState, std::vector<VehicleMaterial*>>> materials;
	static inline std::map<int, std::map<eLightState, std::vector<VehicleDummy*>>> dummies;

	static void RegisterMaterial(CVehicle* vehicle, RpMaterial* material, eLightState state);
	static void RenderLights(CVehicle* vehicle, eLightState state, float vehicleAngle, float cameraAngle);
	static void EnableMaterial(VehicleMaterial* material);
	static void EnableDummy(int id, VehicleDummy* dummy, CVehicle* vehicle);

public:
	static void Initialize();
};