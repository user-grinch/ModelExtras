#pragma once
#include <plugin.h>
#include "core/base.h"

struct SteerWheelData
{
    RwFrame *pFrame = nullptr;
    float factor = 1.0f;
    float prevAngle = 0.0f;

    SteerWheelData(CVehicle *pVeh) {}
    ~SteerWheelData() {}
};

class SteerWheel : public CVehFeature<SteerWheelData>
{
protected:
    void Init() override;
  

public:
  public:
    SteerWheel() : CVehFeature<SteerWheelData>("RotatingSteeringWheel", "FEATURES", eFeatureMatrix::RotatingSteeringWheel) {}
  static void Process(void *ptr, RwFrame *frame, eModelEntityType type);
};
