#pragma once
#include "plugin.h"
#include "../../interface/ifeature.hpp"
#include <vector>

class WheelHubFeature : public IFeature {
protected:
  struct VehData {
    RwFrame *wheelrf = nullptr, *wheelrm = nullptr, *wheelrb = nullptr, *wheellf = nullptr, *wheellm = nullptr, *wheellb = nullptr;
    RwFrame *hubrf = nullptr, *hubrm = nullptr, *hubrb = nullptr, *hublf = nullptr, *hublm = nullptr, *hublb = nullptr;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  VehicleExtendedData<VehData> xData;

public:
  void Process(RwFrame* frame, CVehicle* pVeh);
};

extern WheelHubFeature WheelHub;