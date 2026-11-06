#pragma once
#include <plugin.h>
#include "core/base.h"
#include <vector>

struct SpoilerData
{
  RwFrame *m_pFrame = nullptr;
  float m_fRotation = 0.0f;
  float m_fCurrentRotation = 0.0f;
  float m_nTime = 0.0f;
  float m_nTriggerSpeed = 0.0f;
};

struct SpoilerVehData
{
    FeatureFrameState frameState;
  std::vector<SpoilerData> m_Spoilers;
  SpoilerVehData(CVehicle *pVeh) {}
  ~SpoilerVehData() {}
};

class Spoiler : public CVehFeature<SpoilerVehData>
{
protected:
    void OnToggle(CVehicle *vehicle, bool enabled) override;
    void Init() override;
  

  

public:
  public:
    Spoiler() : CVehFeature<SpoilerVehData>("AnimatedSpoiler", "FEATURES", eFeatureMatrix::AnimatedSpoiler) {}
};