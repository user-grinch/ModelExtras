#pragma once
#include <plugin.h>
#include <vector>

class SpotLight{
public:
	static inline RwTexture* pSpotlightTex = nullptr;
	static inline bool bHooksInjected = false;

	struct VehData {
		RwFrame* pFrame = nullptr;
		bool bEnabled = false, bInit = false;

		VehData(CVehicle *){}
	};
	static inline VehicleExtendedData<VehData> vehData;

	static void FindNodesRecursive(RwFrame* frame, CVehicle* vehicle);
	static void OnHudRender();
	static void OnVehicleRender(CVehicle *pVeh);

public:
	static void Process(void* ptr, RwFrame* frame, eModelEntityType type);
};