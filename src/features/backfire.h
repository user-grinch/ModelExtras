#pragma once
#include <plugin.h>
#include "core/base.h"

struct BackfireVehData
{
    bool wasFullThrottled = false;
    uint8_t prevGchanging = 1;
    int m_nleftFires = 0;
    uint32_t prevTimer = 0;

    BackfireVehData(CVehicle * = nullptr) {}
};

class BackFireEffect : public CVehFeature<BackfireVehData>
{
protected:
    void Init() override;
    void ReloadConfig() override;
    void Reload(CVehicle *pVeh) override { ReloadConfig(); }
    static void Process(CVehicle *pVeh);

public:
    BackFireEffect() : CVehFeature<BackfireVehData>("BackfireEffect", "FEATURES", eFeatureMatrix::BackfireEffect) {}
    static bool IsDisabledByConflict();
    static void BackFireSingle(CVehicle *pVeh, bool bPlaySound = true, bool bHighIntensity = false);
    static void BackFireMulti(CVehicle *pVeh, bool bPlaySound = true, bool bHighIntensity = false);
};
