#include "pch.h"
#include "common.h"
#include "defines.h"
#include <CCamera.h>
#include <CCoronas.h>
#include <CShadows.h>
#include <CWorld.h>

void Common::RegisterCorona(CVehicle *pVeh, int coronaID, CVector pos, CRGBA col, float size)
{
    if (!gConfig.ReadBoolean("VEHICLE_FEATURES", "LightCoronas", false))
    {
        return;
    }

    CCoronas::RegisterCorona(coronaID, pVeh, col.r, col.g, col.b, col.a, pos,
                             size * CORONA_SZ_MUL, 260.0f, CORONATYPE_SHINYSTAR, FLARETYPE_NONE, false, false, 0, 0.0f, false, 0.3f, 0, 30.0f, false, false);
};

float Common::NormalizeAngle(float angle)
{
    while (angle < 0.0f)
        angle += 360.0f;
    while (angle >= 360.0f)
        angle -= 360.0f;
    return angle;
}

void Common::RegisterCoronaWithAngle(CVehicle *pVeh, int coronaID, CVector posn, CRGBA col, float angle, float radius, float size)
{
    constexpr float RAD_TO_DEG = 180.0f / 3.141592653589793f;

    float vehicleAngle = NormalizeAngle(pVeh->GetHeading() * RAD_TO_DEG);
    float cameraAngle = NormalizeAngle(TheCamera.GetHeading() * RAD_TO_DEG);
    float dummyAngle = NormalizeAngle(vehicleAngle + angle);
    float fadeRange = 20.0f;
    float cutoff = (radius / 2.0f);
    float diffAngle = std::fabs(std::fmod(std::fabs(cameraAngle - dummyAngle) + 180.0f, 360.0f) - 180.0f);

    if (diffAngle < cutoff || diffAngle > (360.0f - cutoff))
    {
        return;
    }

    if (diffAngle < cutoff + fadeRange)
    {
        float adjustedAngle = cutoff - diffAngle;
        float mul = std::fabs(adjustedAngle / fadeRange);
        col.a *= mul;
    }
    else if (diffAngle > (360.0f - cutoff - fadeRange))
    {
        float adjustedAngle = fadeRange - (diffAngle - (360.0f - cutoff - fadeRange));
        float mul = std::fabs(adjustedAngle / fadeRange);
        col.a *= mul;
    }

    RegisterCorona(pVeh, coronaID, posn, col, size);
}

RwTexture *Common::GetTexture(std::string texture)
{
    if (Textures.contains(texture) && Textures[texture])
        return Textures[texture];

    Textures[texture] = Util::LoadTextureFromFile((MOD_DATA_PATH("textures/") + texture + ".png").c_str(), 30.0f);
    return Textures[texture];
};

void Common::RegisterShadow(CVehicle *pVeh, CVector position, CRGBA col, float angle, float currentAngle, const std::string &shadwTexName, CVector2D shdwSz, CVector2D shdwOffset, RwTexture *pTexture)
{
    if (shdwSz.x == 0.0f || shdwSz.y == 0.0f || !gConfig.ReadBoolean("VEHICLE_FEATURES", "LightShadows", false))
    {
        return;
    }
    CVector vehPos = pVeh->GetPosition();
    CVector center = pVeh->TransformFromObjectSpace(
        CVector(
            position.x + (shdwOffset.x * cos((90.0f - angle + currentAngle) * 3.14f / 180.0f)),
            position.y + ((0.5f + shdwOffset.y) * sin((90.0f - angle + currentAngle) * 3.14f / 180.0f)),
            position.z + 2.0f));

    // center.z = CWorld::FindGroundZFor3DCoord(center.x, center.y, center.z + 100, nullptr, nullptr);

    // if (CModelInfo::IsHeliModel(pVeh->m_nModelIndex))
    // {
    //     center.z += 3.0f; //  Fix shadows going under rooftops
    // }
    // else
    // {
    //     center.z += 0.5f;
    // }
    if (abs(vehPos.z - center.z) > 15.0f)
    {
        return;
    }

    float fAngle = pVeh->GetHeading() + (((angle + currentAngle) + 180.0f) * 3.14f / 180.0f);

    CVector up = CVector(-sin(fAngle), cos(fAngle), 0.0f);
    CVector right = CVector(cos(fAngle), sin(fAngle), 0.0f);
    up *= shdwSz.y;
    right *= shdwSz.x;

    CShadows::StoreShadowToBeRendered(2, (pTexture != NULL ? pTexture : Common::GetTexture(shadwTexName)), &center,
                                      up.x, up.y,
                                      right.x, right.y,
                                      col.a, col.r, col.g, col.b,
                                      10.0f, false, 1.0f, 0, true);
};