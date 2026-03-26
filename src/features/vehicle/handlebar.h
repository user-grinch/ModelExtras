#pragma once
#include <plugin.h>
#include <vector>

class HandleBar
{
protected:
  struct VehData
  {
    float prevAngle = 0.0f;
    RwFrame *m_pOrigin = nullptr;
    RwFrame *m_pTarget = nullptr;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> xData;

public:
  static void Initialize();
};