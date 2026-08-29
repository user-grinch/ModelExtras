#include "pch.h"
#include "plate.h"
#include <CCustomCarPlateMgr.h>
#include <CWeather.h>
#include <rwcore.h>
#include <rpworld.h>
#include <RenderWare.h>
#include <CTheZones.h>
#include "utils/texmgr.h"
#include <utility>
#include <string>
#include <string_view>
#include <cctype>

using namespace plugin;

static CVehicle *pCurrentVeh = nullptr;
extern RwSurfaceProperties gLightSurfProps;
extern RwSurfaceProperties gLightSurfPropsOff;

namespace
{
    uintptr_t s_sampBase = 0;
    uintptr_t s_pCachedVehPool = 0;

    struct SAMPVersionOffset
    {
        uintptr_t netGame;
        uintptr_t pools;
        uintptr_t vehPool;
    };

    constexpr SAMPVersionOffset s_SampOffsets[] = {
        { 0x26EB94, 0x3DA, 0x00 }, // 0.3.7-R5 / open.mp
        { 0x26E8DC, 0x3DE, 0x0C }, // 0.3.7-R3
        { 0x21A0F8, 0x3CD, 0x1C }, // 0.3.7-R1
        { 0x2ACA24, 0x3DE, 0x0C }, // 0.3.DL
        { 0x26EA0C, 0x3DE, 0x0C }, // 0.3.7-R4
        { 0x21A100, 0x3CD, 0x1C }  // 0.3.7-R2
    };

