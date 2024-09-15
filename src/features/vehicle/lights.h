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
};

class Lights {
public:
	static void Initialize();

private:
	struct VehData {
        bool m_bFogLightsOn = false;

        VehData(CVehicle *pVeh) {}
        ~VehData() {}
    };

    static inline VehicleExtendedData<VehData> vehData;
	static inline std::map<int, std::map<eLightState, std::vector<VehicleMaterial*>>> materials;
	static inline std::map<int, std::map<eLightState, std::vector<VehicleDummy*>>> dummies;

	static void RegisterMaterial(CVehicle* vehicle, RpMaterial* material, eLightState state);
	static void RenderLights(CVehicle* vehicle, eLightState state, float vehicleAngle, float cameraAngle);
	static void EnableMaterial(VehicleMaterial* material);
};