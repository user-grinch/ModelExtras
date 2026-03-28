#pragma once
#include "core/base.h"

class LightsFeature : public CBaseFeature{
protected:
    void Init() override;

public:
    LightsFeature() : CBaseFeature("Lights", "FEATURES", eFeatureMatrix::StandardLights) {}
};