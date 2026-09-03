#pragma once
#include "core/base.h"

class LightsFeature : public CBaseFeature {
protected:
    void Init() override;

public:
    static inline bool m_bEnabled = false;
    LightsFeature() : CBaseFeature("StandardLightsv2", "LIGHTS", eFeatureMatrix::REMOVED_NULL) {}
    void ReloadConfig() override;
    void Reload(CVehicle* pVeh) override;
};