#pragma once
#include <plugin.h>
#include "core/base.h"
#include <vector>

struct SpotlightData
{
	RwFrame *pFrame = nullptr;
	bool bEnabled = false;
	SpotlightData(CVehicle *pVeh) {}
	~SpotlightData() {}
};

class SpotLights : public CVehFeature<SpotlightData>
{
protected:
    void Init() override;

public:
	static inline RwTexture *pSpotlightTex = nullptr;

	

	static void OnHudRender();
	static void OnVehicleRender(CVehicle *pVeh);

public:
    SpotLights() : CVehFeature<SpotlightData>("SpotLights", "FEATURES", eFeatureMatrix::StandardLights) {}
	static bool IsEnabled(CVehicle *pVeh);
};