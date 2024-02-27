#pragma once
#include <map>
#include <plugin.h>

#include "../../interface/ifeature.hpp"
#include "internals/dummy.h"
#include "internals/materials.h"

enum class eFogLightState { 
	Left, 
	Right,
	Both,
	None 
};

class FogLightsFeature {
public:
	void Initialize();

private:
	std::map<int, std::map<eFogLightState, std::vector<VehicleMaterial*>>> materials;
	std::map<int, std::map<eFogLightState, std::vector<VehicleDummy*>>> dummies;
	std::map<int, bool> states;

	void registerMaterial(CVehicle* vehicle, RpMaterial* material, eFogLightState state);
	void enableDummy(VehicleDummy* dummy, CVehicle* vehicle, float vehicleAngle, float cameraAngle);
	void enableMaterial(VehicleMaterial* material);
};

extern FogLightsFeature FogLights;