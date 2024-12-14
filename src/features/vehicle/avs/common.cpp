#include "pch.h"
#include "common.h"
#include <CCoronas.h>
#include <CShadows.h>
#include <CCamera.h>

void Common::RegisterCorona(CVehicle* pVeh, CVector pos, uchar red, uchar green, uchar blue, uchar alpha, float size) {
	if (!gConfig.ReadBoolean("FEATURES", "RenderCoronas", false)) {
		return;
	}

	unsigned int coronaID = plugin::RandomNumberInRange(0, INT_MAX);
	CCoronas::RegisterCorona(coronaID, pVeh, red, green, blue, alpha, pos,
		size, 260.0f, CORONATYPE_SHINYSTAR, FLARETYPE_NONE, false, false, 0, 0.0f, false, 0.3f, 0, 30.0f, false, false);
};

float Common::NormalizeAngle(float angle) {
    while (angle < 0.0f) angle += 360.0f;
    while (angle >= 360.0f) angle -= 360.0f;
    return angle;
}

void Common::RegisterCoronaWithAngle(CVehicle* pVeh, CVector posn, uchar red, uchar green, uchar blue, uchar alpha, float angle, float radius, float size) {
    if (!gConfig.ReadBoolean("FEATURES", "RenderCoronas", false)) {
        return;
    }

    constexpr float RAD_TO_DEG = 180.0f / 3.141592653589793f;

    float vehicleAngle = 180 + NormalizeAngle(pVeh->GetHeading() * RAD_TO_DEG);
    float cameraAngle = NormalizeAngle(TheCamera.GetHeading() * RAD_TO_DEG);
    float dummyAngle = NormalizeAngle(vehicleAngle + angle);

    // Calculate the smallest angular difference
    float differenceAngle = std::abs(cameraAngle - dummyAngle);
    if (differenceAngle > 180.0f) {
        differenceAngle = 360.0f - differenceAngle;
    }

    // Corona is visible only if the camera is facing towards it (within a 90-degree range)
    if (differenceAngle >= 90.0f) {
        return;
    }

    // Adjust alpha based on proximity to the edge of visibility
    float alphaFloat = static_cast<float>(alpha);
    float diameter = radius / 2.0f;

    if (differenceAngle > diameter - 15.0f) {
        float angleOffset = 15.0f - (differenceAngle - (diameter - 15.0f));
        float multiplier = angleOffset / 15.0f;
        alpha = static_cast<uchar>(alphaFloat * multiplier);
    }

    RegisterCorona(pVeh, posn, red, green, blue, alpha, size);
}

RwTexture* Common::GetTexture(std::string texture) {
	if (Textures.contains(texture) && Textures[texture])
		return Textures[texture];

	Textures[texture] = Util::LoadTextureFromFile((MOD_DATA_PATH("textures/") + texture + ".png").c_str(), 60.0f);
    return Textures[texture];
};

void Common::RegisterShadow(CVehicle* pVeh, CVector position, unsigned char red, unsigned char green, unsigned char blue, unsigned int alpha, float angle, float currentAngle, const std::string& shadwTexName, float shdwSz, float shdwOffset, RwTexture *pTexture) {
	if (shdwSz == 0.0f || !gConfig.ReadBoolean("FEATURES", "RenderShadows", false)) {
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

	CShadows::StoreShadowToBeRendered(2, (pTexture != NULL ? pTexture: Common::GetTexture(shadwTexName)), &center,
		up.x, up.y,
		right.x, right.y,
		alpha, red, green, blue,
		2.0f, false, 1.0f, 0, true);
};