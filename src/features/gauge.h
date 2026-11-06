#pragma once
#include <plugin.h>
#include "core/base.h"
#include <vector>
#include <unordered_map>

struct GearIndicatorData {
  uint iCurrent = UINT_MAX;
  RwFrame *pRoot = nullptr;
  std::vector<RwFrame *> vecFrameList;
};

struct VehGearData
{
    FeatureFrameState frameState;
  bool bInitialized = false;
  std::vector<GearIndicatorData> vecIndicatorData;

  VehGearData(CVehicle *pVeh) {}
  ~VehGearData() {}
};

class GearIndicator : public CVehFeature<VehGearData>
{
protected:
    void OnToggle(CVehicle *vehicle, bool enabled) override;
  void Init() override;

public:
  GearIndicator() : CVehFeature<VehGearData>("AnimatedGearMeter", "FEATURES", eFeatureMatrix::AnimatedGearMeter) {}
};

struct MileageIndicatorData {
  RwFrame *pFrame = nullptr;
  double dCurrentDistance = 0.0;
  float fLastWheelRot = 0.0f;
  std::vector<RwFrame *> vecFrameList;
  int lastDigits[6] = {-1, -1, -1, -1, -1, -1};
  float fMul = 160.9f;
};

struct VehMileageData
{
    FeatureFrameState frameState;
  bool bInitialized = false;
  std::unordered_map<std::string, MileageIndicatorData> vecIndicatorData;

  VehMileageData(CVehicle *pVeh) {}
  ~VehMileageData() {}
};

class MileageIndicator : public CVehFeature<VehMileageData>
{
protected:
    void OnToggle(CVehicle *vehicle, bool enabled) override;
  void Init() override;

public:
  MileageIndicator() : CVehFeature<VehMileageData>("AnimatedOdoMeter", "FEATURES", eFeatureMatrix::AnimatedOdoMeter) {}
};

struct RPMGaugeData {
  RwFrame *pFrame = nullptr;
  int iMaxRPM = 8000;
  int iPrevGear = -1;
  float fCurRotation = 0.0f;
  float fMaxRotation = 260.0f;
};

struct VehRPMData
{
    FeatureFrameState frameState;
  bool bInitialized = false;
  std::unordered_map<std::string, RPMGaugeData> vecGaugeData;

  VehRPMData(CVehicle *pVeh) {}
  ~VehRPMData() {}
};

class RPMGauge : public CVehFeature<VehRPMData>
{
protected:
    void OnToggle(CVehicle *vehicle, bool enabled) override;
  void Init() override;

public:
  RPMGauge() : CVehFeature<VehRPMData>("AnimatedRpmMeter", "FEATURES", eFeatureMatrix::AnimatedRpmMeter) {}
};

struct SpeedGaugeData {
  RwFrame *pFrame = nullptr;
  int iMaxSpeed = 240;
  float fMul = 1.0f;
  float fCurRotation = 0.0f;
  float fMaxRotation = 260.0f;
};

struct VehSpeedData
{
    FeatureFrameState frameState;
  bool bInitialized = false;
  std::unordered_map<std::string, SpeedGaugeData> vecGaugeData;

  VehSpeedData(CVehicle *pVeh) {}
  ~VehSpeedData() {}
};

class SpeedGauge : public CVehFeature<VehSpeedData>
{
protected:
    void OnToggle(CVehicle *vehicle, bool enabled) override;
  void Init() override;

public:
  SpeedGauge() : CVehFeature<VehSpeedData>("AnimatedSpeedMeter", "FEATURES", eFeatureMatrix::AnimatedSpeedMeter) {}
};

struct TurboGaugeData {
  RwFrame *pFrame = nullptr;
  float fPrevTurbo = 0.0f;
  float iMaxTurbo = 220.0f;
  float fCurRotation = 0.0f;
  float fMaxRotation = 220.0f;
};

struct VehTurboData
{
    FeatureFrameState frameState;
  bool bInitialized = false;
  std::unordered_map<std::string, TurboGaugeData> vecGaugeData;

  VehTurboData(CVehicle *pVeh) {}
  ~VehTurboData() {}
};

class TurboGauge : public CVehFeature<VehTurboData>
{
protected:
    void OnToggle(CVehicle *vehicle, bool enabled) override;
  void Init() override;

public:
  TurboGauge() : CVehFeature<VehTurboData>("AnimatedTurboMeter", "FEATURES", eFeatureMatrix::AnimatedTurboMeter) {}
};

struct FixedGaugeData {
  RwFrame *frame = nullptr;
  float fraction = 0.0f;
  float angle = 0.0f;
};
struct VehFixedGaugeData {
    FeatureFrameState frameState;
  std::vector<FixedGaugeData> gauges;
  VehFixedGaugeData(CVehicle *) {}
};

class FixedGauge : public CVehFeature<VehFixedGaugeData>
{
protected:
  void OnToggle(CVehicle *vehicle, bool enabled) override;
  void Init() override;

public:
  FixedGauge() : CVehFeature<VehFixedGaugeData>("AnimatedGasMeter", "FEATURES", eFeatureMatrix::AnimatedGasMeter) {}
};
