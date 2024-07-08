#pragma once
#include <map>
#include <plugin.h>
#include "avs/dummy.h"
#include "avs/materials.h"

enum class eIndicatorState { 
	Left, 
	Right, 
	Both,
	None, 
};

class Indicator {
private:
	static inline uint64_t delay;
	static inline bool delayState;

	struct VehData {
		eIndicatorState indicatorState;
		VehData(CVehicle *) : indicatorState(eIndicatorState::None) {}
	};
	static inline VehicleExtendedData<VehData> vehData;

	static inline std::map<int, std::map<eIndicatorState, std::vector<VehicleMaterial*>>> materials;
	static inline std::map<int, std::map<eIndicatorState, std::vector<VehicleDummy*>>> dummies;

	static void registerMaterial(CVehicle* vehicle, RpMaterial* material, eIndicatorState state);
	static void registerDummy(CVehicle* pVeh, RwFrame* pFrame, std::string name, bool parent, eIndicatorState state, eDummyPos rot);
	static void enableMaterial(VehicleMaterial* material);
	static void enableDummy(int id, VehicleDummy* dummy, CVehicle* vehicle, float vehicleAngle, float cameraAngle);
	
public:
	static void Initialize();
};