#pragma once
#include <plugin.h>
#include <vector>

class ChainFeature
{
protected:
  struct VehData
  {
    uint m_nCurChain = 0;
    RwFrame *m_pRootFrame = nullptr;
    std::vector<RwFrame *> m_FrameList;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> vehData;

public:
  static void Initialize();
};