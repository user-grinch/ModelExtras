#pragma once
#include <map>
#include <plugin.h>

#include "../../interface/ifeature.hpp"
#include "internals/dummy.h"
#include "internals/materials.h"

enum class eIndicatorState { 
	Left, 
	Right, 
	Both,
	None, 
};

class IndicatorFeature : public IFeature {
private:
	uint64_t delay;
	bool delayState;

	struct VehData {
		eIndicatorState indicatorState;
		VehData(CVehicle *) : indicatorState(eIndicatorState::None) {}
	};
	VehicleExtendedData<VehData> vehData;

	std::map<int, std::map<eIndicatorState, std::vector<VehicleMaterial*>>> materials;
	std::map<int, std::map<eIndicatorState, std::vector<VehicleDummy*>>> dummies;

	void registerMaterial(CVehicle* vehicle, RpMaterial* material, eIndicatorState state);
	void registerDummy(CVehicle* pVeh, RwFrame* pFrame, std::string name, bool parent, eIndicatorState state, eDummyPos rot);
	void enableMaterial(VehicleMaterial* material);
	void enableDummy(int id, VehicleDummy* dummy, CVehicle* vehicle, float vehicleAngle, float cameraAngle);
	
public:
	void Initialize();
};

extern IndicatorFeature Indicator;