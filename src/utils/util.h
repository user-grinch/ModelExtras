#pragma once
#include "CRGBA.h"
#include "matrix.h"
#include "car.h"
#include "frame.h"
#include "render.h"
#include "mathutil.h"
#include "stringutil.h"
#include "worldutil.h"
#include "inputmgr.h"

typedef enum class eModelEntityType eModelEntityType;

class Util
{
public:
  static inline bool IsNightTime() { return WorldUtil::IsNightTime(); }
  static inline bool IsFoggy() { return WorldUtil::IsFoggy(); }
  static inline bool IsEngineOff(CVehicle *pVeh) { return CarUtil::IsEngineOff(pVeh); }
  static bool IsWindowFocused();
  static inline bool IsKeyPressed(int keyCode) { return InputMgr::IsKeyDown(keyCode); }
  static inline bool IsDoorDamaged(CVehicle *pVeh, eDoors door) { return CarUtil::IsDoorDamaged(pVeh, door); }
  static inline bool IsLightDamaged(CVehicle *pVeh, eLights light) { return CarUtil::IsLightDamaged(pVeh, light); }
  static inline bool IsPanelDamaged(CVehicle *pVeh, ePanels panel) { return CarUtil::IsPanelDamaged(pVeh, panel); }
  static inline bool IsFrameDamaged(CVehicle *pVeh, RwFrame *frame) { return CarUtil::IsFrameDamaged(pVeh, frame); }
  static inline CVector UpdateRelativeToBoundingBox(CVehicle *pVeh, eDummyPos dummyPos, CVector center, CVector up, CVector right) {
    return CarUtil::UpdateRelativeToBoundingBox(pVeh, dummyPos, center, up, right);
  }

  static inline float NormalizeAngle(float angle) { return MathUtil::NormalizeAngle(angle); }
  static inline double RadToDeg(double rad) { return MathUtil::RadToDeg(rad); }
  static inline double DegToRad(double deg) { return MathUtil::DegToRad(deg); }

  static inline float GetVehiclePitch(CVehicle *pVeh) { return CarUtil::GetVehiclePitch(pVeh); }
  static inline bool IsVehicleDoingWheelie(CVehicle *pVeh) { return CarUtil::IsVehicleDoingWheelie(pVeh); }
  static inline float GetVehicleSpeed(CVehicle *pVeh) { return CarUtil::GetVehicleSpeed(pVeh); }
  static inline float GetVehicleSpeedRealistic(CVehicle *vehicle) { return CarUtil::GetVehicleSpeedRealistic(vehicle); }
  static inline void GetModelsFromIni(std::string &line, std::vector<int> &vec) { StringUtil::GetModelsFromIni(line, vec); }

  static inline std::optional<int> GetDigitsAfter(const std::string_view str, const std::string_view prefix) {
    return StringUtil::GetDigitsAfter(str, prefix);
  }
  static inline std::optional<std::string> GetCharsAfterPrefix(const std::string_view str, const std::string_view prefix, size_t num_chars) {
    return StringUtil::GetCharsAfterPrefix(str, prefix, num_chars);
  }

  static CRGBA GetMaterialColor(RpMaterial *pMat);
  static bool IsAntiPatternLightMaterial(RpMaterial *pMat);
};
