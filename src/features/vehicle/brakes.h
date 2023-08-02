#pragma once
#include "plugin.h"
#include "../../interface/ifeature.hpp"

class FrontBrakeFeature : public IFeature {
protected:
    struct VehData
	{
		bool m_bInitialized = false;
		int m_nCurRotation = 0;
		int m_nMaxRotation = 0;
		uint m_nLastFrameMS = 0;
		uint m_nWaitTime = 0;
		
		VehData(CVehicle *pVeh){}
		~VehData(){}
	};

	VehicleExtendedData<VehData> vehData;

public:
    void Initialize(RwFrame* frame, CVehicle* pVeh);
	void Process(RwFrame* frame, CVehicle* pVeh);
};

extern FrontBrakeFeature FrontBrake;

class RearBrakeFeature : public FrontBrakeFeature {
public:
    void Initialize(RwFrame* frame, CVehicle* pVeh);
	void Process(RwFrame* frame, CVehicle* pVeh);
};

extern RearBrakeFeature RearBrake;