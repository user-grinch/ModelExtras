#pragma once
#include <plugin.h>
#include <vector>

class GearIndicator
{
protected:
  struct IndicatorData {
    uint iCurrent = 0;
    RwFrame *pRoot = nullptr;
    std::vector<RwFrame *> vecFrameList;
  };

  struct VehData
  {
    bool bInitialized = false;
    std::vector<IndicatorData> vecIndicatorData;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> vehData;

public:
  static void Initialize();
};

class MileageIndicator
{
protected:
  struct IndicatorData {
    RwFrame *pFrame = nullptr;
    double dCurrentDistance = 0.0;
    float fLastWheelRot = 0.0f;
    std::vector<RwFrame *> vecFrameList;
    int lastDigits[6] = {-1, -1, -1, -1, -1, -1};
    float fMul = 160.9f;
  };

  struct VehData
  {
    bool bInitialized = false;
    std::unordered_map<std::string, IndicatorData> vecIndicatorData;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> vehData;

public:
  static void Initialize();
};

class RPMGauge
{
protected:
  struct GaugeData {
    RwFrame *pFrame = nullptr;
    int iMaxRPM = 5000.0f;
    int iPrevGear = -1;
    float fCurRotation = 0.0f;
    float fMaxRotation = 260.0f;
  };

  struct VehData
  {
    bool bInitialized = false;
    std::unordered_map<std::string, GaugeData> vecGaugeData;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> vehData;

public:
  static void Initialize();
};

class SpeedGauge
{
protected:
  struct GaugeData {
    RwFrame *pFrame = nullptr;
    int iMaxSpeed = 100.0f;
    float fMul = 160.9f;
    float fCurRotation = 0.0f;
    float fMaxRotation = 100.0f;
  };

  struct VehData
  {
    bool bInitialized = false;
    std::unordered_map<std::string, GaugeData> vecGaugeData;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> vehData;

public:
  static void Initialize();
};

class TurboGauge
{
protected:
  struct GaugeData {
    RwFrame *pFrame = nullptr;
    float fPrevTurbo = 0.0f;
    float iMaxTurbo = 220.0f;
    float fCurRotation = 0.0f;
    float fMaxRotation = 220.0f;
  };

  struct VehData
  {
    bool bInitialized = false;
    std::unordered_map<std::string, GaugeData> vecGaugeData;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> vehData;

public:
  static void Initialize();
};

class FixedGauge
{
public:
  static void Initialize();
};