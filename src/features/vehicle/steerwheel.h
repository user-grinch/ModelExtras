#pragma once
#include <plugin.h>
#include "../../interface/ifeature.hpp"
#include <vector>

class SteerWheel
{
protected:
  struct VehData
  {
    float prevAngle = 0.0f;
    float factor = 1.0f;
    RwFrame *pFrame = NULL;
    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };
  static inline VehicleExtendedData<VehData> xData;

public:
  static void Initialize();
  static void Process(void *ptr, RwFrame *frame, eModelEntityType type);
};
