#pragma once
#include "core/dummy.h"
#include "enums/indicatorstate.h"
#include "enums/lightingmode.h"
#include "enums/lightoverride.h"
#include "enums/lighttype.h"
#include "modelinfomgr.h"
#include <map>
#include <plugin.h>

struct VehLightData {
  bool m_bFogLightsOn = false;
  bool m_bLongLightsOn = false;
  eIndicatorState m_nIndicatorState = eIndicatorState::Off;
  bool m_bUsingGlobalIndicators = false;
  bool m_bLightStates[eLightType::TotalLight];

  VehLightData(CVehicle *pVeh) {
    std::fill(std::begin(m_bLightStates), std::end(m_bLightStates), true);
  }
  ~VehLightData() {}
};

class Lights {
private:
  static inline bool m_bEnabled = false;
  static inline bool indicatorsDelay;
  static inline VehicleExtendedData<VehLightData> m_VehData;

  static inline std::map<CVehicle *,
                         std::map<eLightType, std::vector<VehicleDummy *>>>
      m_Dummies;

  static void EnableDummy(int id, VehicleDummy *dummy, CVehicle *vehicle,
                          float szMul = 1.0f);
  static void RenderLight(CVehicle *pVeh, eLightType state, bool shadows,
                          std::string texture, float sz, bool highlight);
  static void RenderLights(CVehicle *pControlVeh, CVehicle *pTowedVeh,
                           eLightType state, bool shadows = true,
                           std::string texture = "indicator", float sz = 1.0f,
                           bool highlight = false);
  static void RenderHeadlights(CVehicle *pControlVeh, bool isLeftOn,
                               bool isRightOn, bool realTime = true);
  static void __cdecl hkTailLightCCoronas_RegisterCorona(
      uint32_t id, CVehicle *pVeh, uint8_t r, uint8_t g, uint8_t b, uint8_t a,
      CVector *pos, float size, float range, int coronaType, uint8_t flareType,
      uint8_t reflectionType, bool checkObstacles, int bUsesTrails,
      float fNormalAngle, bool bNeonFade, float fPullTowardsCam,
      bool bFullBrightAtStart, float fadeSpeed, bool bOnlyFromBelow,
      bool bWhiteCore);

  // Helper functions
  static bool IsDummyAvail(CVehicle *pVeh, eLightType state);
  static bool IsDummyAvail(CVehicle *pVeh,
                           std::initializer_list<eLightType> states);
  static bool IsMatAvail(CVehicle *pVeh, eLightType state);
  static bool IsMatAvail(CVehicle *pVeh,
                         std::initializer_list<eLightType> states);

public:
  static void Initialize();
  static bool IsIndicatorOn(CVehicle *pVeh);
  friend int GetSirenIndex(CVehicle *pVeh, RpMaterial *pMat);
  static void Reload(CVehicle *pVeh);

  static bool GetLightState(CVehicle *pVeh, eLightType lightId);
  static void SetLightState(CVehicle *pVeh, eLightType lightId, bool state);
};