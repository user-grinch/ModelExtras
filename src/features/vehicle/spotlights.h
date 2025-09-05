#pragma once
#include <plugin.h>
#include <vector>

class SpotLights {
public:
  static inline RwTexture *pSpotlightTex = nullptr;

  struct VehData {
    RwFrame *pFrame = nullptr;
    bool bEnabled = false, bInit = false;
    VehData(CVehicle *) {}
  };
  static inline VehicleExtendedData<VehData> vehData;

  static void OnHudRender();
  static void OnVehicleRender(CVehicle *pVeh);

public:
  static void Initialize();
  static bool IsEnabled(CVehicle *pVeh);
};