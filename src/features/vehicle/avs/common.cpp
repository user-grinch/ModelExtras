#include "pch.h"
#include "common.h"
#include <CCoronas.h>
#include <CShadows.h>
#include <CCamera.h>

void Common::RegisterCorona(CVehicle* pVeh, CVector pos, uchar red, uchar green, uchar blue, uchar alpha, int id, float size) {
	unsigned int coronaID = reinterpret_cast<unsigned int>(pVeh) + 30 + id;
	CCoronas::RegisterCorona(coronaID, pVeh, red, green, blue, alpha, pos,
		size, 260.0f, CORONATYPE_HEADLIGHT, FLARETYPE_NONE, false, false, 0, 0.0f, false, 0.5f, 0, 50.0f, false, false);
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

RwTexture* loadTextureFromFile(const char* filename) {
    RwImage* image = RtPNGImageRead(filename);

    RwInt32 width, height, depth, flags;
    RwImageFindRasterFormat(image, 4, &width, &height, &depth, &flags);

    RwRaster* raster = RwRasterCreate(width, height, depth, flags);

    RwRasterSetFromImage(raster, image);

    RwImageDestroy(image);

    return RwTextureCreate(raster);
};

RwTexture* Common::GetTexture(std::string texture) {
	if (Textures.contains(texture) && Textures[texture])
		return Textures[texture];

	Textures[texture] = loadTextureFromFile((MOD_DATA_PATH("textures/") + texture + ".png").c_str());
    return Textures[texture];
};

uint64_t Common::TimeSinceEpochMillisec() {
    using namespace std::chrono;

    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
};