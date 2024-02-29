#pragma once
#include <map>
#include <plugin.h>

#include "../../interface/ifeature.hpp"
#include "internals/dummy.h"
#include "internals/materials.h"

enum class eReverseLightstate { 
	Left, 
	Right,
	Both,
	None 
};

class ReverseLightsFeature {
public:
	void Initialize();

private:
	std::map<int, std::map<eReverseLightstate, std::vector<VehicleMaterial*>>> materials;
	std::map<int, std::map<eReverseLightstate, std::vector<VehicleDummy*>>> dummies;
	std::map<int, bool> states;

	void registerMaterial(CVehicle* vehicle, RpMaterial* material, eReverseLightstate state);
	void enableDummy(VehicleDummy* dummy, CVehicle* vehicle, float vehicleAngle, float cameraAngle);
	void enableMaterial(VehicleMaterial* material);
};

extern ReverseLightsFeature ReverseLights;