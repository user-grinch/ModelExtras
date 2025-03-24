#include "pch.h"
#include "plate.h"
#include <CCustomCarPlateMgr.h>
#include <CWeather.h>
#include <rwcore.h>
#include <rpworld.h>
#include <RenderWare.h>
#include "defines.h"

static CVehicle *pCurrentVeh = nullptr;
PlateFeature LicensePlate;
extern bool IsNightTime();
extern bool IsEngineOff(CVehicle *pVeh);

void PlateFeature::Initialize()
{
    // RpMaterial *__cdecl CCustomCarPlateMgr::SetupMaterialPlatebackTexture(RpMaterial *material, char plateType)
    plugin::patch::PutRetn(0x6FDE50);
    plugin::patch::ReplaceFunction(0x6FD500, CCustomCarPlateMgr_Initialise);
    plugin::patch::ReplaceFunction(0x6FD720, CCustomCarPlateMgr_Shudown);
    plugin::patch::ReplaceFunction(0x6FDEA0, CCustomCarPlateMgr_CreatePlateTexture);

    plugin::Events::vehicleRenderEvent += [](CVehicle *pVeh)
    {
        pCurrentVeh = pVeh;
        RpClumpForAllAtomics(pVeh->m_pRwClump, [](RpAtomic *atomic, void *data)
                             {
        if (atomic->geometry) {
            RpGeometryForAllMaterials(atomic->geometry, [](RpMaterial *mat, void *data) {
                if (mat && mat->texture) {
                    if ( !_stricmp("carpback", mat->texture->name) )
                    {
                        CCustomCarPlateMgr_SetupMaterialPlatebackTexture(mat, -1);
                    }
                }
                return mat;
            }, NULL);
        }
        return atomic; }, NULL);
    };
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
    m_Plates[DAY_LS] = Util::LoadDDSTextureCB(dirPath, "plateback3");
    m_Plates[DAY_LV] = Util::LoadDDSTextureCB(dirPath, "plateback2");
    m_Plates[DAY_SF] = Util::LoadDDSTextureCB(dirPath, "plateback1");

    m_Plates[NIGHT_LS] = Util::LoadDDSTextureCB(dirPath, "plateback3_l");
    m_Plates[NIGHT_LV] = Util::LoadDDSTextureCB(dirPath, "plateback2_l");
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
        if (CWeather::WeatherRegion == WEATHER_REGION_LA)
            plateType = DAY_LS;
        else if (CWeather::WeatherRegion == WEATHER_REGION_SF)
            plateType = DAY_SF;
        else
            plateType = DAY_LV;
    }

    if (IsNightTime() && !IsEngineOff(pCurrentVeh) && pCurrentVeh->m_pDriver)
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