    bool IsValidPtr(const void *ptr, size_t size = 4)
    {
        if (!ptr || reinterpret_cast<uintptr_t>(ptr) < 0x10000) return false;
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(ptr, &mbi, sizeof(mbi)) == sizeof(mbi))
        {
            return (mbi.State == MEM_COMMIT) && !(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD));
        }
        return false;
    }

    bool IsSAMPRunning()
    {
        if (s_sampBase == 0)
        {
            HMODULE hMod = GetModuleHandleA("samp.dll");
            if (!hMod) hMod = GetModuleHandleA("omp-client.dll");
            if (!hMod) hMod = GetModuleHandleA("openmp.dll");
            if (!hMod) hMod = GetModuleHandleA("omp.dll");
            if (hMod) s_sampBase = reinterpret_cast<uintptr_t>(hMod);
        }
        return s_sampBase != 0;
    }

    uintptr_t TryFindVehiclePoolSEH(uintptr_t base)
    {
        __try
        {
            for (const auto &v : s_SampOffsets)
            {
                uintptr_t pNetGameAddr = base + v.netGame;
                if (!IsValidPtr(reinterpret_cast<const void *>(pNetGameAddr))) continue;

                uintptr_t pNetGame = *reinterpret_cast<const uintptr_t *>(pNetGameAddr);
                if (!pNetGame || !IsValidPtr(reinterpret_cast<const void *>(pNetGame + v.pools))) continue;

                uintptr_t pPools = *reinterpret_cast<const uintptr_t *>(pNetGame + v.pools);
                if (!pPools || !IsValidPtr(reinterpret_cast<const void *>(pPools + v.vehPool))) continue;

                uintptr_t pVehPool = *reinterpret_cast<const uintptr_t *>(pPools + v.vehPool);
                if (pVehPool && IsValidPtr(reinterpret_cast<const void *>(pVehPool + 0x1134)) &&
                    IsValidPtr(reinterpret_cast<const void *>(pVehPool + 0x4FB4)))
                {
                    return pVehPool;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return 0;
        }
        return 0;
    }

    uintptr_t GetSAMPVehiclePool()
    {
        if (!IsSAMPRunning()) return 0;

        if (s_pCachedVehPool != 0)
        {
            if (IsValidPtr(reinterpret_cast<const void *>(s_pCachedVehPool + 0x1134)))
            {
                return s_pCachedVehPool;
            }
            s_pCachedVehPool = 0;
        }

        uintptr_t pool = TryFindVehiclePoolSEH(s_sampBase);
        if (pool != 0)
        {
            s_pCachedVehPool = pool;
            return pool;
        }
        return 0;
    }

    const char *FindSAMPPlateTextSEH(uintptr_t pVehPool, CVehicle *pGameVeh)
    {
        __try
        {
            auto pGameObjects = reinterpret_cast<CVehicle **>(pVehPool + 0x4FB4); // m_pGameObject[2000]
            auto pObjects = reinterpret_cast<uintptr_t *>(pVehPool + 0x1134);     // m_pObject[2000]
            auto pNotEmpty = reinterpret_cast<int *>(pVehPool + 0x3074);          // m_bNotEmpty[2000]

            for (size_t i = 0; i < 2000; ++i)
            {
                if (pNotEmpty[i])
                {
                    bool match = (pGameObjects[i] == pGameVeh);
                    uintptr_t pSampVeh = pObjects[i];

                    if (!match && pSampVeh && IsValidPtr(reinterpret_cast<const void *>(pSampVeh + 0x4C)))
                    {
                        match = (*reinterpret_cast<CVehicle **>(pSampVeh + 0x4C) == pGameVeh);
                    }

                    if (match && pSampVeh && IsValidPtr(reinterpret_cast<const void *>(pSampVeh + 0x93), 33))
                    {
                        const char *szText = reinterpret_cast<const char *>(pSampVeh + 0x93);
                        if (szText && szText[0] != '\0')
                        {
                            return szText;
                        }
                        return nullptr;
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return nullptr;
        }
        return nullptr;
    }

    const char *GetSAMPVehiclePlateText(CVehicle *pGameVeh)
    {
        if (!pGameVeh) return nullptr;
        uintptr_t pVehPool = GetSAMPVehiclePool();
        if (!pVehPool) return nullptr;
        return FindSAMPPlateTextSEH(pVehPool, pGameVeh);
    }

    std::string SanitizeAndFormatPlateText(std::string_view rawText)
    {
        std::string clean;
        clean.reserve(rawText.size());

        for (size_t i = 0; i < rawText.size(); ++i)
        {
            if (rawText[i] == '{')
            {
                size_t close = rawText.find('}', i);
                if (close != std::string_view::npos && (close - i == 7 || close - i == 9))
                {
                    i = close;
                    continue;
                }
            }
            if (rawText[i] == '~' && i + 2 < rawText.size() && rawText[i + 2] == '~')
            {
                i += 2;
                continue;
            }
            unsigned char c = static_cast<unsigned char>(rawText[i]);
            if (c >= 32 && c <= 126)
            {
                clean.push_back(static_cast<char>(std::toupper(c)));
            }
        }

        size_t start = clean.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = clean.find_last_not_of(" \t\r\n");
        clean = clean.substr(start, end - start + 1);

        if (clean.empty() || clean == "XYZSR998") return "";

        if (clean.size() > 8) clean = clean.substr(0, 8);

        size_t len = clean.size();
        size_t rem = 8 - len;
        size_t left = rem / 2;
        size_t right = rem - left;

        std::string formatted;
        formatted.reserve(8);
        formatted.append(left, ' ');
        formatted.append(clean);
        formatted.append(right, ' ');
        return formatted;
    }
}

void LicensePlate::Init()
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
    if (!m_bEnabled || !pVeh || !pMat || !pMat->texture || !pMat->texture->name)
    {
        return;
    }

    pCurrentVeh = pVeh;
    const char *texName = pMat->texture->name;

    if (IsSAMPRunning())
    {
        PlateData &data = m_VehData.Get(pVeh);

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
            const char *szSampPlate = GetSAMPVehiclePlateText(pVeh);
            if (szSampPlate && szSampPlate[0] != '\0')
            {
                std::string formatted = SanitizeAndFormatPlateText(szSampPlate);
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
    }

    if (!_stricmp("carpback", texName))
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
    RwTextureSetFilterMode(pCharSetTex, rwFILTERLINEAR);
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
        if (m_Plates[i])
        {
            RwTextureSetName(m_Plates[i], "carpback");
            RwTextureSetAddressingU(m_Plates[i], rwFILTERMIPNEAREST);
            RwTextureSetAddressingV(m_Plates[i], rwFILTERMIPNEAREST);
            if (RwTextureGetRaster(m_Plates[i]))
            {
                RwTextureRasterGenerateMipmaps(RwTextureGetRaster(m_Plates[i]), nullptr);
            }
            RwTextureSetFilterMode(m_Plates[i], rwFILTERLINEARMIPLINEAR);
        }
    }
    pCharsetLockedData = RwRasterLock(RwTextureGetRaster(pCharSetTex), 0, rwRASTERLOCKREAD);
    return pCharsetLockedData != 0;
}

RpMaterial *__cdecl LicensePlate::CCustomCarPlateMgr_SetupMaterialPlatebackTexture(RpMaterial *material, char plateType)
{
    if (plateType == -1)
    {
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

    bool isBike = pCurrentVeh->m_nVehicleSubClass == VEHICLE_BIKE;
    bool lightsOn = (pCurrentVeh->bLightsOn || CarUtil::IsLightsForcedOn(pCurrentVeh) || (Util::IsNightTime() && !Util::IsEngineOff(pCurrentVeh)) || (isBike && !Util::IsEngineOff(pCurrentVeh))) && !CarUtil::IsLightsForcedOff(pCurrentVeh);
    if (pCurrentVeh->m_fHealth > 0.0f && lightsOn)
    {
        material->surfaceProps = {50.0f, 0.0f, 0.0f};
        if (plateType + 4 < ePlateType::TOTAL_SZ && m_Plates[plateType + 4])
            RpMaterialSetTexture(material, m_Plates[plateType + 4]);
    }
    else
    {
        material->surfaceProps = gLightSurfPropsOff;
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

    // Create a new raster for the plate with mipmap support
    const auto plateRaster = RwRasterCreate(256, 64, 32, rwRASTERFORMAT8888 | rwRASTERFORMATMIPMAP | rwRASTERFORMATAUTOMIPMAP | rwRASTERPIXELLOCKEDWRITE);
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
        RwTextureRasterGenerateMipmaps(plateRaster, nullptr);
        RwTextureSetFilterMode(plateTex, rwFILTERLINEARMIPLINEAR);
        return plateTex;
    }

    // Cleanup if texture creation fails
    RwRasterDestroy(plateRaster);
    return nullptr;
}