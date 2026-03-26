#include "pch.h"
#include "plate.h"
#include <CCustomCarPlateMgr.h>
#include <CWeather.h>
#include <rwcore.h>
#include <rpworld.h>
#include <RenderWare.h>
#include <CTheZones.h>
#include "texmgr.h"
#include <utility>

using namespace plugin;

static CVehicle *pCurrentVeh = nullptr;
extern RwSurfaceProperties &gLightSurfProps;
extern RwSurfaceProperties gLightSurfPropsOff;

void LicensePlate::Initialize()
{
    m_bEnabled = true;
    // RpMaterial *__cdecl CCustomCarPlateMgr::SetupMaterialPlatebackTexture(RpMaterial *material, char plateType)
    patch::PutRetn(0x6FDE50);
    patch::ReplaceFunction(0x6FD500, (void *)CCustomCarPlateMgr_Initialise);
    patch::ReplaceFunction(0x6FD720, (void *)CCustomCarPlateMgr_Shudown);
    patch::ReplaceFunction(0x6FDEA0, (void *)CCustomCarPlateMgr_CreatePlateTexture);
}

void LicensePlate::ProcessTextures(CVehicle *pVeh, RpMaterial *pMat)
{
    if (!m_bEnabled || !pMat || !pMat->texture)
    {
        return;
    }

    pCurrentVeh = pVeh;
    if (!_stricmp("carpback", pMat->texture->name))
    {
        CCustomCarPlateMgr_SetupMaterialPlatebackTexture(pMat, -1);
    }
}

void __cdecl LicensePlate::CCustomCarPlateMgr_Shudown()
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

bool __cdecl LicensePlate::CCustomCarPlateMgr_Initialise()
{
    pCharSetTex = TextureMgr::Get("plate_char");
    RwTextureSetFilterMode(pCharSetTex, rwFILTERNEAREST);
    RwTextureSetAddressingU(pCharSetTex, rwFILTERMIPNEAREST);
    RwTextureSetAddressingV(pCharSetTex, rwFILTERMIPNEAREST);
    pCharSetTex->raster->stride = 512;

    m_Plates[DAY_CS] = TextureMgr::Get("plate_cs");
    m_Plates[DAY_LS] = TextureMgr::Get("plate_ls");
    m_Plates[DAY_LV] = TextureMgr::Get("plate_lv");
    m_Plates[DAY_SF] = TextureMgr::Get("plate_sf");

    m_Plates[NIGHT_CS] = TextureMgr::Get("plate_cs_l");
    m_Plates[NIGHT_LS] = TextureMgr::Get("plate_ls_l");
    m_Plates[NIGHT_LV] = TextureMgr::Get("plate_lv_l");
    m_Plates[NIGHT_SF] = TextureMgr::Get("plate_sf_l");

    for (int i = 0; i < ePlateType::TOTAL_SZ; i++)
    {
        strcpy(m_Plates[i]->name, "carpback\0");
        RwTextureSetAddressingU(m_Plates[i], rwFILTERMIPNEAREST);
        RwTextureSetAddressingV(m_Plates[i], rwFILTERMIPNEAREST);
        RwTextureSetFilterMode(m_Plates[i], rwFILTERLINEAR);
    }
    pCharsetLockedData = RwRasterLock(RwTextureGetRaster(pCharSetTex), 0, rwRASTERLOCKREAD);
    return pCharsetLockedData != 0;
}

RpMaterial *__cdecl LicensePlate::CCustomCarPlateMgr_SetupMaterialPlatebackTexture(RpMaterial *material, char plateType)
{
    if (plateType == -1)
    {
        VehData &data = vehData.Get(pCurrentVeh);
        if (data.cityId == -1)
        {
            data.cityId = CTheZones::m_CurrLevel;
        }
        if (data.cityId == 0)
        {
            plateType = DAY_CS;
        }
        else if (data.cityId == 1)
        {
            plateType = DAY_LS;
        }
        else if (data.cityId == 2)
        {
            plateType = DAY_SF;
        }
        else if (data.cityId == 3)
        {
            plateType = DAY_LV;
        }
    }

    if ((Util::IsNightTime() || CarUtil::IsLightsForcedOn(pCurrentVeh)) && !Util::IsEngineOff(pCurrentVeh) && !CarUtil::IsLightsForcedOff(pCurrentVeh))
    {
        material->surfaceProps.ambient = gLightSurfProps.ambient;
        RpMaterialSetTexture(material, m_Plates[plateType + 4]);
    }
    else
    {
        material->surfaceProps.ambient = gLightSurfPropsOff.ambient;
        RpMaterialSetTexture(material, m_Plates[plateType]);
    }
    return material;
}

