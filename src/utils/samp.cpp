#include "pch.h"
#include "utils/samp.h"
#include <windows.h>
#include <cctype>
#include <string_view>

namespace SAMP
{
    namespace
    {
        struct Offsets
        {
            uintptr_t netGame; // Offset to CNetGame*
            uintptr_t pools;   // Offset to m_pPools in CNetGame
            uintptr_t vehPool; // Offset to m_pVehicle in Pools
            uintptr_t input;   // Offset to CInput*
            uintptr_t dialog;  // Offset to CDialog*
        };

        uintptr_t s_baseAddr = 0;
        const Offsets *s_offsets = nullptr;
        bool s_initialized = false;

        const Offsets *ResolveOffsets(HMODULE hMod)
        {
            if (!hMod) return nullptr;
            auto *dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(hMod);
            if (dos->e_magic != IMAGE_DOS_SIGNATURE) return nullptr;
            auto *nt = reinterpret_cast<const IMAGE_NT_HEADERS32 *>(reinterpret_cast<const uint8_t *>(hMod) + dos->e_lfanew);
            if (nt->Signature != IMAGE_NT_SIGNATURE) return nullptr;

            switch (nt->OptionalHeader.AddressOfEntryPoint)
            {
                // 0.3.7-R1
                case 0x31DF13: { static constexpr Offsets r1 = { 0x21A0F8, 0x3CD, 0x1C, 0x21A0E8, 0x21A0B8 }; return &r1; }
                // 0.3.7-R3 and R3-1
                case 0x0CC490:
                case 0x0CC4D0: { static constexpr Offsets r3 = { 0x26E8DC, 0x3DE, 0x0C, 0x26E8CC, 0x26E898 }; return &r3; }
                // 0.3.7-R5
                case 0x0CBC90: { static constexpr Offsets r5 = { 0x26EB94, 0x3DE, 0x00, 0x26EB84, 0x26EB50 }; return &r5; }
                // 0.3.DL-1
                case 0x0FDB60: { static constexpr Offsets dl = { 0x2ACA24, 0x3DE, 0x0C, 0x2ACA14, 0x2AC9E0 }; return &dl; }
                default:       return nullptr;
            }
        }

        void Init()
        {
            if (s_initialized) return;
            s_initialized = true;

            HMODULE hMod = GetModuleHandleA("samp.dll");
            if (!hMod) return;

            s_baseAddr = reinterpret_cast<uintptr_t>(hMod);
            s_offsets = ResolveOffsets(hMod);
        }

        uintptr_t GetVehiclePool()
        {
            if (!s_baseAddr || !s_offsets) return 0;
            __try
            {
                uintptr_t pNetGame = *reinterpret_cast<const uintptr_t *>(s_baseAddr + s_offsets->netGame);
                if (!pNetGame) return 0;
                uintptr_t pPools = *reinterpret_cast<const uintptr_t *>(pNetGame + s_offsets->pools);
                if (!pPools) return 0;
                return *reinterpret_cast<const uintptr_t *>(pPools + s_offsets->vehPool);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return 0;
            }
        }

        std::string SanitizeAndFormatPlateText(std::string_view raw)
        {
            std::string clean;
            for (size_t i = 0; i < raw.size(); ++i)
            {
                if (raw[i] == '{' && raw.find('}', i) != std::string_view::npos)
                {
                    i = raw.find('}', i);
                }
                else if (raw[i] == '~' && i + 2 < raw.size() && raw[i + 2] == '~')
                {
                    i += 2;
                }
                else if (static_cast<unsigned char>(raw[i]) >= 32 && static_cast<unsigned char>(raw[i]) <= 126)
                {
                    clean += static_cast<char>(std::toupper(static_cast<unsigned char>(raw[i])));
                }
            }

            size_t s = clean.find_first_not_of(" \t\r\n");
            size_t e = clean.find_last_not_of(" \t\r\n");
            if (s == std::string::npos || (clean = clean.substr(s, e - s + 1)) == "XYZSR998")
            {
                return "";
            }

            if (clean.size() > 8) clean.resize(8);
            size_t pad = (8 - clean.size()) / 2;
            return std::string(pad, ' ') + clean + std::string(8 - clean.size() - pad, ' ');
        }
    }

    bool IsPresent()
    {
        Init();
        return s_offsets != nullptr;
    }

    bool IsInputActive()
    {
        if (!s_baseAddr || !s_offsets) return false;
        __try
        {
            uintptr_t pInput = *reinterpret_cast<const uintptr_t *>(s_baseAddr + s_offsets->input);
            if (pInput && *reinterpret_cast<const int *>(pInput + 0x14E0) != 0) return true;

            uintptr_t pDialog = *reinterpret_cast<const uintptr_t *>(s_baseAddr + s_offsets->dialog);
            if (pDialog && *reinterpret_cast<const int *>(pDialog + 0x28) != 0) return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
        return false;
    }

    namespace
    {
        const char *FindPlateText(uintptr_t pool, CVehicle *pGameVeh)
        {
            __try
            {
                auto pGameObjects = reinterpret_cast<CVehicle **>(pool + 0x4FB4);
                auto pObjects     = reinterpret_cast<uintptr_t *>(pool + 0x1134);
                auto pNotEmpty    = reinterpret_cast<const int *>(pool + 0x3074);

                for (size_t i = 0; i < 2000; ++i)
                {
                    if (pNotEmpty[i] && pGameObjects[i] == pGameVeh && pObjects[i])
                    {
                        const char *text = reinterpret_cast<const char *>(pObjects[i] + 0x93);
                        return (text && text[0] != '\0') ? text : nullptr;
                    }
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return nullptr;
            }
            return nullptr;
        }
    }

    std::string GetVehiclePlateText(CVehicle *pGameVeh)
    {
        uintptr_t pool = GetVehiclePool();
        if (!pool || !pGameVeh) return "";

        const char *text = FindPlateText(pool, pGameVeh);
        return text ? SanitizeAndFormatPlateText(text) : "";
    }

    void PatchVehicleLights()
    {
        if (!IsPresent()) return;

        static bool s_patched = false;
        if (s_patched) return;
        s_patched = true;

        // Apply the exact 7 NOP patches that SA-MP normally applies at 0xAA800 (0.3.DL)
        // inside CVehicle::DoVehicleLights when ManualVehicleEngineAndLights() is enabled.
        patch::Nop(0x6E1BA0, 6);
        patch::Nop(0x6E1BB1, 6);
        patch::Nop(0x6E1BD2, 7);
        patch::Nop(0x6E1BE3, 0x25);
        patch::Nop(0x6E1C38, 8);
        patch::Nop(0x6E1D98, 0xF);
        patch::Nop(0x6E1DBC, 8);
    }
}

