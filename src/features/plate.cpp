#include "pch.h"
#include "plate.h"
#include <CCustomCarPlateMgr.h>
#include <CWeather.h>
#include <CTheZones.h>
#include "utils/texmgr.h"
#include "utils/modelinfomgr.h"
#include "utils/samp.h"
#include "utils/car.h"
#include "utils/util.h"
#include "utils/datamgr.h"
#include <utility>
#include <string>
#include <string_view>
#include <cctype>
#include <optional>

using namespace plugin;

static CVehicle *pCurrentVeh = nullptr;

static const nlohmann::json *FindPlateJson(int modelIndex)
{
    if (!DataMgr::Has(modelIndex)) return nullptr;
    const auto &json = DataMgr::Get(modelIndex);
    if (json.contains("plate")) return &json["plate"];
    if (json.contains("license_plate")) return &json["license_plate"];
    if (json.contains("lights"))
    {
        const auto &lights = json["lights"];
        if (lights.contains("plate")) return &lights["plate"];
        if (lights.contains("license_plate")) return &lights["license_plate"];
    }
    return nullptr;
}

static std::optional<CRGBA> ParsePlateColor(const nlohmann::json &sec, const char *key)
{
    const nlohmann::json *val = nullptr;
    if (sec.contains(key))
    {
        val = &sec[key];
    }
    else if (sec.contains("material") && sec["material"].contains(key))
    {
        val = &sec["material"][key];
    }
    if (!val) return std::nullopt;

    if (val->is_array() && val->size() >= 3)
    {
        return CRGBA((*val)[0].get<uint8_t>(), (*val)[1].get<uint8_t>(), (*val)[2].get<uint8_t>(),
                     val->size() >= 4 ? (*val)[3].get<uint8_t>() : 255);
    }
    if (val->is_object())
    {
        uint8_t r = val->value("red", val->value("r", 255));
        uint8_t g = val->value("green", val->value("g", 255));
        uint8_t b = val->value("blue", val->value("b", 255));
        uint8_t a = val->value("alpha", val->value("a", 255));
        return CRGBA(r, g, b, a);
    }
    return std::nullopt;
}

void LicensePlate::ReloadConfig()
{
    CBaseFeature::ReloadConfig();
    m_bEnabled = m_bActive;
}

void LicensePlate::Init()
{
    ReloadConfig();
    if (!m_bEnabled)
    {
        return;
    }
    // RpMaterial *__cdecl CCustomCarPlateMgr::SetupMaterialPlatebackTexture(RpMaterial *material, char plateType)
    patch::PutRetn(0x6FDE50);
    patch::ReplaceFunction(0x6FD500, (void *)CCustomCarPlateMgr_Initialise);
    patch::ReplaceFunction(0x6FD720, (void *)CCustomCarPlateMgr_Shudown);
    patch::ReplaceFunction(0x6FDEA0, (void *)CCustomCarPlateMgr_CreatePlateTexture);

    Events::vehicleDtorEvent += [](CVehicle *pVeh) {
        if (pCurrentVeh == pVeh)
        {
            pCurrentVeh = nullptr;
        }
    };
}

