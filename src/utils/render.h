#pragma once
#include <rwcore.h>
#include <string>
#include <CVector.h>
#include <CVector2D.h>

class CEntity;
class CRGBA;
enum class eDummyPos;
struct DummyConfig;

class RenderUtil
{
public:
    static void RegisterCorona(CEntity *pEntity, int coronaID, CVector pos, CRGBA col, float size);
    // Headlight spotlight, the thing that lights up peds, cars and the road ahead.
    // rangeMul scales how far it reaches, 1.0 is the vanilla 20.0 units.
    static void RegisterHeadlightPointLight(const DummyConfig *pConfig, float rangeMul);
    static void RegisterPointLight(const DummyConfig *pConfig, CRGBA col, float radius, bool isSpotlight = true);
    static void RegisterCoronaDirectional(const DummyConfig *pConfig, float angle, float radius, float szMul = 1.0f, bool inversed = false, bool skipCheck = true);
    static void RegisterShadow(CEntity *pEntity, CVector position, CRGBA col, float angle, eDummyPos dummyPos, const std::string &shadwTexName, CVector2D shdwSz = {1.0f, 1.0f}, CVector2D shdwOffset = {0.0f, 0.0f}, RwTexture *pTexture = nullptr);
    static void RegisterShadowDirectional(const DummyConfig *pConfig, const std::string &shadwTexName, float shdwSz);
    static void ReloadConfig();
};