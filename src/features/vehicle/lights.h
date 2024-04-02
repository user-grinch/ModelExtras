#pragma once
#include <map>
#include <plugin.h>

#include "../../interface/ifeature.hpp"
#include "internals/dummy.h"
#include "internals/materials.h"

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

class LightsFeature : public IFeature {
public:
	void Initialize();

private:
	struct VehData {
        bool m_bFogLightsOn = false;

        VehData(CVehicle *pVeh) {}
        ~VehData() {}
    };

    VehicleExtendedData<VehData> vehData;
	std::map<int, std::map<eLightState, std::vector<VehicleMaterial*>>> materials;
	std::map<int, std::map<eLightState, std::vector<VehicleDummy*>>> dummies;

	void registerMaterial(CVehicle* vehicle, RpMaterial* material, eLightState state);
	void renderLights(CVehicle* vehicle, eLightState state, float vehicleAngle, float cameraAngle);
	void enableMaterial(VehicleMaterial* material);
};

extern LightsFeature Lights;
