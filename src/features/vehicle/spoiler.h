#pragma once
#include <plugin.h>
#include <vector>
#include <map>

class Spoiler
{
protected:
  struct SpoilerData
  {
    float m_fRotation, m_fCurrentRotation;
    size_t m_nTime, m_nTriggerSpeed;
  };

  struct VehData
  {
    bool m_bInit = false;
    std::unordered_map<std::string, SpoilerData> m_Spoilers;
    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> vehData;
  static void Initialize(void *ptr, RwFrame *frame, eModelEntityType type);

public:
  static void Process(void *ptr, RwFrame *frame, eModelEntityType type);
};