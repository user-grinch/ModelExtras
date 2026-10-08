#include "pch.h"
#include "neon.h"
#include "features/carcols.h"
#include "features/lights/data.h"
#include "features/lights/manager.h"
#include "utils/datamgr.h"
#include "utils/modelinfomgr.h"
#include "utils/texmgr.h"
#include "utils/car.h"
#include "utils/mathutil.h"
#include "utils/render.h"
#include <CShadows.h>
#include <CModelInfo.h>
#include <CVehicleModelInfo.h>
#include <CBike.h>
#include <CCamera.h>
#include <CTimer.h>
#include <charconv>

enum class NeonMode {
    Static,
    Rainbow,
    VehicleAll,
    VehiclePrimary,
    VehicleSecondary,
    VehicleTertiary,
    VehicleQuaternary
};

struct VehColorList {
    CRGBA colors[4];
    int count = 0;
};

static CRGBA RainbowColor(float hue)
{
    float r = std::clamp(std::abs(hue * 6.0f - 3.0f) - 1.0f, 0.0f, 1.0f);
    float g = std::clamp(2.0f - std::abs(hue * 6.0f - 2.0f), 0.0f, 1.0f);
    float b = std::clamp(2.0f - std::abs(hue * 6.0f - 4.0f), 0.0f, 1.0f);
    return CRGBA(static_cast<uint8_t>(r * 255.0f),
                 static_cast<uint8_t>(g * 255.0f),
                 static_cast<uint8_t>(b * 255.0f),
                 255);
}

static CRGBA LerpColor(const CRGBA &c1, const CRGBA &c2, float t)
{
    return CRGBA(
        static_cast<uint8_t>(c1.r + (c2.r - c1.r) * t),
        static_cast<uint8_t>(c1.g + (c2.g - c1.g) * t),
        static_cast<uint8_t>(c1.b + (c2.b - c1.b) * t),
        static_cast<uint8_t>(c1.a + (c2.a - c1.a) * t)
    );
}

static CRGBA GetVehicleSlotColor(CVehicle *pVeh, int slot)
{
    CRGBA *colorTable = *reinterpret_cast<CRGBA **>(0x4C8390);
    uint8_t idx = 0;
    if (slot == 0) idx = pVeh->m_nPrimaryColor;
    else if (slot == 1) idx = pVeh->m_nSecondaryColor;
    else if (slot == 2) idx = pVeh->m_nTertiaryColor;
    else if (slot == 3) idx = pVeh->m_nQuaternaryColor;

    CRGBA col = (colorTable && idx < 256) ? colorTable[idx] : CRGBA(255, 255, 255, 255);
    col.a = 255;

    auto &cData = Carcols::m_VehData.Get(pVeh);
    if (slot == 0 && cData.m_bPri) col = cData.m_Colors.primary;
    else if (slot == 1 && cData.m_bSec) col = cData.m_Colors.secondary;
    else if (slot == 2 && cData.m_bTer) col = cData.m_Colors.tert;
    else if (slot == 3 && cData.m_bQuat) col = cData.m_Colors.quart;

    return col;
}

static VehColorList GetVehicleColors(CVehicle *pVeh)
{
    VehColorList list;
    for (int i = 0; i < 4; ++i)
    {
        CRGBA c = GetVehicleSlotColor(pVeh, i);
        bool duplicate = false;
        for (int j = 0; j < list.count; ++j)
        {
            if (list.colors[j].r == c.r && list.colors[j].g == c.g && list.colors[j].b == c.b)
            {
                duplicate = true;
                break;
            }
        }
        if (!duplicate)
        {
            list.colors[list.count++] = c;
        }
    }
    if (list.count == 0)
    {
        list.colors[0] = CRGBA(255, 255, 255, 255);
        list.count = 1;
    }
    return list;
}

static NeonMode ParseMode(std::string_view s)
{
    if (s == "rainbow" || s == "RAINBOW") return NeonMode::Rainbow;
    if (s == "vehicle" || s == "VEHICLE" || s == "car" || s == "carcols" || s == "dual" || s == "both" || s == "all") return NeonMode::VehicleAll;
    if (s == "primary" || s == "PRIMARY" || s == "body" || s == "main") return NeonMode::VehiclePrimary;
    if (s == "secondary" || s == "SECONDARY" || s == "roof" || s == "stripe") return NeonMode::VehicleSecondary;
    if (s == "tertiary" || s == "TERTIARY") return NeonMode::VehicleTertiary;
    if (s == "quaternary" || s == "QUATERNARY") return NeonMode::VehicleQuaternary;
    return NeonMode::Static;
}

