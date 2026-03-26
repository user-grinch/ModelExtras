#pragma once
#include <map>
#include <plugin.h>
#include "modelinfomgr.h"
#include "enums/materialtype.h"

struct VehLEDData {
	bool bLEDStates[eMaterialType::TotalMaterial];

	VehLEDData(CVehicle *pVeh) {
		std::fill(std::begin(bLEDStates), std::end(bLEDStates), true);
	}
	~VehLEDData() {}
};

class DashboardLEDs
{
private:
	static inline bool bEnabled = false;
	static inline VehicleExtendedData<VehLEDData> VehData;

	static void EnableLED(CVehicle *pVeh, eMaterialType state);
	
public:
	static void Initialize();
};