#pragma once
#include <plugin.h>

class SteerWheel
{
protected:
  struct VehData
  {
    float prevAngle = 0.0f;
    float factor = 1.1f;
    RwFrame *pFrame = NULL;
    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };
  static inline VehicleExtendedData<VehData> xData;

public:
  static void Initialize();
  static void Process(void *ptr, RwFrame *frame, eModelEntityType type);
};
