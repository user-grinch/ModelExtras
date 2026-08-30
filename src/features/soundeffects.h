#pragma once
#include "utils/audiomgr.h"
#include "core/base.h"



struct SoundEffectsData
{
    bool m_bEngineState = false;
    bool m_bIndicatorState = false;
    bool m_bInitialized = false;
    unsigned int m_nLastEngineSoundTime = 0;
    unsigned int m_nLastReverseSoundTime = 0;
    float m_fBrakePressure = 0.0f;
    float m_fMaxPedal = 0.0f;
    SoundEffectsData(CVehicle *pVeh) {}
    ~SoundEffectsData() {}
};

class SoundEffects : public CVehFeature<SoundEffectsData>
{
protected:
    void Init() override;
    void Reload(CVehicle *pVeh) override;
    static void ReloadConfig();

public:
    SoundEffects() : CVehFeature<SoundEffectsData>("SoundEffects", "SOUND", eFeatureMatrix::SoundEffects) {}
};