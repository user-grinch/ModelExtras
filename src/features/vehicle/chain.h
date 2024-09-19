#pragma once
#include <plugin.h>
#include <vector>

class Chain {
protected:
  struct VehData {
    bool m_bInitialized = false;
    uint m_nCurChain = 0;
    uint m_nLastFrameMS = 0;
    std::vector<RwFrame*> m_FrameList;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> vehData;

public:
  static void Process(void* ptr, RwFrame* frame, eModelEntityType type);
};