std::pair<unsigned int, unsigned int> GetCharacterPositionInCharSet(char c)
{
    switch (c)
    {
    case '0':
        return std::make_pair(2, 6);
    case '1':
        return std::make_pair(3, 6);
    case '2':
        return std::make_pair(0, 7);
    case '3':
        return std::make_pair(1, 7);
    case '4':
        return std::make_pair(2, 7);
    case '5':
        return std::make_pair(3, 7);
    case '6':
        return std::make_pair(0, 8);
    case '7':
        return std::make_pair(1, 8);
    case '8':
        return std::make_pair(2, 8);
    case '9':
        return std::make_pair(3, 8);
    case 'A':
    case 'a':
        return std::make_pair(0, 0);
    case 'B':
    case 'b':
        return std::make_pair(1, 0);
    case 'C':
    case 'c':
        return std::make_pair(2, 0);
    case 'D':
    case 'd':
        return std::make_pair(3, 0);
    case 'E':
    case 'e':
        return std::make_pair(0, 1);
    case 'F':
    case 'f':
        return std::make_pair(1, 1);
    case 'G':
    case 'g':
        return std::make_pair(2, 1);
    case 'H':
    case 'h':
        return std::make_pair(3, 1);
    case 'I':
    case 'i':
        return std::make_pair(0, 2);
    case 'J':
    case 'j':
        return std::make_pair(1, 2);
    case 'K':
    case 'k':
        return std::make_pair(2, 2);
    case 'L':
    case 'l':
        return std::make_pair(3, 2);
    case 'M':
    case 'm':
        return std::make_pair(0, 3);
    case 'N':
    case 'n':
        return std::make_pair(1, 3);
    case 'O':
    case 'o':
        return std::make_pair(2, 3);
    case 'P':
    case 'p':
        return std::make_pair(3, 3);
    case 'Q':
    case 'q':
        return std::make_pair(0, 4);
    case 'R':
    case 'r':
        return std::make_pair(1, 4);
    case 'S':
    case 's':
        return std::make_pair(2, 4);
    case 'T':
    case 't':
        return std::make_pair(3, 4);
    case 'U':
    case 'u':
        return std::make_pair(0, 5);
    case 'V':
    case 'v':
        return std::make_pair(1, 5);
    case 'W':
    case 'w':
        return std::make_pair(2, 5);
    case 'X':
    case 'x':
        return std::make_pair(3, 5);
    case 'Y':
    case 'y':
        return std::make_pair(0, 6);
    case 'Z':
    case 'z':
        return std::make_pair(1, 6);
    default:
        return std::make_pair(0, 9);
    }
}

bool LicensePlate::CCustomCarPlateMgr_RenderLicenseplateTextToRaster(const char *text, RwRaster *charsRaster, RwRaster *plateRaster)
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
        auto t = GetCharacterPositionInCharSet(text[letter]);
        charCol = t.first;
        charRow = t.second;

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

RwTexture *LicensePlate::CCustomCarPlateMgr_CreatePlateTexture(char *text, uint8_t plateType)
{
    assert(text);

    // Create a new raster for the plate with mipmap support
    const auto plateRaster = RwRasterCreate(256, 64, 32, rwRASTERFORMAT8888 | rwRASTERPIXELLOCKEDWRITE);
    if (!plateRaster)
    {
        return nullptr;
    }

    // Ensure the charset texture is valid before proceeding
    if (!RwTextureGetRaster(pCharSetTex))
    {
        RwRasterDestroy(plateRaster);
        return nullptr;
    }

    // Render the license plate text to the raster
    if (!CCustomCarPlateMgr_RenderLicenseplateTextToRaster(text, RwTextureGetRaster(pCharSetTex), plateRaster))
    {
        RwRasterDestroy(plateRaster);
        return nullptr;
    }

    // Create a texture from the raster
    if (const auto plateTex = RwTextureCreate(plateRaster))
    {
        // Set the texture name and filter mode
        RwTextureSetName(plateTex, text);
        RwTextureSetFilterMode(plateTex, rwFILTERNEAREST);
        return plateTex;
    }

    // Cleanup if texture creation fails
    RwRasterDestroy(plateRaster);
    return nullptr;
}