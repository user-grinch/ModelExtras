#pragma once
#include <plugin.h>
#include "../../interface/ifeature.hpp"
#include <vector>

class WheelHub {
protected:
  struct VehData {
    RwFrame *wheelrf = nullptr, *wheelrm = nullptr, *wheelrb = nullptr, *wheellf = nullptr, *wheellm = nullptr, *wheellb = nullptr;
    RwFrame *hubrf = nullptr, *hubrm = nullptr, *hubrb = nullptr, *hublf = nullptr, *hublm = nullptr, *hublb = nullptr;
    bool m_bInit = false;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  }; 

  static inline VehicleExtendedData<VehData> xData;
  static void FindNodes(RwFrame * frame, void* pEntity) ;

public:
  static void Process(void* ptr, RwFrame* frame, eModelEntityType type);
};