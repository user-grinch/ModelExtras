#pragma once
#include <plugin.h>
#include "../../interface/ifeature.hpp"
#include <vector>

class SpotLightFeature : public IFeature {
public:
	RwTexture* pSpotlightTex = nullptr;
	bool bHooksInjected = false;

	struct VehData {
		RwFrame* pFrame = nullptr;
		bool bEnabled = false, bInit = false;

		VehData(CVehicle *){}
	};
	VehicleExtendedData<VehData> vehData;

	void FindNodesRecursive(RwFrame* frame, CVehicle* vehicle);
	void OnHudRender();
	void OnVehicleRender(CVehicle *pVeh);

public:
	void Initialize(RwFrame* frame, CVehicle* pVeh, std::string& name);
	void Process(RwFrame* frame, CVehicle* pVeh, std::string& name);
};

extern SpotLightFeature SpotLight;