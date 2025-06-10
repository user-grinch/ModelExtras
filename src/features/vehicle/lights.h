#pragma once
#include <map>
#include <plugin.h>
#include "core/dummy.h"
#include "core/materials.h"
#include "enums/lightingmode.h"
#include "enums/lightoverride.h"
#include "enums/lighttype.h"
#include "enums/indicatorstate.h"

class Lights
{
private:
	static inline bool m_bEnabled = false;
	static inline bool indicatorsDelay;
	struct VehData
	{
		bool m_bFogLightsOn = false;
		bool m_bLongLightsOn = false;
		eIndicatorState m_nIndicatorState = eIndicatorState::Off;

		VehData(CVehicle *pVeh) {}
		~VehData() {}
	};
	static inline VehicleExtendedData<VehData> m_VehData;

	static inline std::map<CVehicle *, std::map<eLightType, std::vector<VehicleDummy *>>> m_Dummies;

	static void EnableDummy(int id, VehicleDummy *dummy, CVehicle *vehicle, float szMul = 1.0f);
	static void RenderLight(CVehicle *pVeh, eLightType state, bool shadows, std::string texture, CVector2D sz, CVector2D offset, bool highlight);
	static void RenderLights(CVehicle *pControlVeh, CVehicle *pTowedVeh, eLightType state, bool shadows = true, std::string texture = "indicator", CVector2D sz = {1.0f, 1.0f}, CVector2D offset = {0.0f, 0.0f}, bool highlight = false);
	static void __cdecl hkTailLightCCoronas_RegisterCorona(uint32_t id, CVehicle *pVeh, uint8_t r, uint8_t g, uint8_t b, uint8_t a, CVector *pos, float size, float range, int coronaType, uint8_t flareType, uint8_t reflectionType, bool checkObstacles, int bUsesTrails, float fNormalAngle, bool bNeonFade, float fPullTowardsCam, bool bFullBrightAtStart, float fadeSpeed, bool bOnlyFromBelow, bool bWhiteCore);

	// Helper functions
	static bool IsDummyAvail(CVehicle *pVeh, eLightType state);

public:

	static void Initialize();
	static bool IsIndicatorOn(CVehicle *pVeh)
	{
		return (pVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pVeh->m_nVehicleSubClass == VEHICLE_BIKE) && indicatorsDelay && m_VehData.Get(pVeh).m_nIndicatorState != eIndicatorState::Off && pVeh->m_nOverrideLights != eLightOverride::ForceLightsOff;
	}

	friend int GetSirenIndex(CVehicle *pVeh, RpMaterial *pMat);
	static void Reload(CVehicle *pVeh);
};