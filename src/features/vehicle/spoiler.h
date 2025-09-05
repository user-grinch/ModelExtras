#pragma once
#include <plugin.h>
#include <vector>

class Spoiler {
protected:
  struct SpoilerData {
    RwFrame *m_pFrame = NULL;
    float m_fRotation, m_fCurrentRotation;
    size_t m_nTime, m_nTriggerSpeed;
  };

  struct VehData {
    std::vector<SpoilerData> m_Spoilers;
    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> xData;

public:
  static void Initialize();
};