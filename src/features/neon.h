#pragma once
#include "core/base.h"

class Neon : public CBaseFeature {
protected:
    void Init() override;

public:
    Neon() : CBaseFeature("Neon", "FEATURES", eFeatureMatrix::Neon) {}
};
