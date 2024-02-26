#pragma once
#include <map>

#include "plugin.h"
#include "internals/dummy.h"
#include "internals/materials.h"

enum class eLightState { 
	None = 0, 
	LightLeft, 
	LightRight, 
	TailLight, 
	Reverselight, 
	Brakelight, 
	Light, 
	Daylight, 
	Nightlight 
};

class VehicleLights {
public:
	static void RegisterEvents();

private:
	static inline std::map<int, std::map<eLightState, std::vector<VehicleMaterial*>>> materials;
	static inline std::map<int, std::map<eLightState, std::vector<VehicleDummy*>>> dummies;
	static inline std::map<int, bool> states;

	static void registerMaterial(CVehicle* vehicle, RpMaterial* material, eLightState state);

	static void renderLights(CVehicle* vehicle, eLightState state, float vehicleAngle, float cameraAngle);

	static void enableDummy(int id, VehicleDummy* dummy, CVehicle* vehicle, float vehicleAngle, float cameraAngle);

	static void enableMaterial(VehicleMaterial* material);
};
