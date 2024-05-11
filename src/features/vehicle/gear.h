#pragma once
#include "plugin.h"
#include <vector>
extern struct CAudioStream;

enum class eFrameState {
  AtOrigin,
  IsMoving,
  AtOffset,
};

class Clutch {
protected:
  struct VehData {
    bool m_bInitialized = false;
    eFrameState m_eState = eFrameState::AtOrigin;
    float m_fCalVal = 1.0f;
    float m_fCurRotation = 0.0f;
    int m_nCurOffset = 0;
    uint m_nWaitTime = 0;
    uint m_nLastFrameMS = 0;
    short m_nLastGear = 0;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> vehData;

public:
  static void Process(RwFrame* frame, CEntity* ptr);
};

class GearLever {
protected:
  struct VehData {
    bool m_bInitialized = false;
    eFrameState m_eState = eFrameState::AtOrigin;
    float m_fCalVal = 1.0f;
    float m_fCurRotation = 0.0f;
    int m_nCurOffset = 0;
    uint m_nWaitTime = 0;
    uint m_nLastFrameMS = 0;
    short m_nLastGear = 0;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> vehData;

public:
  static void Process(RwFrame* frame, CEntity* ptr);
};

class GearSound {
protected:
  struct VehData {
    bool m_bInitialized = false;
    uint m_nCurGear = 0;
    CAudioStream *m_pUpAudio, *m_pDownAudio;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> vehData;

public:
  static void Process(RwFrame* frame, CEntity* ptr);
};