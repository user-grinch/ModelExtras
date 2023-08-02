#pragma once
#include "plugin.h"
#include "../../interface/ifeature.hpp"
#include <vector>

class LicensePlateFeature : public IFeature {
protected:
    struct VehData
	{
		bool m_bInitialized = false;
		
		VehData(CVehicle *pVeh){}
		~VehData(){}
	};

	VehicleExtendedData<VehData> vehData;
public:
    void Initialize(RwFrame* frame, CVehicle* pVeh);
	void Process(RwFrame* frame, CVehicle* pVeh);
};

extern LicensePlateFeature LicensePlate;