static bool ParseColor(const nlohmann::json &val, CRGBA &col)
{
    if (val.is_string())
    {
        std::string_view s = val.get<std::string_view>();
        if (s.starts_with('#')) s.remove_prefix(1);
        else if (s.starts_with("0x") || s.starts_with("0X")) s.remove_prefix(2);

        if (s.length() == 3 || s.length() == 4)
        {
            auto hexNibble = [](char c) -> int {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                return -1;
            };
            int r = hexNibble(s[0]), g = hexNibble(s[1]), b = hexNibble(s[2]);
            if (r >= 0 && g >= 0 && b >= 0)
            {
                int a = (s.length() == 4) ? hexNibble(s[3]) : 15;
                if (a >= 0)
                {
                    col = CRGBA(static_cast<uint8_t>(r * 17), static_cast<uint8_t>(g * 17),
                                static_cast<uint8_t>(b * 17), static_cast<uint8_t>(a * 17));
                    return true;
                }
            }
        }
        else if (s.length() == 6 || s.length() == 8)
        {
            unsigned int hex = 0;
            auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), hex, 16);
            if (ec == std::errc())
            {
                col = (s.length() == 6)
                    ? CRGBA((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, 255)
                    : CRGBA((hex >> 24) & 0xFF, (hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF);
                return true;
            }
        }
    }
    else if (val.is_array() && val.size() >= 3)
    {
        col = CRGBA(val[0].get<uint8_t>(), val[1].get<uint8_t>(), val[2].get<uint8_t>(),
                    val.size() >= 4 ? val[3].get<uint8_t>() : 255);
        return true;
    }
    else if (val.is_object())
    {
        col = CRGBA(val.value("r", val.value("red", 255)),
                    val.value("g", val.value("green", 255)),
                    val.value("b", val.value("blue", 255)),
                    val.value("a", val.value("alpha", 255)));
        return true;
    }
    return false;
}

