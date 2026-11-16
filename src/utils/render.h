#pragma once
#include <rwcore.h>
#include <string>
#include <CVector.h>
#include <CVector2D.h>
#include <CRegisteredCorona.h>
#include <string_view>
class CEntity;
class CRGBA;
struct DummyConfig;

class RenderUtil
{
public:
    static void RegisterCorona(CEntity *pEntity, int coronaID, CVector pos, CRGBA col, float size, float coronaDistMul = -1.0f, eCoronaType coronaType = CORONATYPE_SHINYSTAR);
    static eCoronaType ParseCoronaType(std::string_view name);
    // Headlight spotlight, the thing that lights up peds, cars and the road ahead.
    // rangeMul scales how far it reaches, 1.0 is the vanilla 20.0 units.
    static void RegisterHeadlightPointLight(const DummyConfig *pConfig, float rangeMul);
    static void RegisterPointLight(const DummyConfig *pConfig, CRGBA col, float radius, bool isSpotlight = true);
    static void RegisterCoronaDirectional(const DummyConfig *pConfig, float angle, float radius, float szMul = 1.0f, bool inversed = false, bool skipCheck = true, float coronaDistMul = -1.0f);
    static void RegisterShadow(CEntity *pEntity, CVector position, CRGBA col, float angle, const std::string &shadwTexName, CVector2D shdwSz = {1.0f, 1.0f}, CVector2D shdwOffset = {0.0f, 0.0f}, RwTexture *pTexture = nullptr);
    static void RegisterShadowDirectional(const DummyConfig *pConfig, const std::string &shadwTexName, float shdwSz, float alphaMul = 1.0f);
    // Downward cast shared by every ground shadow, so pools stay pinned to the
    // surface below while the vehicle is airborne. 7m from the stored caster
    // (vehicle +2) means the pool ends at roughly 5m of real altitude, about
    // three car heights: past that a headlight beam aimed ~11 degrees down no
    // longer reaches the road inside its 20m range, so the pool would be fake.
    static constexpr float GROUND_SHADOW_REACH = 7.0f;
    static void ReloadConfig();
    static float GetCoronaDistanceMul();
    static float GetHeadLightCoronaDistanceMul();
    static float GetTailLightCoronaDistanceMul();
    static float GetLightShadowDistance();
};