void LicensePlate::ProcessTextures(CVehicle *pVeh, RpMaterial *pMat)
{
    if (!pVeh || !pMat || !pMat->texture || !pMat->texture->name)
    {
        return;
    }

    pCurrentVeh = pVeh;
    const char *texName = pMat->texture->name;
    PlateData &data = m_VehData.Get(pVeh);

    bool isPlateMat = !_stricmp("carpback", texName) ||
                      !_stricmp("carplate", texName) ||
                      !_strnicmp(texName, "plate_", 6) ||
                      (data.m_pCustomPlateTex && pMat->texture == data.m_pCustomPlateTex) ||
                      (!data.m_szLastPlateText.empty() && !_stricmp(data.m_szLastPlateText.c_str(), texName));

    if (!isPlateMat && pMat->texture->raster)
    {
        RwRaster *r = pMat->texture->raster;
        if (r->width == 256 && r->height == 64)
        {
            isPlateMat = true;
        }
    }

    if (m_bEnabled)
    {
        if (SAMP::IsPresent())
        {
            bool isPlateTextMat = !_stricmp("carplate", texName) ||
                                  (data.m_pCustomPlateTex && pMat->texture == data.m_pCustomPlateTex) ||
                                  (!data.m_szLastPlateText.empty() && !_stricmp(data.m_szLastPlateText.c_str(), texName));

            if (!isPlateTextMat && pMat->texture->raster)
            {
                RwRaster *r = pMat->texture->raster;
                if (r->width == 256 && r->height == 64 &&
                    strncmp(texName, "plate_", 6) != 0 && _stricmp(texName, "carpback") != 0)
                {
                    isPlateTextMat = true;
                }
            }

            if (isPlateTextMat)
            {
                std::string formatted = SAMP::GetVehiclePlateText(pVeh);
                if (!formatted.empty())
                {
                    if (data.m_szLastPlateText != formatted || !data.m_pCustomPlateTex)
                    {
                        if (data.m_pCustomPlateTex)
                        {
                            RwTextureDestroy(data.m_pCustomPlateTex);
                            data.m_pCustomPlateTex = nullptr;
                        }
                        data.m_pCustomPlateTex = CCustomCarPlateMgr_CreatePlateTexture(formatted.data(), 0);
                        data.m_szLastPlateText = formatted;
                    }

                    if (data.m_pCustomPlateTex)
                    {
                        RpMaterialSetTexture(pMat, data.m_pCustomPlateTex);
                    }
                }
            }
        }

        if (!_stricmp("carpback", texName))
        {
            CCustomCarPlateMgr_SetupMaterialPlatebackTexture(pMat, -1);
        }
    }

    if (isPlateMat)
    {
        if (const auto *pSec = FindPlateJson(pVeh->m_nModelIndex))
        {
            auto onCol = ParsePlateColor(*pSec, "color");
            auto offCol = ParsePlateColor(*pSec, "color_off");

            if (onCol || offCol)
            {
                bool lightsOn = CarUtil::AreLightsOn(pVeh);

                CRGBA targetCol;
                if (lightsOn)
                {
                    targetCol = onCol.value_or(offCol.value_or(CRGBA(255, 255, 255, 255)));
                }
                else
                {
                    targetCol = offCol.value_or(onCol.value_or(CRGBA(255, 255, 255, 255)));
                }

                RwRGBA *pColor = RpMaterialGetColor(pMat);
                ModelInfoMgr::RegisterRestore(pColor, *reinterpret_cast<void **>(pColor));
                pColor->red = targetCol.r;
                pColor->green = targetCol.g;
                pColor->blue = targetCol.b;
            }
        }
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
    RwTextureSetFilterMode(pCharSetTex, rwFILTERLINEAR);
    RwTextureSetAddressingU(pCharSetTex, rwTEXTUREADDRESSCLAMP);
    RwTextureSetAddressingV(pCharSetTex, rwTEXTUREADDRESSCLAMP);
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
        if (m_Plates[i])
        {
            RwTextureSetName(m_Plates[i], "carpback");
            RwTextureSetAddressingU(m_Plates[i], rwTEXTUREADDRESSCLAMP);
            RwTextureSetAddressingV(m_Plates[i], rwTEXTUREADDRESSCLAMP);
            RwTextureSetFilterMode(m_Plates[i], rwFILTERLINEAR);
        }
    }
    pCharsetLockedData = RwRasterLock(RwTextureGetRaster(pCharSetTex), 0, rwRASTERLOCKREAD);
    return pCharsetLockedData != 0;
}

RpMaterial *__cdecl LicensePlate::CCustomCarPlateMgr_SetupMaterialPlatebackTexture(RpMaterial *material, char plateType)
{
    if (plateType == -1)
    {
        if (!pCurrentVeh)
        {
            return material;
        }
        PlateData &data = m_VehData.Get(pCurrentVeh);
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
        else
        {
            plateType = DAY_LS;
        }
    }

    bool lightsOn = CarUtil::AreLightsOn(pCurrentVeh);
    if (pCurrentVeh->m_fHealth > 0.0f && lightsOn)
    {
        ModelInfoMgr::RegisterRestoreSurfProps(material);
        material->surfaceProps = *reinterpret_cast<RwSurfaceProperties *>(0x8A645C);
        if (plateType + 4 < ePlateType::TOTAL_SZ && m_Plates[plateType + 4])
            RpMaterialSetTexture(material, m_Plates[plateType + 4]);
    }
    else
    {
        if (plateType >= 0 && plateType < ePlateType::TOTAL_SZ && m_Plates[plateType])
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

    if (!pCharsetLockedData)
        return false;

    const auto lockedPlateRaster = RwRasterLock(plateRaster, 0, rwRASTERLOCKNOFETCH | rwRASTERLOCKWRITE);
    if (!lockedPlateRaster)
        return false;

    const auto plateRasterStride = RwRasterGetStride(plateRaster);
    if (!plateRasterStride)
    {
        RwRasterUnlock(plateRaster);
        return false;
    }

    const auto charsRasterStride = RwRasterGetStride(charsRaster);
    if (!charsRasterStride)
    {
        RwRasterUnlock(plateRaster);
        return false;
    }

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

    // Create a new raster for the plate
    const auto plateRaster = RwRasterCreate(256, 64, 32, rwRASTERFORMAT8888 | rwRASTERPIXELLOCKEDWRITE);
    if (!plateRaster)
    {
        return nullptr;
    }

    // Ensure the charset texture is valid before proceeding
    if (!pCharSetTex || !RwTextureGetRaster(pCharSetTex))
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
        RwTextureSetFilterMode(plateTex, rwFILTERLINEAR);
        return plateTex;
    }

    // Cleanup if texture creation fails
    RwRasterDestroy(plateRaster);
    return nullptr;
}