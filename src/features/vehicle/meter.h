#pragma once
#include "../../interface/ifeature.hpp"
#include <plugin.h>
#include <vector>

class GearMeter {
protected:
  struct VehData {
    uint m_nCurrent = 0;
    RwFrame *pRoot = nullptr;
    std::vector<RwFrame *> m_FrameList;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> vehData;

public:
  static void Initialize();
};

class OdoMeter {
protected:
  struct VehData {
    RwFrame *pFrame = nullptr;
    bool m_bInitialized = false;
    bool m_bDigital = false;
    int m_nPrevRot = 0;
    std::string m_ScreenText = "000000";
    std::vector<RwFrame *> m_FrameList;
    float m_fMul = 160.9f;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> vehData;

public:
  static void Initialize();
};

class RpmMeter {
protected:
  struct VehData {
    RwFrame *pFrame = nullptr;
    bool m_bInitialized = false;
    int m_nMaxRpm = 5000.0f;
    int prevGear = -1;
    float m_fCurRotation = 0.0f;
    float m_fMaxRotation = 260.0f;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> vehData;

public:
  static void Initialize();
};

class SpeedMeter {
protected:
  struct VehData {
    RwFrame *pFrame = nullptr;
    bool m_bInitialized = false;
    int m_nMaxSpeed = 100.0f;
    float m_fMul = 160.9f;
    float m_fCurRotation = 0.0f;
    float m_fMaxRotation = 100.0f;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> vehData;

public:
  static void Initialize();
};

class TurboMeter {
protected:
  struct VehData {
    RwFrame *pFrame = nullptr;
    bool m_bInitialized = false;
    float m_nMaxTurbo = 220.0f;
    float m_fCurRotation = 0.0f;
    float m_fMaxRotation = 220.0f;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> vehData;

public:
  static void Initialize();
};

class GasMeter {
public:
  static void Initialize();
};