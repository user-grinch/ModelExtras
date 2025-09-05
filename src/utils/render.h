#pragma once
#include <CVector.h>
#include <CVector2D.h>
#include <rwcore.h>
#include <string>

class CEntity;
class CRGBA;
enum class eDummyPos;
class VehicleDummyConfig;

class RenderUtil {
public:
  static void RegisterCorona(CEntity *pEntity, int coronaID, CVector pos,
                             CRGBA col, float size);
  static void RegisterCoronaDirectional(const VehicleDummyConfig *pConfig,
                                        float angle, float radius,
                                        float szMul = 1.0f, bool checks = false,
                                        bool inversed = false);
  static void RegisterShadow(CEntity *pEntity, CVector position, CRGBA col,
                             float angle, eDummyPos dummyPos,
                             const std::string &shadwTexName,
                             CVector2D shdwSz = {1.0f, 1.0f},
                             CVector2D shdwOffset = {0.0f, 0.0f},
                             RwTexture *pTexture = nullptr);
  static void RegisterShadowDirectional(const VehicleDummyConfig *pConfig,
                                        const std::string &shadwTexName,
                                        float shdwSz);
};