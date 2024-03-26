#pragma once
#include "plugin.h"
#include "../../interface/ifeature.hpp"
#include <vector>

class HandleBarFeature : public IFeature {
  protected:
    struct VehData {
        RwFrame* m_pHandleBar = nullptr, *m_pForkFront = nullptr;

        VehData(CVehicle *pVeh) {}
        ~VehData() {}
    };

    VehicleExtendedData<VehData> xData;

  public:
    void Initialize(RwFrame* frame, CVehicle* pVeh);
    void Process(RwFrame* frame, CVehicle* pVeh);
};

extern HandleBarFeature HandleBar;