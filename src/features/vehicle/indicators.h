#pragma once
#include <map>
#include <plugin.h>
#include "avs/dummy.h"
#include "avs/materials.h"
#include "lights.h"

class Indicator {
private:
	struct VehData {
		eLightState indicatorState;
		VehData(CVehicle *) : indicatorState(eLightState::IndicatorNone) {}
	};
	
	static inline uint64_t delay;
	static inline bool delayState;
	static inline VehicleExtendedData<VehData> vehData;
	static inline std::map<int, std::map<eLightState, std::vector<VehicleMaterial*>>> materials;
	static inline std::map<int, std::map<eLightState, std::vector<VehicleDummy*>>> dummies;
public:
	static void Initialize();
};