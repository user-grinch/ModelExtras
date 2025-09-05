#pragma once
#include <plugin.h>

class WheelHub {
protected:
  struct VehData {
    RwFrame *m_pWRF = NULL, *m_pWRM = NULL, *m_pWRR = NULL;
    RwFrame *m_pWLF = NULL, *m_pWLM = NULL, *m_pWLR = NULL;

    RwFrame *m_pHRF = NULL, *m_pHRM = NULL, *m_pHRR = NULL;
    RwFrame *m_pHLF = NULL, *m_pHLM = NULL, *m_pHLR = NULL;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> xData;

public:
  static void Initialize();
};