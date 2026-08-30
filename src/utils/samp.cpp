#include "pch.h"
#include "utils/samp.h"
#include <windows.h>
#include <cctype>
#include <string_view>

namespace SAMP
{
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

        uintptr_t GetVehiclePool()
        {
            if (!IsPresent()) return 0;

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

        const char *FindPlateTextSEH(uintptr_t pVehPool, CVehicle *pGameVeh)
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

    bool IsPresent()
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

    bool IsInputActive()
    {
        if (!IsPresent()) return false;

        __try
        {
            constexpr uintptr_t inputOffsets[] = { 0x26EB84, 0x26E8CC, 0x21A0E8, 0x2ACA14 };
            for (uintptr_t off : inputOffsets)
            {
                uintptr_t pInputAddr = s_sampBase + off;
                if (IsValidPtr(reinterpret_cast<const void *>(pInputAddr)))
                {
                    uintptr_t pInput = *reinterpret_cast<const uintptr_t *>(pInputAddr);
                    if (pInput && IsValidPtr(reinterpret_cast<const void *>(pInput + 0x8)))
                    {
                        int enabled = *reinterpret_cast<const int *>(pInput + 0x8);
                        if (enabled != 0) return true;
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
        return false;
    }

    std::string GetVehiclePlateText(CVehicle *pGameVeh)
    {
        if (!pGameVeh) return "";
        uintptr_t pVehPool = GetVehiclePool();
        if (!pVehPool) return "";
        const char *szPlate = FindPlateTextSEH(pVehPool, pGameVeh);
        if (!szPlate || szPlate[0] == '\0') return "";
        return SanitizeAndFormatPlateText(szPlate);
    }
}
