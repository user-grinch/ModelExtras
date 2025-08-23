#pragma once
#include "matrix.h"
#include "frame.h"
#include "render.h"

typedef enum class eModelEntityType eModelEntityType;

class Util
{
public:
  static bool IsNightTime();
  static bool IsEngineOff(CVehicle *pVeh);
  static bool IsDoorDamaged(CVehicle *pVeh, eDoors door);
  static bool IsLightDamaged(CVehicle *pVeh, eLights light);
  static bool IsPanelDamaged(CVehicle *pVeh, ePanels panel);
  static CVector UpdateRelativeToBoundingBox(CVehicle *pVeh, eDummyPos dummyPos, CVector center, CVector up, CVector right);

  static float NormalizeAngle(float angle);
  static double RadToDeg(double rad);
  static double DegToRad(double rad);
 
  // Returns the speed of the vehicle handler
  static float GetVehiclePitch(CVehicle *pVeh);
  static bool IsVehicleDoingWheelie(CVehicle *pVeh);
  static float GetVehicleSpeed(CVehicle *pVeh);
  static float GetVehicleSpeedRealistic(CVehicle *vehicle);
  static unsigned int GetEntityModel(void *ptr, eModelEntityType type);
  static void GetModelsFromIni(std::string &line, std::vector<int> &vec);

  static std::optional<int> GetDigitsAfter(const std::string &str, const std::string &prefix);
  static std::optional<std::string> GetCharsAfterPrefix(const std::string &str, const std::string &prefix, size_t num_chars);
};
