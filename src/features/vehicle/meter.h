#pragma once
#include "plugin.h"
#include "../../interface/ifeature.hpp"
#include <vector>

class GearMeter {
protected:
  struct VehData {
    bool m_bInitialized = false;
    uint m_nCurrent = 0;
    std::vector<RwFrame*> m_FrameList;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> vehData;

public:
  static void Process(RwFrame* frame, CEntity* ptr);
};

class OdoMeter {
protected:
  struct VehData {
    bool m_bInitialized = false;
    int m_nTempVal = 0;
    std::string m_ScreenText = "000000";
    std::vector<RwFrame*> m_FrameList;
    float m_fMul = 160.9f;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> vehData;

public:
  static void Process(RwFrame* frame, CEntity* ptr);
};

class RpmMeter {
protected:
  struct VehData {
    bool m_bInitialized = false;
    int m_nMaxRpm = 0;
    float m_fCurRotation = 0.0f;
    float m_fMaxRotation = 0.0f;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> vehData;

public:
  static void Process(RwFrame* frame, CEntity* ptr);
};

class SpeedMeter {
protected:
  struct VehData {
      bool m_bInitialized = false;
      int m_nMaxSpeed = 0;
      float m_fMul = 160.9f;
      float m_fCurRotation = 0.0f;
      float m_fMaxRotation = 0.0f;

      VehData(CVehicle *pVeh) {}
      ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> vehData;

public:
  static void Process(RwFrame* frame, CEntity* ptr);
};

class TachoMeter{
protected:
  struct VehData {
    bool m_bInitialized = false;
    int m_nMaxVal = 0;
    float m_fCurRotation = 0.0f;
    float m_fMaxRotation = 0.0f;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> vehData;

public:
  static void Process(RwFrame* frame, CEntity* ptr);
};

class GasMeter {
protected:
  struct VehData {
    bool m_bInitialized = false;
    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> vehData;

public:
  static void Process(RwFrame* frame, CEntity* ptr);
};