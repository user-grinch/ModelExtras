#pragma once
#include <plugin.h>

class FrontBrake {
protected:
  struct VehData {
    bool m_bInitialized = false;
    int m_nCurRotation = 0;
    int m_nMaxRotation = 0;
    uint m_nLastFrameMS = 0;
    uint m_nWaitTime = 0;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> vehData;

public:
  static inline void Initialize(RwFrame* frame, CEntity* pVeh);
  static void Process(void* ptr, RwFrame* frame, eModelEntityType type);
};

class RearBrake {
protected:
  struct VehData {
    bool m_bInitialized = false;
    int m_nCurRotation = 0;
    int m_nMaxRotation = 0;
    uint m_nLastFrameMS = 0;
    uint m_nWaitTime = 0;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> vehData;

public:
  static inline void Initialize(RwFrame* frame, CEntity* pVeh);
  static void Process(void* ptr, RwFrame* frame, eModelEntityType type);
};