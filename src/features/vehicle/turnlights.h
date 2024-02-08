#pragma once
#include "plugin.h"
#include "../../interface/ifeature.hpp"

enum class eLightsStatus {
  Off, 
  Left, 
  Right, 
  Both
};

class TurnLightsFeature : public IFeature {
protected:
  struct VehData {
    eLightsStatus lightsStatus;

    VehData(CVehicle *) : lightsStatus(eLightsStatus::Off) {}
  };

  VehicleExtendedData<VehData> vehData;

public:
  void Initialize();
};

extern TurnLightsFeature TurnLights;