void Neon::Init()
{
    ModelInfoMgr::RegisterRender([](CVehicle *pVeh)
    {
        if (!CBaseFeature::IsEnabled(eFeatureMatrix::Neon)) return;
        if (!pVeh || pVeh->m_nType != ENTITY_TYPE_VEHICLE || !pVeh->m_pRwClump) return;
        if (pVeh->m_fHealth <= 0.0f || pVeh->IsUpsideDown() || pVeh->bSubmergedInWater) return;

        bool isLightsActive = CarUtil::AreLightsOn(pVeh);
        if (!isLightsActive)
        {
            auto &lData = LightManager::m_VehData.Get(pVeh);
            if (lData.bLongLightsOn || lData.bFogLightsOn)
            {
                isLightsActive = true;
            }
        }
        if (!isLightsActive) return;

        const auto *pJson = DataMgr::Find(pVeh->m_nModelIndex);
        if (!pJson || !pJson->contains("neon")) return;
        const auto &sec = (*pJson)["neon"];

        NeonMode mode = NeonMode::Static;
        CRGBA col(255, 255, 255, 255);
        CVector2D size(1.0f, 1.0f);
        bool smooth = true;
        float speed = 1.0f;

        if (sec.is_string())
        {
            std::string_view s = sec.get<std::string_view>();
            mode = ParseMode(s);
            if (mode == NeonMode::Static) ParseColor(sec, col);
        }
        else if (sec.is_object())
        {
            if (sec.value("rainbow", false)) mode = NeonMode::Rainbow;
            else if (sec.value("vehicle", false)) mode = NeonMode::VehicleAll;

            if (sec.contains("mode") && sec["mode"].is_string())
            {
                mode = ParseMode(sec["mode"].get<std::string_view>());
            }

            if (sec.contains("color"))
            {
                if (sec["color"].is_string())
                {
                    NeonMode m = ParseMode(sec["color"].get<std::string_view>());
                    if (m != NeonMode::Static) mode = m;
                    else ParseColor(sec["color"], col);
                }
                else
                {
                    ParseColor(sec["color"], col);
                }
            }

            smooth = sec.value("smooth", true);
            speed = sec.value("speed", 1.0f);
            if (sec.contains("size"))
            {
                if (sec["size"].is_number()) size = {sec["size"].get<float>(), sec["size"].get<float>()};
                else if (sec["size"].is_object())
                {
                    size.x = sec["size"].value("x", 1.0f);
                    size.y = sec["size"].value("y", 1.0f);
                }
            }
        }
        else
        {
            ParseColor(sec, col);
        }

        float timeSec = static_cast<float>(CTimer::m_snTimeInMilliseconds) * 0.001f * speed;

        if (mode == NeonMode::Rainbow)
        {
            float hue = smooth ? std::fmod(timeSec * 0.25f, 1.0f) : (std::floor(timeSec * 2.0f) / 6.0f);
            hue = std::fmod(hue, 1.0f);
            if (hue < 0.0f) hue += 1.0f;
            uint8_t a = col.a;
            col = RainbowColor(hue);
            col.a = a;
        }
        else if (mode == NeonMode::VehiclePrimary)
        {
            col = GetVehicleSlotColor(pVeh, 0);
        }
        else if (mode == NeonMode::VehicleSecondary)
        {
            col = GetVehicleSlotColor(pVeh, 1);
        }
        else if (mode == NeonMode::VehicleTertiary)
        {
            col = GetVehicleSlotColor(pVeh, 2);
        }
        else if (mode == NeonMode::VehicleQuaternary)
        {
            col = GetVehicleSlotColor(pVeh, 3);
        }
        else if (mode == NeonMode::VehicleAll)
        {
            VehColorList list = GetVehicleColors(pVeh);
            if (list.count == 1)
            {
                col = list.colors[0];
            }
            else
            {
                if (smooth)
                {
                    float progress = std::fmod(timeSec * 0.25f, 1.0f);
                    if (progress < 0.0f) progress += 1.0f;
                    float scaled = progress * static_cast<float>(list.count);
                    int idx1 = static_cast<int>(scaled) % list.count;
                    int idx2 = (idx1 + 1) % list.count;
                    float frac = scaled - static_cast<float>(idx1);
                    col = LerpColor(list.colors[idx1], list.colors[idx2], frac);
                }
                else
                {
                    int idx = static_cast<int>(timeSec * 2.0f) % list.count;
                    if (idx < 0) idx += list.count;
                    col = list.colors[idx];
                }
            }
        }

        float distSq = MathUtil::DistanceSquared(pVeh->GetPosition(), TheCamera.GetPosition());
        float maxDist = RenderUtil::GetLightShadowDistance();
        if (distSq > maxDist * maxDist) return;

        float distAlpha = 1.0f;
        float dist = std::sqrt(distSq);
        if (dist > maxDist * 0.7f) distAlpha = (maxDist - dist) / (maxDist * 0.3f);

        auto *pInfo = static_cast<CVehicleModelInfo *>(CModelInfo::GetModelInfo(pVeh->m_nModelIndex));
        if (!pInfo || !pInfo->m_pColModel) return;

        const auto &minB = pInfo->m_pColModel->m_boundBox.m_vecMin;
        const auto &maxB = pInfo->m_pColModel->m_boundBox.m_vecMax;

        const CMatrix &matrix = *(CMatrix *)pVeh->m_matrix;
        CVector upDir = matrix.up;
        CVector rightDir = matrix.right;
        if (pVeh->m_nVehicleSubClass == VEHICLE_BIKE)
        {
            CBike *pBike = static_cast<CBike *>(pVeh);
            if (!pBike->m_bLeanMatrixCalculated)
            {
                pBike->CalculateLeanMatrix();
            }
            upDir = pBike->m_mLeanMatrix.up;
            rightDir = pBike->m_mLeanMatrix.right;
        }
        upDir.z = rightDir.z = 0.0f;
        upDir.Normalize();
        rightDir.Normalize();

        CVector up = upDir * ((maxB.y - minB.y) * 0.5f * size.y);
        CVector right = rightDir * ((maxB.x - minB.x) * 0.5f * size.x);

        CVector center = pVeh->TransformFromObjectSpace(CVector((minB.x + maxB.x) * 0.5f, (minB.y + maxB.y) * 0.5f, 0.0f));
        CVector shdwPos(center.x, center.y, pVeh->GetPosition().z + 1.5f);

        RwTexture *pTex = TextureMgr::Get("neon");
        if (!pTex) pTex = TextureMgr::FindInDict("neon", nullptr, true);
        if (!pTex) return;

        short intensity = static_cast<short>(col.a * distAlpha);
        CShadows::StoreShadowToBeRendered(2, pTex, &shdwPos, up.x, up.y, right.x, right.y,
                                          intensity, col.r, col.g, col.b, 6.0f, false, 1.0f, nullptr, true);
    });
}
