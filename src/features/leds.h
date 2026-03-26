#pragma once
#include <map>
#include "core/base.h"
#include <plugin.h>
#include "utils/modelinfomgr.h"
#include "enums/materialtype.h"

struct VehLEDData
{
	std::map<eMaterialType, bool> bLEDStates;
	VehLEDData(CVehicle *pVeh) {}
	~VehLEDData() {}
};

class DashboardLEDs : public CVehFeature<VehLEDData>
{
private:
	static inline bool bEnabled = false;

	static void EnableLED(CVehicle *pVeh, eMaterialType state);
	
protected:
    void Init() override;

public:
	public:
    DashboardLEDs() : CVehFeature<VehLEDData>("DashboardLED", "FEATURES", eFeatureMatrix::DashboardLED) {}
};