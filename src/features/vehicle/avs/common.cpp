#include "pch.h"
#include "common.h"
#include <CCoronas.h>
#include <CShadows.h>
#include <CCamera.h>

void Common::RegisterCorona(CVehicle* pVeh, int coronaID, CVector pos, uchar red, uchar green, uchar blue, uchar alpha, float size) {
    if (!gConfig.ReadBoolean("VEHICLE_FEATURES", "LightCoronas", false)) {
        return;
    }

    CCoronas::RegisterCorona(coronaID, pVeh, red, green, blue, alpha, pos,
        size / 4.0f, 260.0f, CORONATYPE_SHINYSTAR, FLARETYPE_NONE, false, false, 0, 0.0f, false, 0.3f, 0, 30.0f, false, false);
};

float Common::NormalizeAngle(float angle) {
    while (angle < 0.0f) angle += 360.0f;
    while (angle >= 360.0f) angle -= 360.0f;
    return angle;
}

void Common::RegisterCoronaWithAngle(CVehicle* pVeh, int coronaID, CVector posn, uchar red, uchar green, uchar blue, uchar alpha, float angle, float radius, float size) {
    constexpr float RAD_TO_DEG = 180.0f / 3.141592653589793f;

    float vehicleAngle = 180 + NormalizeAngle(pVeh->GetHeading() * RAD_TO_DEG);
    float cameraAngle = NormalizeAngle(TheCamera.GetHeading() * RAD_TO_DEG);
    float dummyAngle = NormalizeAngle(vehicleAngle + angle);
    float InertiaAngle = 5.0f;

    float differenceAngle = ((cameraAngle > angle) ? (cameraAngle - angle) : (angle - cameraAngle));

    float diameter = (radius / 2.0f);

    if (differenceAngle < diameter || differenceAngle >(360.0f - diameter))
        return;

    // if (PluginConfig::Lights->Enhancement->InertiaEnabled) {
    float alphaFloat = static_cast<float>(alpha);

    alphaFloat = (alphaFloat < 0.0f) ? (alphaFloat * -1) : (alphaFloat);

    if (differenceAngle < diameter + InertiaAngle) {
        float angle = diameter - differenceAngle;

        float multiplier = (angle / InertiaAngle);

        alpha = static_cast<char>(alphaFloat * multiplier);
    }
    else if (differenceAngle > (360.0f - diameter) - InertiaAngle) {
        float angle = InertiaAngle - (differenceAngle - ((360.0f - diameter) - InertiaAngle));

        float multiplier = angle / InertiaAngle;

        alpha = static_cast<char>(alphaFloat * multiplier);
    }
// }

    return RegisterCorona(pVeh, coronaID, posn, red, green, blue, alpha, size);
}

RwTexture* Common::GetTexture(std::string texture) {
    if (Textures.contains(texture) && Textures[texture])
        return Textures[texture];

    Textures[texture] = Util::LoadTextureFromFile((MOD_DATA_PATH("textures/") + texture + ".png").c_str(), 60.0f);
    return Textures[texture];
};

void Common::RegisterShadow(CVehicle* pVeh, CVector position, unsigned char red, unsigned char green, unsigned char blue, unsigned int alpha, float angle, float currentAngle, const std::string& shadwTexName, float shdwSz, float shdwOffset, RwTexture* pTexture) {
    if (shdwSz == 0.0f || !gConfig.ReadBoolean("VEHICLE_FEATURES", "LightShadows", false)) {
        return;
    }

    CVector center = pVeh->TransformFromObjectSpace(
        CVector(
            position.x + (shdwOffset * cos((90.0f - angle + currentAngle) * 3.14f / 180.0f)),
            position.y + ((1.2f + shdwOffset) * sin((90.0f - angle + currentAngle) * 3.14f / 180.0f)),
            position.z
        )
    );

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