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

enum class eIndicatorPos { 
	Forward, 
	Backward, 
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

	std::map<int, std::map<eIndicatorState, std::vector<RpMaterial*>>> materials;
	std::map<int, std::map<eIndicatorState, std::vector<VehicleDummy*>>> dummies;

	void registerMaterial(CVehicle* vehicle, RpMaterial* &material, eIndicatorState state);
	void enableMaterial(RpMaterial* material);
	void enableDummy(int id, VehicleDummy* dummy, CVehicle* vehicle, float vehicleAngle, float cameraAngle);
	
public:
	void Initialize();
	void enableShadow(CVehicle* pVeh, unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha, CVector position, float angle, float currentAngle);

};

extern IndicatorFeature Indicator;