#include "pch.h"
#include "common.h"
#include <CCoronas.h>
#include <CShadows.h>

void Common::RegisterCorona(CVehicle* pVeh, CVector pos, uchar red, uchar green, uchar blue, uchar alpha, int id, float size) {
	return CCoronas::RegisterCorona(reinterpret_cast<unsigned int>(pVeh) + 30 + id, pVeh, red, green, blue, alpha, pos,
		size, 150.0f, CORONATYPE_HEADLIGHT, FLARETYPE_NONE, false, false, 0, 0.0f, false, 0.5f, 0, 50.0f, false, false);
};

void Common::RegisterCoronaWithAngle(CVehicle* pVeh, CVector posn, uchar red, uchar green, uchar blue, uchar alpha, int id, float cameraAngle, float angle, float radius, float size) {
	float differenceAngle = ((cameraAngle > angle) ? (cameraAngle - angle) : (angle - cameraAngle));

	float diameter = (radius / 2.0f);

	if (differenceAngle < diameter || differenceAngle > (360.0f - diameter))
		return;

	float alphaFloat = static_cast<float>(alpha);

	alphaFloat = (alphaFloat < 0.0f) ? (alphaFloat * -1) : (alphaFloat);

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

void Common::RegisterShadow(CVehicle* pVeh, CVector position, unsigned char red, unsigned char green, unsigned char blue, float angle, float currentAngle) {
	static RwTexture *pShadowTex = nullptr;
	if (!pShadowTex) {
		pShadowTex = Util::LoadTextureFromFile(MOD_DATA_PATH_S(std::string("textures/taillight256.png")));
	}

	float Offset = 0.0f;
	float Size = 0.6f;
	float InertiaMultiplier = 1.0f;
	
	CVector center = pVeh->TransformFromObjectSpace(
		CVector(
			position.x + (Offset * cos((90.0f - angle + currentAngle) * 3.14f / 180.0f)),
			position.y + ((0.5f + Offset) * sin((90.0f - angle + currentAngle) * 3.14f / 180.0f)),
			position.z
		)
	);

	float fAngle = pVeh->GetHeading() + (((angle + currentAngle) + 180.0f) * 3.14f / 180.0f);

	CVector up = CVector(-sin(fAngle), cos(fAngle), 0.0f);

	CVector right = CVector(cos(fAngle), sin(fAngle), 0.0f);

	CShadows::StoreShadowToBeRendered(2, pShadowTex, &center,
		up.x, up.y,
		right.x, right.y,
		115, red, green, blue,
		2.0f, false, 1.0f, 0, true);
};