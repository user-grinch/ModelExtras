#pragma once
#include <plugin.h>
#include "../../interface/ifeature.hpp"
#include <vector>

enum ePlateType {
  DAY_LS,
  DAY_SF,
  DAY_LV,
  NIGHT_LS,
  NIGHT_SF,
  NIGHT_LV,
  TOTAL_SZ
};

class LicensePlateFeature : public IFeature {
protected:
  static inline RwTexture* pCharSetTex = nullptr;
  static inline RwTexture* m_Plates[ePlateType::TOTAL_SZ];

  struct VehData {
    bool m_bInit = false;
    std::string plateText = "DEFAULT";
    bool m_bNightTexture = false;

    VehData(CVehicle* pVeh) {}
    ~VehData() {}
  };

  VehicleExtendedData<VehData> vehData;
  static RpMaterial* __cdecl CCustomCarPlateMgr_SetupMaterialPlatebackTexture(RpMaterial* material, char plateType);
  static bool __cdecl CCustomCarPlateMgr_Initialise();

public:
  void Initialize();
  void Process(RwFrame* frame, CVehicle* pVeh);
};

extern LicensePlateFeature LicensePlate;