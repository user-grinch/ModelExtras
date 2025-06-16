#pragma once
#include <plugin.h>
#include <vector>

enum ePlateType
{
  DAY_SF,
  DAY_LV,
  DAY_LS,
  DAY_CS,
  NIGHT_SF,
  NIGHT_LV,
  NIGHT_LS,
  NIGHT_CS,
  TOTAL_SZ
};

class LicensePlate
{
private:
  static inline bool m_bEnabled = false;
  static inline RwTexture *pCharSetTex = nullptr;
  static inline RwTexture *m_Plates[ePlateType::TOTAL_SZ];

  static constexpr uint32_t CHARSET_CHAR_WIDTH{32u};
  static constexpr uint32_t CHARSET_CHAR_HEIGHT{64u};
  static constexpr uint32_t CHARSET_COL_WIDTH{4 * CHARSET_CHAR_WIDTH}; // 4x chars / row
  static constexpr uint32_t CHARSET_ROW_HEIGHT{64u};
  static constexpr uint32_t MAX_TEXT_LENGTH{8u};
  static inline RwUInt8 *pCharsetLockedData;

  struct VehData
  {
    bool m_bInit = false;
    int cityId = -1;
    bool m_bNightTexture = false;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> vehData;
  static RpMaterial *__cdecl CCustomCarPlateMgr_SetupMaterialPlatebackTexture(RpMaterial *material, char plateType);
  static bool __cdecl CCustomCarPlateMgr_Initialise();
  static void __cdecl CCustomCarPlateMgr_Shudown();
  static RwTexture *CCustomCarPlateMgr_CreatePlateTexture(char *text, uint8_t plateType);
  static bool CCustomCarPlateMgr_RenderLicenseplateTextToRaster(const char *text, RwRaster *charsRaster, RwRaster *plateRaster);

public:
  static void Initialize();
  static void ProcessTextures(CVehicle *pVeh, RpMaterial *pMat);
};