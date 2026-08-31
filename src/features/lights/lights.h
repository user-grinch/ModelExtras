#pragma once
#include "core/base.h"

class LightsFeature : public CBaseFeature{
protected:
    void Init() override;

public:
    LightsFeature() : CBaseFeature("StandardLightsv2", "FEATURES", eFeatureMatrix::StandardLights) {}
    void ReloadConfig() override;
    void Reload(CVehicle* pVeh) override;
};