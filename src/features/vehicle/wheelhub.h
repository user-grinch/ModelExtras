#pragma once
#include <plugin.h>
#include "../../interface/ifeature.hpp"
#include <vector>

class WheelHub
{
protected:
  struct VehData
  {
    bool m_bInit = false;
    RwFrame *wheelrf = NULL, *wheelrm = NULL, *wheelrb = NULL, *wheellf = NULL, *wheellm = NULL, *wheellb = NULL;
    RwFrame *hubrf = NULL, *hubrm = NULL, *hubrb = NULL, *hublf = NULL, *hublm = NULL, *hublb = NULL;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> xData;
  static void FindNodes(RwFrame *frame, void *pEntity);

public:
  static void Process(void *ptr, RwFrame *frame, eModelEntityType type);
};