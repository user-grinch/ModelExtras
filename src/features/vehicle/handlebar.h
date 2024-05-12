#pragma once
#include "plugin.h"
#include "../../interface/ifeature.hpp"
#include <vector>

class HandleBar{
  protected:
    struct VehData {
      RwFrame* m_pSource = nullptr;

      VehData(CVehicle *pVeh) {}
      ~VehData() {}
    };

    static inline VehicleExtendedData<VehData> xData;

  public:
    static void AddSource(RwFrame* frame, CEntity* ptr);
    static void Process(RwFrame* frame, CEntity* ptr);
};