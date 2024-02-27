#pragma once
#include <map>
#include <plugin.h>

#include "../../interface/ifeature.hpp"
#include "internals/dummy.h"
#include "internals/materials.h"

enum class eLightState { 
	None, 
	LightLeft, 
	LightRight, 
	TailLight, 
	Reverselight, 
	Brakelight, 
	Light, 
	Daylight, 
	Nightlight 
};

class LightsFeature : public IFeature {
public:
	void Initialize();

private:
	std::map<int, std::map<eLightState, std::vector<VehicleMaterial*>>> materials;
	std::map<int, std::map<eLightState, std::vector<VehicleDummy*>>> dummies;
	std::map<int, bool> states;

	void registerMaterial(CVehicle* vehicle, RpMaterial* material, eLightState state);
	void renderLights(CVehicle* vehicle, eLightState state, float vehicleAngle, float cameraAngle);
	void enableDummy(int id, VehicleDummy* dummy, CVehicle* vehicle, float vehicleAngle, float cameraAngle);
	void enableMaterial(VehicleMaterial* material);
};

extern LightsFeature Lights;
