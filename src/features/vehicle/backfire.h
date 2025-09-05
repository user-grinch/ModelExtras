#pragma once
#include <plugin.h>

class BackFireEffect {
protected:
  struct VehData {
    bool m_bInitialized = false;
    size_t m_nleftFires = 0;
    bool wasFullThrottled = false;
    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> vehData;
  static void BackFireFX(CVehicle *pVeh, float x, float y, float z);
  static void BackFireSingle(CVehicle *pVeh);
  static void BackFireMulti(CVehicle *pVeh);
  static void Process(CVehicle *pVeh);

public:
  static void Initialize(RwFrame *frame, CEntity *pVeh);
};