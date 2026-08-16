#pragma once
#include "core/base.h"
#include "core/dummy.h"
#include "utils/modelinfomgr.h"
#include "enums/lightingmode.h"
#include "enums/materialtype.h"
#include "enums/indicatorstate.h"

struct VehLightDatav1
{
	bool m_bFogLightsOn = false;
	bool m_bLongLightsOn = false;
	eIndicatorState m_nIndicatorState = eIndicatorState::Off;
	bool m_bUsingGlobalIndicators = false;
	bool m_bLightStates[eMaterialType::TotalMaterial];

	VehLightDatav1(CVehicle *pVeh) {
		std::fill(std::begin(m_bLightStates), std::end(m_bLightStates), true);
	}
	~VehLightDatav1() {}
};

class Lights : public CVehFeature<VehLightDatav1>
{
private:
	static inline bool m_bEnabled = false;
	static inline bool indicatorsDelay;

	static inline std::map<CVehicle *, std::map<eMaterialType, std::vector<VehicleDummy *>>> m_Dummies;

	static void EnableDummy(int id, VehicleDummy *dummy, CVehicle *vehicle, float szMul = 1.0f);

	static void RenderLight(CVehicle *pVeh, eMaterialType state, bool shadows, std::string texture, float sz, bool highlight, bool isDummyOk = true);
	static void RenderLights(CVehicle *pControlVeh, CVehicle *pTowedVeh, eMaterialType state, bool shadows = true, std::string texture = "indicator", float sz = 1.0f, bool highlight = false, bool isDummyOk = true);
	static void RenderHeadlights(CVehicle *pControlVeh, bool isLeftOn, bool isRightOn, bool realTime = true);

	// Helper functions
	static bool IsDummyAvail(CVehicle *pVeh, eMaterialType state);
	static bool IsDummyAvail(CVehicle* pVeh, std::initializer_list<eMaterialType> states);
	static bool IsMatAvail(CVehicle *pVeh, eMaterialType state);
	static bool IsMatAvail(CVehicle* pVeh, std::initializer_list<eMaterialType> states);
	
protected:
    void Init() override;

public:
    Lights() : CVehFeature<VehLightDatav1>("StandardLights", "FEATURES", eFeatureMatrix::StandardLights) {}
	static bool IsIndicatorOn(CVehicle *pVeh);
	static VehLightDatav1 GetVehicleData(CVehicle *pVeh);

	friend int GetSirenIndex(CVehicle *pVeh, RpMaterial *pMat);
	void Reload(CVehicle* pVeh) override;

	static bool GetLightState(CVehicle *pVeh, eMaterialType lightId);
	static void SetLightState(CVehicle *pVeh, eMaterialType lightId, bool state);
};