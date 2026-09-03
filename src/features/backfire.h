#pragma once
#include <plugin.h>
#include "core/base.h"
#include <Fx_c.h>
#include <vector>

struct BackfireData
{
    bool wasFullThrottled = false;
    int m_nleftFires = 0;
    size_t prevTimer = 0;
    int lastSoundizeGear = 0;

    bool bSystemsInitialized = false;
    std::vector<FxSystem_c *> m_fxLow;
    std::vector<FxSystem_c *> m_fxHigh;
    std::vector<RwMatrix *> m_matrices;

    void CleanUpSystems();

    BackfireData(CVehicle *pVeh) {}
    ~BackfireData();
};

class BackFireEffect : public CVehFeature<BackfireData>
{
protected:
  void Init() override;
  void ReloadConfig() override;
  void Reload(CVehicle *pVeh) override { ReloadConfig(); }
  static void EnsureSystemsCreated(CVehicle *pVeh, BackfireData &data);
  static void BackFireSingle(CVehicle *pVeh, bool bPlaySound = true);
  static void BackFireMulti(CVehicle *pVeh, bool bPlaySound = true);
  static void Process(CVehicle *pVeh);

public:
  BackFireEffect() : CVehFeature<BackfireData>("BackfireEffect", "FEATURES", eFeatureMatrix::BackfireEffect) {}
};