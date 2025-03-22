#include "pch.h"
#include "plate.h"
#include <CCustomCarPlateMgr.h>
#include <CWeather.h>
#include <rwcore.h>
#include <rpworld.h>
#include <RenderWare.h>
#include "defines.h"

PlateFeature LicensePlate;
extern bool IsNightTime();
extern bool IsEngineOff(CVehicle *pVeh);

bool lightState = false;
RpMaterial *__cdecl CCustomCarPlateMgr_SetupClump(RpAtomic *clump, void *plateText, char plateType)
{
    return NULL;
}

void PlateFeature::Initialize()
{
    CVehicleModelInfo
        plugin::patch::SetPointer(0xC3EF60, LicensePlate.m_Plates[DAY_SF]);
    plugin::patch::SetPointer(0xC3EF64, LicensePlate.m_Plates[DAY_LV]);
    plugin::patch::SetPointer(0xC3EF68, LicensePlate.m_Plates[DAY_LS]);
    plugin::patch::SetPointer(0xC3EF5C, &pCharSetTex);
    plugin::patch::SetPointer(0x6FDEE5, &pCharSetTex);
    plugin::patch::SetChar(0x6FDF47, rwFILTERLINEARMIPLINEAR);
    plugin::patch::ReplaceFunction(0x6FD500, CCustomCarPlateMgr_Initialise);
    plugin::patch::ReplaceFunction(0x6FD720, CCustomCarPlateMgr_Shudown);
    plugin::patch::ReplaceFunction(0x6FDE50, CCustomCarPlateMgr_SetupMaterialPlatebackTexture);
    plugin::patch::ReplaceFunction(0x6FDEA0, CCustomCarPlateMgr_CreatePlateTexture);
    // plugin::patch::ReplaceFunctionCall(0x4C949E, CCustomCarPlateMgr_SetupClump);
    // plugin::Events::vehicleRenderEvent.before += [this](CVehicle* pVeh) {
    //     bool hasDriver = (pVeh->m_pDriver != nullptr);
    //     bool curState = hasDriver ? IsNightTime() : false;

    //     auto& data = vehData.Get(pVeh);
    //     if (data.m_bNightTexture != curState || !data.m_bInit) {
    //         CVehicleModelInfo* pInfo = (CVehicleModelInfo*)CModelInfo::GetModelInfo(pVeh->m_nModelIndex);
    //         lightState = data.m_bNightTexture = curState;
    //         CCustomCarPlateMgr::SetupClump(pVeh->m_pRwClump, pInfo->m_szPlateText, pInfo->m_nPlateType);
    //         data.m_bInit = true;
    //     }
    //     };
}

void __cdecl PlateFeature::CCustomCarPlateMgr_Shudown()
{
    if (pCharSetTex)
    {
        RwRasterUnlock(pCharSetTex->raster);
        pCharsetLockedData = nullptr;
        RwTextureDestroy(pCharSetTex);
        pCharSetTex = nullptr;
    }

    for (size_t i = 0; i < ePlateType::TOTAL_SZ; i++)
    {
        RwTextureDestroy(m_Plates[i]);
    }
}

bool __cdecl PlateFeature::CCustomCarPlateMgr_Initialise()
{
    const char *dirPath = MOD_DATA_PATH_S(std::string("plates"));
    pCharSetTex = Util::LoadPNGTextureCB(dirPath, "platecharset");
    RwTextureSetFilterMode(pCharSetTex, rwFILTERLINEARMIPLINEAR);
    RwTextureSetAddressingU(pCharSetTex, rwFILTERLINEARMIPLINEAR);
    RwTextureSetAddressingV(pCharSetTex, rwFILTERLINEARMIPLINEAR);
    pCharSetTex->raster->stride = 512;

    // Don't add .dds extension!
    m_Plates[DAY_LV] = Util::LoadDDSTextureCB(dirPath, "plateback3");
    m_Plates[DAY_LS] = Util::LoadDDSTextureCB(dirPath, "plateback2");
    m_Plates[DAY_SF] = Util::LoadDDSTextureCB(dirPath, "plateback1");

    m_Plates[NIGHT_LV] = Util::LoadDDSTextureCB(dirPath, "plateback3_l");
    m_Plates[NIGHT_LS] = Util::LoadDDSTextureCB(dirPath, "plateback2_l");
    m_Plates[NIGHT_SF] = Util::LoadDDSTextureCB(dirPath, "plateback1_l");

    for (int i = 0; i < ePlateType::TOTAL_SZ; i++)
    {
        RwTextureSetAddressingU(m_Plates[i], rwFILTERLINEARMIPLINEAR);
        RwTextureSetAddressingV(m_Plates[i], rwFILTERLINEARMIPLINEAR);
        RwTextureSetFilterMode(m_Plates[i], rwFILTERLINEAR);
    }
    pCharsetLockedData = RwRasterLock(RwTextureGetRaster(LicensePlate.pCharSetTex), 0, rwRASTERLOCKREAD);
    return pCharsetLockedData != 0;
}

