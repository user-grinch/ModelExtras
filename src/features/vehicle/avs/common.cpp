#include "pch.h"
#include "common.h"
#include <CCoronas.h>
#include <CShadows.h>
#include <CCamera.h>

void Common::RegisterCorona(CVehicle* pVeh, CVector pos, uchar red, uchar green, uchar blue, uchar alpha, int id, float size) {
	if (!gConfig.ReadBoolean("FEATURES", "RenderCoronas", false)) {
		return;
	}

	unsigned int coronaID = reinterpret_cast<unsigned int>(pVeh) + 30 + id;
	CCoronas::RegisterCorona(coronaID, pVeh, red, green, blue, alpha, pos,
		size, 260.0f, CORONATYPE_SHINYSTAR, FLARETYPE_NONE, false, false, 0, 0.0f, false, 0.5f, 0, 50.0f, false, false);
};

void Common::RegisterCoronaWithAngle(CVehicle* pVeh, CVector posn, uchar red, uchar green, uchar blue, uchar alpha, int id, float angle, float radius, float size) {
	if (!gConfig.ReadBoolean("FEATURES", "RenderCoronas", false)) {
		return;
	}
	float vehicleAngle = (pVeh->GetHeading() * 180.0f) / 3.14f;
	float cameraAngle = (TheCamera.GetHeading() * 180.0f) / 3.14f;
	float dummyAngle = vehicleAngle + angle;

	float differenceAngle = ((cameraAngle > dummyAngle) ? (cameraAngle - dummyAngle) : (dummyAngle - cameraAngle));

	if (differenceAngle < 90.0f || differenceAngle > 270.0f)
		return;

	float alphaFloat = static_cast<float>(alpha);

	alphaFloat = (alphaFloat < 0.0f) ? (alphaFloat * -1) : (alphaFloat);
	float diameter = (radius / 2.0f);
	if (differenceAngle < diameter + 15.0f) { // 15.0f
		float angle = diameter - differenceAngle;

		float multiplier = (angle / 15.0f);

		alpha = static_cast<char>(alphaFloat * multiplier);
	}
	else if (differenceAngle > (360.0f - diameter) - 15.0f) {
		float angle = 15.0f - (differenceAngle - ((360.0f - diameter) - 15.0f));

		float multiplier = angle / 15.0f;

		alpha = static_cast<char>(alphaFloat * multiplier);
	}

	return RegisterCorona(pVeh, posn, red, green, blue, alpha, id, size);
};

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