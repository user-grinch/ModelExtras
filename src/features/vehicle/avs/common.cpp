#include "pch.h"
#include "common.h"
#include "defines.h"
#include <CCamera.h>
#include <CCoronas.h>
#include <CShadows.h>
#include <CWorld.h>

void Common::RegisterCorona(CVehicle *pVeh, int coronaID, CVector pos, uchar red, uchar green, uchar blue, uchar alpha, float size)
{
    if (!gConfig.ReadBoolean("VEHICLE_FEATURES", "LightCoronas", false))
    {
        return;
    }

    CCoronas::RegisterCorona(coronaID, pVeh, red, green, blue, alpha, pos,
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

void Common::RegisterCoronaWithAngle(CVehicle *pVeh, int coronaID, CVector posn, uchar red, uchar green, uchar blue, uchar alpha, float angle, float radius, float size)
{
    constexpr float RAD_TO_DEG = 180.0f / 3.141592653589793f;

    float vehicleAngle = NormalizeAngle(pVeh->GetHeading() * RAD_TO_DEG);
    float cameraAngle = NormalizeAngle(TheCamera.GetHeading() * RAD_TO_DEG);
    float dummyAngle = NormalizeAngle(vehicleAngle + angle);
    float InertiaAngle = 5.0f;

    float differenceAngle = std::fmod(std::fabs(cameraAngle - dummyAngle) + 180.0f, 360.0f) - 180.0f;
    differenceAngle = std::fabs(differenceAngle);

    float diameter = (radius / 2.0f);

    if (differenceAngle < diameter || differenceAngle > (360.0f - diameter))
        return;

    float alphaFloat = static_cast<float>(alpha);

    if (differenceAngle < diameter + InertiaAngle)
    {
        float adjustedAngle = diameter - differenceAngle;
        float multiplier = adjustedAngle / InertiaAngle;
        alpha = static_cast<uchar>(std::clamp(alphaFloat * multiplier, 0.0f, (float)alpha));
    }
    else if (differenceAngle > (360.0f - diameter) - InertiaAngle)
    {
        float adjustedAngle = InertiaAngle - (differenceAngle - ((360.0f - diameter) - InertiaAngle));
        float multiplier = adjustedAngle / InertiaAngle;
        alpha = static_cast<uchar>(std::clamp(alphaFloat * multiplier, 0.0f, (float)alpha));
    }

    RegisterCorona(pVeh, coronaID, posn, red, green, blue, alpha, size);
}

RwTexture *Common::GetTexture(std::string texture)
{
    if (Textures.contains(texture) && Textures[texture])
        return Textures[texture];

    Textures[texture] = Util::LoadTextureFromFile((MOD_DATA_PATH("textures/") + texture + ".png").c_str(), 100.0f);
    return Textures[texture];
};

void Common::RegisterShadow(CVehicle *pVeh, CVector position, unsigned char red, unsigned char green, unsigned char blue, unsigned int alpha, float angle, float currentAngle, const std::string &shadwTexName, float shdwSz, float shdwOffset, RwTexture *pTexture)
{
    if (shdwSz == 0.0f || !gConfig.ReadBoolean("VEHICLE_FEATURES", "LightShadows", false))
    {
        return;
    }

    float ground = CWorld::FindGroundZFor3DCoord(position.x, position.y, position.z, nullptr, nullptr);
    CVector center = pVeh->TransformFromObjectSpace(
        CVector(
            position.x + (shdwOffset * cos((90.0f - angle + currentAngle) * 3.14f / 180.0f)),
            position.y + ((1.2f + shdwOffset) * sin((90.0f - angle + currentAngle) * 3.14f / 180.0f)),
            ground - 0.5f));

    float fAngle = pVeh->GetHeading() + (((angle + currentAngle) + 180.0f) * 3.14f / 180.0f);

    CVector up = CVector(-sin(fAngle), cos(fAngle), 0.0f);
    CVector right = CVector(cos(fAngle), sin(fAngle), 0.0f);
    up *= shdwSz;
    right *= shdwSz;

    CShadows::StoreShadowToBeRendered(2, (pTexture != NULL ? pTexture : Common::GetTexture(shadwTexName)), &center,
                                      up.x, up.y,
                                      right.x, right.y,
                                      alpha, red, green, blue,
                                      2.0f, false, 1.0f, 0, true);
};