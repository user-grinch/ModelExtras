#pragma once
#include "plugin.h"
#include "../../interface/ifeature.hpp"
#include <vector>

enum class eSteerWheelRotation {
  Default,
  Left,
  Right
};

class SteerWheelFeature : public IFeature {
  protected:
    struct VehData {
        eSteerWheelRotation m_eRotation = eSteerWheelRotation::Default;

        VehData(CVehicle *pVeh) {}
        ~VehData() {}
    };

    VehicleExtendedData<VehData> xData;

  public:
    void Initialize(RwFrame* frame, CVehicle* pVeh);
    void Process(RwFrame* frame, CVehicle* pVeh);
};

extern SteerWheelFeature SteerWheel;