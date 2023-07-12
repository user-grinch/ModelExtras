#pragma once
#include "plugin.h"
#include "../interface/ifeature.hpp"
#include <vector>

enum class eFrameState {
	AtOrigin,
	IsMoving,
	AtOffset,
};

class ClutchFeature : public IFeature {
protected:
    struct VehData
	{
		eFrameState m_eState = eFrameState::AtOrigin;
		float m_fCalVal = 1.0f;
		float m_fCurRotation = 0.0f;
		int m_nCurOffset = 0;
		uint m_nWaitTime = 0;
		uint m_nLastFrameMS = 0;
		short m_nLastGear = 0;
		
		VehData(CVehicle *pVeh){}
		~VehData(){}
	};

	VehicleExtendedData<VehData> vehData;

public:
    void Initialize(RwFrame* frame, CVehicle* pVeh);
	void Process(RwFrame* frame, CVehicle* pVeh);
};

extern ClutchFeature Clutch;

class GearLeverFeature : public ClutchFeature {
public:
    void Initialize(RwFrame* frame, CVehicle* pVeh);
	void Process(RwFrame* frame, CVehicle* pVeh);
};

extern GearLeverFeature GearLever;


class GearSoundFeature : public IFeature {
protected:
    struct VehData
	{
		uint m_nCurGear = 0;
		HSTREAM m_hUpSound, m_hDownSound;
		
		VehData(CVehicle *pVeh){}
		~VehData(){}
	};

	VehicleExtendedData<VehData> vehData;

public:
    void Initialize(RwFrame* frame, CVehicle* pVeh);
	void Process(RwFrame* frame, CVehicle* pVeh);
};

extern GearSoundFeature GearSound;