RpMaterial *__cdecl PlateFeature::CCustomCarPlateMgr_SetupMaterialPlatebackTexture(RpMaterial *material, char plateType)
{
    if (plateType == -1)
    {
        if (CWeather::WeatherRegion == 1)
            plateType = 2;
        else
            plateType = CWeather::WeatherRegion > 2 && CWeather::WeatherRegion <= 4;
    }

    if (IsNightTime())
    {
        material->surfaceProps.ambient = AMBIENT_ON_VAL;
        RpMaterialSetTexture(material, m_Plates[plateType + 3]);
    }
    else
    {
        material->surfaceProps.ambient = 1.0f;
        RpMaterialSetTexture(material, m_Plates[plateType]);
    }
    return material;
}

bool PlateFeature::CCustomCarPlateMgr_RenderLicenseplateTextToRaster(const char *text, RwRaster *charsRaster, RwRaster *plateRaster)
{
    assert(text);
    assert(charsRaster);
    assert(plateRaster);

    const auto lockedPlateRaster = RwRasterLock(plateRaster, 0, rwRASTERLOCKNOFETCH | rwRASTERLOCKWRITE);
    if (!lockedPlateRaster)
        return false;

    if (!pCharsetLockedData)
        return false;

    const auto plateRasterStride = RwRasterGetStride(plateRaster);
    if (!plateRasterStride)
        return false;

    const auto charsRasterStride = RwRasterGetStride(charsRaster);
    if (!charsRasterStride)
        return false;

    // Copy each character from charset raster to plate raster
    // Going from left to right

    auto plateRasterCharIter = lockedPlateRaster; // Always points to the top left corner of each character
    for (auto letter = 0; letter < MAX_TEXT_LENGTH; letter++)
    {
        unsigned int charCol, charRow;
        GetCharacterPositionInCharSet(text[letter], charCol, charRow);

        // Copy specific character from charset raster to plate raster

        // Size of a pixel (texel) in `pCharsetLockedData`. It's in 32 bit BGRA format
        constexpr auto texelSize = 4;

        // Character's top left corner in charset raster
        auto charRasterIt = &pCharsetLockedData[(CHARSET_COL_WIDTH * CHARSET_ROW_HEIGHT * charRow + CHARSET_CHAR_WIDTH * charCol) * texelSize];

        // Character's top left corner in target (plate) raster
        auto plateRasterIt = plateRasterCharIter;

        // Copy character row by row (going from top to bottom) to target (plate) raster
        for (auto r = 0u; r < CHARSET_CHAR_HEIGHT; r++)
        {
            memcpy(plateRasterIt, charRasterIt, CHARSET_CHAR_WIDTH * texelSize); // Copy row

            // Advance to next row
            plateRasterIt += plateRasterStride;
            charRasterIt += charsRasterStride;
        }

        // Advance to next character's column
        plateRasterCharIter += CHARSET_CHAR_WIDTH * texelSize;
    }

    RwRasterUnlock(plateRaster);

    return true;
}

RwTexture *PlateFeature::CCustomCarPlateMgr_CreatePlateTexture(char *text, uint8_t plateType)
{
    assert(text);
    const auto plateRaster = RwRasterCreate(256, 64, 32, rwRASTERFORMAT8888 | rwRASTERPIXELLOCKEDWRITE);
    if (!plateRaster)
        return nullptr;

    if (!RwTextureGetRaster(pCharSetTex))
    {
        RwRasterDestroy(plateRaster);
        return nullptr;
    }

    if (!CCustomCarPlateMgr_RenderLicenseplateTextToRaster(text, RwTextureGetRaster(pCharSetTex), plateRaster))
    {
        RwRasterDestroy(plateRaster);
        return nullptr;
    }

    if (const auto plateTex = RwTextureCreate(plateRaster))
    {
        RwTextureSetName(plateTex, text);
        RwTextureSetFilterMode(plateTex, rwFILTERNEAREST);
        return plateTex;
    }

    return nullptr;
}