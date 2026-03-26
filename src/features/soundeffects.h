#pragma once
#include "utils/audiomgr.h"
#include "core/base.h"



struct SoundEffectsData
{
    bool m_bEngineState = false;
    bool m_bIndicatorState = false;
    float m_fBrakePressure = 0.0f;
    SoundEffectsData(CVehicle *pVeh) {}
    ~SoundEffectsData() {}
};

class SoundEffects : public CVehFeature<SoundEffectsData>
{
private:
    

protected:
    void Init() override;

public:
    SoundEffects() : CVehFeature<SoundEffectsData>("SoundEffects", "FEATURES", eFeatureMatrix::SoundEffects) {}
};