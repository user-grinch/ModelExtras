#pragma once
#include <plugin.h>

class BackFireEffect
{
protected:
  struct VehData
  {
    bool m_bInitialized = false;
    size_t m_nleftFires = 0;
    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> vehData;
  static void BackFireFX(CVehicle *pVeh, float x, float y, float z);
  static void BackFireSingle(CVehicle *pVeh);
  static void BackFireMulti(CVehicle *pVeh);

public:
  static inline void Initialize(RwFrame *frame, CEntity *pVeh);
  static void Process(void *ptr, RwFrame *frame, eModelEntityType type);
};