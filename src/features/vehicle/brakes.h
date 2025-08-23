#pragma once
#include <plugin.h>

class FrontBrake {
protected:
  struct VehData {
    RwFrame *pFrame = nullptr;
    bool m_bInitialized = false;
    int m_nCurRotation = 0;
    int m_nMaxRotation = -20.0f;
    uint m_nLastFrameMS = 0;
    uint m_nWaitTime = 0;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> vehData;

public:
  static void Initialize();
};

class RearBrake {
protected:
  struct VehData {
    RwFrame *pFrame = nullptr;
    bool m_bInitialized = false;
    int m_nCurRotation = 0;
    int m_nMaxRotation = 20;
    uint m_nLastFrameMS = 0;
    uint m_nWaitTime = 0;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> vehData;

public:
  static void Initialize();
};