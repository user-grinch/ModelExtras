#pragma once
#include <plugin.h>
#include "core/base.h"

struct BackfireData
{
    bool wasFullThrottled = false;
    int m_nleftFires = 0;

    BackfireData(CVehicle *pVeh) {}
    ~BackfireData() {}
};

class BackFireEffect : public CVehFeature<BackfireData>
{
protected:
  void Init() override;
  static void BackFireFX(CVehicle *pVeh, float x, float y, float z, float dirX = 0.0f, float dirY = -25.0f, float dirZ = 0.0f);
  static void BackFireSingle(CVehicle *pVeh);
  static void BackFireMulti(CVehicle *pVeh);
  static void Process(CVehicle *pVeh);

public:
  BackFireEffect() : CVehFeature<BackfireData>("BackfireEffect", "FEATURES", eFeatureMatrix::BackfireEffect) {}
};