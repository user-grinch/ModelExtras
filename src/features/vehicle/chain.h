#pragma once
#include "plugin.h"
#include "../../interface/ifeature.hpp"
#include <vector>

class ChainFeature : public IFeature {
  protected:
    struct VehData {
        bool m_bInitialized = false;
        uint m_nCurChain = 0;
        uint m_nLastFrameMS = 0;
        std::vector<RwFrame*> m_FrameList;

        VehData(CVehicle *pVeh) {}
        ~VehData() {}
    };

    VehicleExtendedData<VehData> vehData;

  public:
    void Initialize(RwFrame* frame, CVehicle* pVeh, std::string& name);
    void Process(RwFrame* frame, CVehicle* pVeh, std::string& name);
};

extern ChainFeature Chain;