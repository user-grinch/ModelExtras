#pragma once
#include <plugin.h>
#include "../../interface/ifeature.hpp"
#include <vector>

enum class eSteerWheelRotation {
  Default,
  Left,
  Right
};

class SteerWheel{
protected:
  struct VehData {
    eSteerWheelRotation m_eRotation = eSteerWheelRotation::Default;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };
  static inline VehicleExtendedData<VehData> xData;

public:
  static void Process(void* ptr, RwFrame* frame, eModelEntityType type);
};
