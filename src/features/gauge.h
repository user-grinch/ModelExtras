#pragma once
#include <plugin.h>
#include "core/base.h"
#include <vector>
#include <unordered_map>

struct GearIndicatorData {
  uint iCurrent = 0;
  RwFrame *pRoot = nullptr;
  std::vector<RwFrame *> vecFrameList;
};

struct VehGearData
{
  bool bInitialized = false;
  std::vector<GearIndicatorData> vecIndicatorData;

  VehGearData(CVehicle *pVeh) {}
  ~VehGearData() {}
};

class GearIndicator : public CVehFeature<VehGearData>
{
protected:
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
  bool bInitialized = false;
  std::unordered_map<std::string, MileageIndicatorData> vecIndicatorData;

  VehMileageData(CVehicle *pVeh) {}
  ~VehMileageData() {}
};

class MileageIndicator : public CVehFeature<VehMileageData>
{
protected:
  void Init() override;

public:
  MileageIndicator() : CVehFeature<VehMileageData>("AnimatedOdoMeter", "FEATURES", eFeatureMatrix::AnimatedOdoMeter) {}
};

struct RPMGaugeData {
  RwFrame *pFrame = nullptr;
  int iMaxRPM = 5000.0f;
  int iPrevGear = -1;
  float fCurRotation = 0.0f;
  float fMaxRotation = 260.0f;
};

struct VehRPMData
{
  bool bInitialized = false;
  std::unordered_map<std::string, RPMGaugeData> vecGaugeData;

  VehRPMData(CVehicle *pVeh) {}
  ~VehRPMData() {}
};

class RPMGauge : public CVehFeature<VehRPMData>
{
protected:
  void Init() override;

public:
  RPMGauge() : CVehFeature<VehRPMData>("AnimatedRpmMeter", "FEATURES", eFeatureMatrix::AnimatedRpmMeter) {}
};

struct SpeedGaugeData {
  RwFrame *pFrame = nullptr;
  int iMaxSpeed = 100.0f;
  float fMul = 160.9f;
  float fCurRotation = 0.0f;
  float fMaxRotation = 100.0f;
};

struct VehSpeedData
{
  bool bInitialized = false;
  std::unordered_map<std::string, SpeedGaugeData> vecGaugeData;

  VehSpeedData(CVehicle *pVeh) {}
  ~VehSpeedData() {}
};

class SpeedGauge : public CVehFeature<VehSpeedData>
{
protected:
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
  bool bInitialized = false;
  std::unordered_map<std::string, TurboGaugeData> vecGaugeData;

  VehTurboData(CVehicle *pVeh) {}
  ~VehTurboData() {}
};

class TurboGauge : public CVehFeature<VehTurboData>
{
protected:
  void Init() override;

public:
  TurboGauge() : CVehFeature<VehTurboData>("AnimatedTurboMeter", "FEATURES", eFeatureMatrix::AnimatedTurboMeter) {}
};

class FixedGauge : public CBaseFeature
{
protected:
  void Init() override;

public:
  FixedGauge() : CBaseFeature("AnimatedGasMeter", "FEATURES", eFeatureMatrix::AnimatedGasMeter) {}
};