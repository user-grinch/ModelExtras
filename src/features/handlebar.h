#pragma once
#include <plugin.h>
#include "core/base.h"
#include <vector>

struct HandlebarData
{
  RwFrame *m_pOrigin = nullptr;
  RwFrame *m_pTarget = nullptr;
  float prevAngle = 0.0f;

  HandlebarData(CVehicle *pVeh) {}
  ~HandlebarData() {}
};

class HandleBar : public CVehFeature<HandlebarData>
{
protected:
    void Init() override;
  

public:
  public:
    HandleBar() : CVehFeature<HandlebarData>("RotatingHandleBar", "FEATURES", eFeatureMatrix::RotatingHandleBar) {}
};