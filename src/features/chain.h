#pragma once
#include "plugin.h"
#include "../interface/ifeature.hpp"
#include <vector>

class ChainFeature : public IFeature {
protected:
    struct VehData
	{
		uint m_nCurChain = 0;
		uint m_nLastFrameMS = 0;
        std::vector<RwFrame*> m_FrameList;
		
		VehData(CVehicle *pVeh){}
		~VehData(){}
	};

	VehicleExtendedData<VehData> vehData;

public:
    void Initialize(RwFrame* frame, CVehicle* pVeh);
	void Process(RwFrame* frame, CVehicle* pVeh);
};

extern ChainFeature Chain;