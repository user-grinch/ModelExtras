#include "pch.h"
#include "render.h"
#include "util.h"

#include <CCoronas.h>
#include <CShadows.h>
#include <CBike.h>
#include <CWorld.h>
#include "utils/texmgr.h"
#include "defines.h"
#include "features/core/dummy.h"
#include "features/lights/data.h"
#include "features/lights/damage.h"
#include <CPointLights.h>

// Test if dummy forward vector points toward vehicle center (catches backward-facing dummies from 3D models)
static inline bool IsShadowTowardVehicle(const CMatrix &dummyMatrix, const CVector &vehicleCenter)
{
    CVector toVehicle = vehicleCenter - dummyMatrix.pos;
    // Dot product sign in 2D determines direction alignment without needing vector normalization
    return (dummyMatrix.up.x * toVehicle.x + dummyMatrix.up.y * toVehicle.y) > 0.0f;
}

static inline void RotateMatrix180Z(CMatrix &mat)
{
    mat.right.x = -mat.right.x;
    mat.right.y = -mat.right.y;
    mat.up.x = -mat.up.x;
    mat.up.y = -mat.up.y;
}

// In RenderWare frame conventions, mat.up represents the dummy's forward direction.
// Checking if up.z > 0.7f verifies if the dummy points upward (e.g., roof/vertical markers) without temporary allocations.
static inline bool IsDummyPointingUp(const CMatrix &mat)
{
    return mat.up.z > 0.7f;
}

static bool gbLightCoronas = false;
static bool gbLightShadows = false;
static float gfCoronaDistanceMul = 0.07f;
static float gfHeadLightCoronaDistanceMul = 0.07f;
static float gfTailLightCoronaDistanceMul = 0.05f;
constexpr float gfCoronaNearClip = 0.45f;
static bool gbConfigInitialized = false;
static float gfLightShadowDistance = 120.0f;
static int gnHeadLightShadowIntensity = 80;
static int gnTailLightShadowIntensity = 80;

extern int gGlobalShadowIntensity;
extern int gGlobalCoronaIntensity;

void RenderUtil::ReloadConfig()
{
    gbLightCoronas = gConfig.ReadBoolean("LIGHTS", "LightCoronas", true);
    gbLightShadows = gConfig.ReadBoolean("LIGHTS", "LightShadows", true);
    gfCoronaDistanceMul = gConfig.ReadFloat("LIGHTS", "LightCoronaDistanceMul", 0.07f);
    gfHeadLightCoronaDistanceMul = gConfig.ReadFloat("LIGHTS", "HeadLightCoronaDistanceMul", gfCoronaDistanceMul);
    gfTailLightCoronaDistanceMul = gConfig.ReadFloat("LIGHTS", "TailLightCoronaDistanceMul", 0.05f);
    gGlobalShadowIntensity = gConfig.ReadInteger("LIGHTS", "LightShadowIntensity", 80);
    gGlobalCoronaIntensity = gConfig.ReadInteger("LIGHTS", "LightCoronaIntensity", 80);
    gnHeadLightShadowIntensity = gConfig.ReadInteger("LIGHTS", "HeadLightShadowIntensity", gGlobalShadowIntensity);
    gnTailLightShadowIntensity = gConfig.ReadInteger("LIGHTS", "TailLightShadowIntensity", gGlobalShadowIntensity);
    gfLightShadowDistance = gConfig.ReadFloat("LIGHTS", "LightShadowDistance", 120.0f);
    gbConfigInitialized = true;
}

static void EnsureConfigLoaded()
{
    if (!gbConfigInitialized)
    {
        RenderUtil::ReloadConfig();
    }
}

float RenderUtil::GetCoronaDistanceMul()
{
    EnsureConfigLoaded();
    return gfCoronaDistanceMul;
}

float RenderUtil::GetHeadLightCoronaDistanceMul()
{
    EnsureConfigLoaded();
    return gfHeadLightCoronaDistanceMul;
}

float RenderUtil::GetTailLightCoronaDistanceMul()
{
    EnsureConfigLoaded();
    return gfTailLightCoronaDistanceMul;
}

float RenderUtil::GetLightShadowDistance()
{
    EnsureConfigLoaded();
    return gfLightShadowDistance;
}

void RenderUtil::RegisterCorona(CEntity *pEntity, int coronaID, CVector pos, CRGBA col, float size, float coronaDistMul, eCoronaType coronaType)
{
    EnsureConfigLoaded();
    if (!gbLightCoronas)
    {
        return;
    }

    float coronaSz = size;

    // Smooth day/night balance factor from GTA SA engine pipeline (0x8D12C0: 0.0 = day, 1.0 = night)
    float dnBalance = *(float *)0x8D12C0;
    if (Util::IsNightTime() && dnBalance < 0.2f) {
        dnBalance = 0.5f;
    }

    if (dnBalance > 0.01f) {
        float effectiveMul = (coronaDistMul >= 0.0f) ? coronaDistMul : gfCoronaDistanceMul;
        if (effectiveMul > 0.0f) {
            // pEntity is null for unattached coronas, pos is already in world space then
            CVector refPos = pEntity ? pEntity->GetPosition() : pos;
            float distSq = MathUtil::DistanceSquared(TheCamera.GetPosition(), refPos);
            float dist = std::sqrt(distSq);
            coronaSz = std::max(size, size * dist * effectiveMul * dnBalance);
        }
    }

    CCoronas::RegisterCorona(coronaID, pEntity, col.r, col.g, col.b, col.a, pos,
                             coronaSz, 350.0f, coronaType, FLARETYPE_NONE, true, false, 0, 0.0f, false, gfCoronaNearClip, 0, 255.0f, false, false);
}

// Vanilla reach of the headlight spotlight
static inline CVector GetDummyWorldPosition(const DummyConfig *pConfig)
{
    if (pConfig->pVeh->m_nVehicleSubClass == VEHICLE_BIKE && pConfig->leanAffected)
    {
        CBike *pBike = static_cast<CBike *>(pConfig->pVeh);
        bool wasCalculated = pBike->m_bLeanMatrixCalculated;
        if (!wasCalculated)
        {
            pBike->CalculateLeanMatrix();
        }
        CVector pos = pBike->m_mLeanMatrix * pConfig->position;
        pBike->m_bLeanMatrixCalculated = wasCalculated;
        return pos;
    }
    return pConfig->pVeh->TransformFromObjectSpace(pConfig->position);
}

static inline bool IsPointLightDummyDamaged(const DummyConfig *pConfig)
{
    if (pConfig->pVeh->m_nVehicleSubClass == VEHICLE_BIKE || !pConfig->pVeh->GetIsOnScreen())
    {
        return false;
    }

    if (pConfig->damagePanel != -1 || pConfig->damageDoor != -1)
    {
        return CarUtil::IsDummyDamaged(pConfig->pVeh, *pConfig);
    }

    RwFrame *parent = pConfig->frame ? RwFrameGetParent(pConfig->frame) : nullptr;
    if (parent)
    {
        return Util::IsFrameDamaged(pConfig->pVeh, parent) || !FrameUtil::IsOkAtomicVisible(parent);
    }
    return false;
}

static inline bool IsLightTypeDamaged(CVehicle *pVeh, eMaterialType type)
{
    auto damage = LightDamageState::Get(pVeh);
    switch (type)
    {
    case eMaterialType::HeadLightLeft:
    case eMaterialType::HighBeamLeft:
    case eMaterialType::FogLightLeft:
        return !damage.isHeadlightLeftOk;

    case eMaterialType::HeadLightRight:
    case eMaterialType::HighBeamRight:
    case eMaterialType::FogLightRight:
        return !damage.isHeadlightRightOk;

    case eMaterialType::IndicatorLightLeftFront:
        return !damage.isFrontLeftOk;

    case eMaterialType::IndicatorLightRightFront:
        return !damage.isFrontRightOk;

    case eMaterialType::TailLightLeft:
    case eMaterialType::BrakeLightLeft:
    case eMaterialType::ReverseLightLeft:
    case eMaterialType::IndicatorLightLeftRear:
    case eMaterialType::NABrakeLightLeft:
    case eMaterialType::STTLightLeft:
        return !damage.isRearLeftOk;

    case eMaterialType::TailLightRight:
    case eMaterialType::BrakeLightRight:
    case eMaterialType::ReverseLightRight:
    case eMaterialType::IndicatorLightRightRear:
    case eMaterialType::NABrakeLightRight:
    case eMaterialType::STTLightRight:
        return !damage.isRearRightOk;

    case eMaterialType::SideLightLeft:
    case eMaterialType::IndicatorLightLeftMiddle:
        return !damage.isMiddleLeftOk;

    case eMaterialType::SideLightRight:
    case eMaterialType::IndicatorLightRightMiddle:
        return !damage.isMiddleRightOk;

    default:
        return false;
    }
}

// Vanilla reach of the headlight spotlight
static const float HEADLIGHT_PLIGHT_RANGE = 20.0f;

void RenderUtil::RegisterHeadlightPointLight(const DummyConfig *pConfig, float rangeMul)
{
    extern bool gbLightPointLights;
    if (!gbLightPointLights || !pConfig || !pConfig->frame || !pConfig->pVeh || !pConfig->pointlight.enabled)
    {
        return;
    }

    if (IsPointLightDummyDamaged(pConfig))
    {
        return;
    }

    bool isLeft = (pConfig->lightType == eMaterialType::HeadLightLeft || pConfig->lightType == eMaterialType::HighBeamLeft || pConfig->position.x < 0.0f);
    eMaterialType checkType = isLeft ? eMaterialType::HeadLightLeft : eMaterialType::HeadLightRight;
    if (IsLightTypeDamaged(pConfig->pVeh, checkType))
    {
        return;
    }

    const CMatrix &mat = *reinterpret_cast<const CMatrix *>(&pConfig->frame->ltm);
    if (IsDummyPointingUp(mat))
    {
        return;
    }

    CVector lightPos = GetDummyWorldPosition(pConfig);
    const CMatrix &vehMat = pConfig->pVeh->GetMatrix();

    CVector localDir(
        CVector::Dot(mat.up, vehMat.right),
        CVector::Dot(mat.up, vehMat.up),
        CVector::Dot(mat.up, vehMat.at)
    );
    if (pConfig->mirroredX) localDir.x = -localDir.x;
    if (localDir.Magnitude() < 0.1f || localDir.y <= 0.0f) {
        localDir = CVector(0.0f, 1.0f, -0.10f);
    }
    localDir.Normalize();

    CVector lightDir = vehMat.right * localDir.x + vehMat.up * localDir.y + vehMat.at * localDir.z;
    lightDir.Normalize();

    CRGBA col = pConfig->corona.color;
    float intensity = pConfig->pointlight.intensity;
    CVector plightPos = lightPos + lightDir * (pConfig->pointlight.distance * LightsConfig::Get().fPointLightOffsetMul);
    float effectiveRange = HEADLIGHT_PLIGHT_RANGE * rangeMul * LightsConfig::Get().fPointLightSizeMul * pConfig->pointlight.size;
    CPointLights::AddLight(PLTYPE_SPOTLIGHT, plightPos, lightDir, effectiveRange,
                           (col.r / 255.0f) * intensity, (col.g / 255.0f) * intensity, (col.b / 255.0f) * intensity, 0, 0, 0);
}

void RenderUtil::RegisterPointLight(const DummyConfig *pConfig, CRGBA col, float radius, bool isSpotlight)
{
    extern bool gbLightPointLights;
    if (!gbLightPointLights || !pConfig || !pConfig->frame || !pConfig->pVeh || radius <= 0.0f || !pConfig->pointlight.enabled)
    {
        return;
    }

    if (IsPointLightDummyDamaged(pConfig))
    {
        return;
    }

    if (IsLightTypeDamaged(pConfig->pVeh, pConfig->lightType))
    {
        return;
    }

    const CMatrix &mat = *reinterpret_cast<const CMatrix *>(&pConfig->frame->ltm);
    if (IsDummyPointingUp(mat))
    {
        return;
    }

    CVector lightPos = GetDummyWorldPosition(pConfig);
    const CMatrix &vehMat = pConfig->pVeh->GetMatrix();

    // Extract dummy forward vector directly in vehicle space (respecting modder's 3D rotation)
    CVector localDir(
        CVector::Dot(mat.up, vehMat.right),
        CVector::Dot(mat.up, vehMat.up),
        CVector::Dot(mat.up, vehMat.at)
    );

    if (pConfig->mirroredX)
    {
        localDir.x = -localDir.x;
    }

    bool isExplicitRearType = (pConfig->lightType == eMaterialType::TailLightLeft || pConfig->lightType == eMaterialType::TailLightRight ||
                               pConfig->lightType == eMaterialType::BrakeLightLeft || pConfig->lightType == eMaterialType::BrakeLightRight ||
                               pConfig->lightType == eMaterialType::ReverseLightLeft || pConfig->lightType == eMaterialType::ReverseLightRight ||
                               pConfig->lightType == eMaterialType::STTLightLeft || pConfig->lightType == eMaterialType::STTLightRight ||
                               pConfig->lightType == eMaterialType::NABrakeLightLeft || pConfig->lightType == eMaterialType::NABrakeLightRight ||
                               pConfig->lightType == eMaterialType::IndicatorLightLeftRear || pConfig->lightType == eMaterialType::IndicatorLightRightRear);

    // If modder did not rotate the dummy (localDir is default +Y forward) but placed it at the rear:
    if (isExplicitRearType || (pConfig->position.y < -0.3f && localDir.y > 0.7f && std::abs(localDir.x) < 0.3f))
    {
        localDir.y = -std::abs(localDir.y);
    }

    if (pConfig->lightType == eMaterialType::SideLightLeft || pConfig->lightType == eMaterialType::IndicatorLightLeftMiddle)
    {
        localDir.x = -1.0f;
    }
    else if (pConfig->lightType == eMaterialType::SideLightRight || pConfig->lightType == eMaterialType::IndicatorLightRightMiddle)
    {
        localDir.x = 1.0f;
    }

    // Apply a slight downward pitch (-0.2f) so the spotlight cone cleanly hits the ground/surroundings
    if (localDir.z > -0.2f)
    {
        localDir.z = -0.2f;
    }

    localDir.Normalize();
    CVector lightDir = vehMat.right * localDir.x + vehMat.up * localDir.y + vehMat.at * localDir.z;
    lightDir.Normalize();

    float pushDist = 0.5f;
    if (gbProperShadersDetected)
    {
        pushDist = 0.05f;
    }
    else
    {
        switch (pConfig->lightType)
        {
        case eMaterialType::FogLightLeft:
        case eMaterialType::FogLightRight:
            pushDist = 0.65f;
            break;
        case eMaterialType::ReverseLightLeft:
        case eMaterialType::ReverseLightRight:
            pushDist = 0.55f;
            break;
        case eMaterialType::IndicatorLightLeftFront:
        case eMaterialType::IndicatorLightRightFront:
        case eMaterialType::IndicatorLightLeftRear:
        case eMaterialType::IndicatorLightRightRear:
        case eMaterialType::IndicatorLightLeftMiddle:
        case eMaterialType::IndicatorLightRightMiddle:
            pushDist = 0.4f;
            break;
        case eMaterialType::SideLightLeft:
        case eMaterialType::SideLightRight:
        case eMaterialType::AllDayLight:
        case eMaterialType::DayLight:
        case eMaterialType::NightLight:
            pushDist = 0.25f;
            break;
        default:
            pushDist = 0.5f;
            break;
        }
    }

    float effectivePushDist = (pushDist * LightsConfig::Get().fPointLightOffsetMul) + pConfig->pointlight.distance;
    CVector plightPos = lightPos + lightDir * effectivePushDist;

    constexpr float INV_255 = 1.0f / 255.0f;
    float intensity = pConfig->pointlight.intensity;
    float r = std::max(0.0f, (col.r * INV_255) * intensity);
    float g = std::max(0.0f, (col.g * INV_255) * intensity);
    float b = std::max(0.0f, (col.b * INV_255) * intensity);

    float effectiveRadius = radius * LightsConfig::Get().fPointLightSizeMul * pConfig->pointlight.size;
    if (effectiveRadius <= 0.0f)
    {
        return;
    }

    unsigned char lightType = isSpotlight ? PLTYPE_SPOTLIGHT : PLTYPE_POINTLIGHT;
    CPointLights::AddLight(lightType, plightPos, lightDir, effectiveRadius, r, g, b, 0, false, nullptr);
}

void RenderUtil::RegisterCoronaDirectional(const DummyConfig *pConfig, float angle, float radius, float szMul, bool inversed, bool skipCheck, float coronaDistMul)
{
    float sz = pConfig->corona.size * szMul;
    CRGBA col = pConfig->corona.color;

    CMatrix mat = *(CMatrix *)&pConfig->frame->ltm;
    if (!IsDummyPointingUp(mat))
    {
        float targetAngle = angle;
        if (pConfig->lightType == eMaterialType::HeadLightLeft || pConfig->lightType == eMaterialType::HeadLightRight
            || pConfig->lightType == eMaterialType::HighBeamLeft || pConfig->lightType == eMaterialType::HighBeamRight
            || pConfig->lightType == eMaterialType::IndicatorLightLeftFront || pConfig->lightType == eMaterialType::IndicatorLightRightFront)
            targetAngle = 0.0f;
        else if (pConfig->lightType == eMaterialType::TailLightLeft || pConfig->lightType == eMaterialType::TailLightRight
              || pConfig->lightType == eMaterialType::BrakeLightLeft || pConfig->lightType == eMaterialType::BrakeLightRight
              || pConfig->lightType == eMaterialType::ReverseLightLeft || pConfig->lightType == eMaterialType::ReverseLightRight
              || pConfig->lightType == eMaterialType::STTLightLeft || pConfig->lightType == eMaterialType::STTLightRight
              || pConfig->lightType == eMaterialType::IndicatorLightLeftRear || pConfig->lightType == eMaterialType::IndicatorLightRightRear
              || pConfig->lightType == eMaterialType::NABrakeLightLeft || pConfig->lightType == eMaterialType::NABrakeLightRight)
            targetAngle = 180.0f;
        else if (pConfig->lightType == eMaterialType::SideLightLeft || pConfig->lightType == eMaterialType::IndicatorLightLeftMiddle)
            targetAngle = 90.0f;
        else if (pConfig->lightType == eMaterialType::SideLightRight || pConfig->lightType == eMaterialType::IndicatorLightRightMiddle)
            targetAngle = 270.0f;

        if (inversed)
        {
            targetAngle += 180.0f;
        }

        float vehicleAngle = Util::NormalizeAngle(static_cast<float>(Util::RadToDeg(pConfig->pVeh->GetHeading())));
        float cameraAngle = Util::NormalizeAngle(static_cast<float>(Util::RadToDeg(TheCamera.GetHeading())));
        float dummyAngle = Util::NormalizeAngle(vehicleAngle + targetAngle);
        float diffAngle = Util::NormalizeAngle(cameraAngle - dummyAngle);
        float cutoff = (radius / 2.0f);
        float angleFromCenter = std::fabs(diffAngle - 180.0f);

        if (angleFromCenter >= cutoff)
        {
            return;
        }

        // Smooth cosine falloff from 1.0 at center facing camera to 0.0 at cutoff edge
        float normAngle = (angleFromCenter / cutoff) * 1.57079632679f; // PI / 2
        float angleMul = std::cos(normAngle);
        col.a = static_cast<unsigned char>(col.a * angleMul);
    }

    float distMul = coronaDistMul;
    if (distMul < 0.0f && pConfig) {
        if (pConfig->lightType == eMaterialType::HeadLightLeft || pConfig->lightType == eMaterialType::HeadLightRight || pConfig->lightType == eMaterialType::HighBeamLeft || pConfig->lightType == eMaterialType::HighBeamRight) {
            distMul = gfHeadLightCoronaDistanceMul;
        } else if (pConfig->lightType == eMaterialType::TailLightLeft || pConfig->lightType == eMaterialType::TailLightRight ||
                   pConfig->lightType == eMaterialType::BrakeLightLeft || pConfig->lightType == eMaterialType::BrakeLightRight ||
                   pConfig->lightType == eMaterialType::ReverseLightLeft || pConfig->lightType == eMaterialType::ReverseLightRight ||
                   pConfig->lightType == eMaterialType::STTLightLeft || pConfig->lightType == eMaterialType::STTLightRight ||
                   pConfig->lightType == eMaterialType::NABrakeLightLeft || pConfig->lightType == eMaterialType::NABrakeLightRight) {
            distMul = gfTailLightCoronaDistanceMul;
        }
    }
    RegisterCorona(pConfig->pVeh, reinterpret_cast<int32_t>(pConfig), pConfig->position, col, sz, distMul, pConfig->corona.textureType);
}

// Shadow opacity is resolved here so the per light type INI keys are respected. The dummy's
// alpha is baked in when the dummy is created, so reading it back would go stale after a reload.
static int GetShadowIntensity(eMaterialType lightType)
{
    int intensity = gGlobalShadowIntensity;
    switch (lightType)
    {
    case eMaterialType::HeadLightLeft:
    case eMaterialType::HeadLightRight:
    case eMaterialType::HighBeamLeft:
    case eMaterialType::HighBeamRight:
        intensity = gnHeadLightShadowIntensity;
        break;
    case eMaterialType::TailLightLeft:
    case eMaterialType::TailLightRight:
    case eMaterialType::STTLightLeft:
    case eMaterialType::STTLightRight:
    case eMaterialType::BrakeLightLeft:
    case eMaterialType::BrakeLightRight:
    case eMaterialType::NABrakeLightLeft:
    case eMaterialType::NABrakeLightRight:
        intensity = gnTailLightShadowIntensity;
        break;
    default:
        break;
    }
    return std::clamp(intensity, 0, 255);
}

void RenderUtil::RegisterShadowDirectional(const DummyConfig *pConfig, const std::string &shadwTexName, float shdwSz, float alphaMul)
{
    EnsureConfigLoaded();
    const float SHDW_SZ_MUL = 2.0f;
    const float SHDW_MAX_DIST = gfLightShadowDistance;
    const float SHDW_FADE_DIST = gfLightShadowDistance * 0.60f;
    if (!pConfig || !pConfig->pVeh || !pConfig->frame || shdwSz <= 0.0f || !gbLightShadows)
    {
        return;
    }

    // Distance culling: shadow is not visible past configured distance
    float distSq = MathUtil::DistanceSquared(pConfig->pVeh->GetPosition(), TheCamera.GetPosition());
    if (distSq > (SHDW_MAX_DIST * SHDW_MAX_DIST))
    {
        return;
    }

    extern bool gbProperShadersDetected;
    extern bool gbLightPointLights;
    if (gbProperShadersDetected && (gbLightPointLights || pConfig->lightType == eMaterialType::HeadLightLeft || pConfig->lightType == eMaterialType::HeadLightRight || pConfig->lightType == eMaterialType::HighBeamLeft || pConfig->lightType == eMaterialType::HighBeamRight))
    {
        return;
    }

    const CMatrix &ltmMat = *reinterpret_cast<const CMatrix *>(&pConfig->frame->ltm);
    if (IsDummyPointingUp(ltmMat))
    {
        return;
    }

    CMatrix mat = ltmMat;
    if (pConfig->shadow.rotationChecks && IsShadowTowardVehicle(ltmMat, pConfig->pVeh->GetPosition()))
    {
        RotateMatrix180Z(mat);
    }

    // Calculate world position (accounting for motorcycle lean when applicable)
    CVector worldPos = GetDummyWorldPosition(pConfig);

    // Prevent unrealistic ground shadow projection when vehicle is airborne (flying off ramps/in air)
    if (pConfig->pVeh->GetNumContactWheels() == 0)
    {
        float groundZ = pConfig->pVeh->m_fVehicleFrontGroundZ;
        if (groundZ < -100.0f)
        {
            bool found = false;
            groundZ = CWorld::FindGroundZFor3DCoord(worldPos.x, worldPos.y, worldPos.z, &found, nullptr);
        }
        if ((worldPos.z - groundZ) >= 1.4f)
        {
            return;
        }
    }

    // 3D light direction vectors directly from dummy matrix (retaining full 3D roll, pitch, and yaw)
    CVector lightDir = mat.up;
    CVector rightDir = pConfig->mirroredX ? -mat.right : mat.right;

    // Scale shadow size/thickness with inertia factor so it expands/contracts naturally
    float currentShdwSz = shdwSz * (0.35f + 0.65f * std::clamp(alphaMul, 0.0f, 1.0f));

    // Push shadow forward along light direction (with optional JSON shadow.offset: offset.y along lightDir, offset.x along rightDir)
    float forwardPush = currentShdwSz * SHDW_SZ_MUL + 0.2f + pConfig->shadow.offset.y;
    float sidePush = pConfig->shadow.offset.x;

    CVector shdwCenter = worldPos + lightDir * forwardPush + rightDir * sidePush;

    CVector2D shdwFront(lightDir.x * (currentShdwSz * SHDW_SZ_MUL), lightDir.y * (currentShdwSz * SHDW_SZ_MUL));
    CVector2D shdwSide(rightDir.x * currentShdwSz, rightDir.y * currentShdwSz);

    RwTexture *pTex = TextureMgr::Get(shadwTexName);
    if (!pTex)
    {
        return;
    }

    CVector shdwPos(shdwCenter.x, shdwCenter.y, pConfig->pVeh->GetPosition().z + 2.0f);

    // Fade towards the cutoff so distant shadows don't pop in and out
    float distAlpha = 1.0f;
    if (distSq > (SHDW_FADE_DIST * SHDW_FADE_DIST))
    {
        float distToCam = std::sqrt(distSq);
        distAlpha = (SHDW_MAX_DIST - distToCam) / (SHDW_MAX_DIST - SHDW_FADE_DIST);
    }
    float finalAlpha = distAlpha * std::clamp(alphaMul, 0.0f, 1.0f);

    constexpr float INV_255 = 1.0f / 255.0f;
    float shadowIntensityFactor = static_cast<float>(GetShadowIntensity(pConfig->lightType)) * INV_255;
    unsigned char r = static_cast<unsigned char>(pConfig->shadow.color.r * shadowIntensityFactor);
    unsigned char g = static_cast<unsigned char>(pConfig->shadow.color.g * shadowIntensityFactor);
    unsigned char b = static_cast<unsigned char>(pConfig->shadow.color.b * shadowIntensityFactor);

    CShadows::StoreShadowToBeRendered(
        2,
        pTex,
        &shdwPos,
        shdwFront.x, shdwFront.y,
        shdwSide.x, shdwSide.y,
        static_cast<short>(255 * finalAlpha),
        r,
        g,
        b,
        7.0f,
        false,
        1.0f,
        0,
        true);
}

void RenderUtil::RegisterShadow(CEntity *pEntity, CVector position, CRGBA col, float angle,
                                const std::string &shadwTexName,
                                CVector2D shdwSz, CVector2D shdwOffset, RwTexture *pTexture)
{
    EnsureConfigLoaded();
    extern bool gbProperShadersDetected;
    extern bool gbLightPointLights;
    extern bool gbSirenPointLights;
    if (!pEntity || shdwSz.x == 0.0f || shdwSz.y == 0.0f || !gbLightShadows)
    {
        return;
    }
    if (gbProperShadersDetected && (gbLightPointLights && gbSirenPointLights))
    {
        return;
    }

    const float angleRad = DegToRad(angle);
    const CVector vehPos = pEntity->GetPosition();
    const CMatrix &entityMatrix = *reinterpret_cast<const CMatrix *>(pEntity->m_matrix);

    float cosA = std::cos(angleRad);
    float sinA = std::sin(angleRad);

    auto RotateVector2D = [cosA, sinA](const CVector &v) -> CVector
    {
        return {
            v.x * cosA - v.y * sinA,
            v.x * sinA + v.y * cosA,
            v.z};
    };

    CVector upDir = entityMatrix.up;
    CVector rightDir = entityMatrix.right;
    upDir.z = rightDir.z = 0.0f;
    upDir.Normalize();
    rightDir.Normalize();

    CVector dir = RotateVector2D(upDir);
    CVector rightVec = RotateVector2D(rightDir);

    float halfLen = shdwSz.y * 0.5f;
    float halfWidth = shdwSz.x * 0.5f;

    CVector up = dir * halfLen;
    CVector right = rightVec * halfWidth;

    CVector worldBase = pEntity->TransformFromObjectSpace(position);
    CVector shdwPos = worldBase + dir * shdwOffset.y + rightVec * shdwOffset.x;

    if (pEntity->m_nType == ENTITY_TYPE_VEHICLE && static_cast<CVehicle *>(pEntity)->m_nVehicleSubClass == VEHICLE_BIKE)
    {
        CBike *pBike = static_cast<CBike *>(pEntity);
        bool wasCalculated = pBike->m_bLeanMatrixCalculated;
        if (!wasCalculated)
        {
            pBike->CalculateLeanMatrix();
        }
        shdwPos = pBike->m_mLeanMatrix * position + dir * shdwOffset.y + rightVec * shdwOffset.x;
        pBike->m_bLeanMatrixCalculated = wasCalculated;
    }

    // Engine's StoreShadowToBeRendered natively casts downward along fZDistance (6.0f)
    shdwPos.z = vehPos.z + 1.5f;

    RwTexture *pTex = (pTexture != nullptr)
                          ? pTexture
                          : TextureMgr::Get(shadwTexName);

    if (pTex)
    {
        float shadowIntensityFactor = static_cast<float>(col.a) / 255.0f;
        unsigned char r = static_cast<unsigned char>(col.r * shadowIntensityFactor);
        unsigned char g = static_cast<unsigned char>(col.g * shadowIntensityFactor);
        unsigned char b = static_cast<unsigned char>(col.b * shadowIntensityFactor);

        CShadows::StoreShadowToBeRendered(2, pTex, &shdwPos,
                                          up.x, up.y,
                                          right.x, right.y,
                                          255, r, g, b,
                                          6.0f, false, 1.0f, 0, true);
    }
}

eCoronaType RenderUtil::ParseCoronaType(std::string_view name)
{
    if (name.empty()) return CORONATYPE_SHINYSTAR;
    if (name == "torus" || name == "coronaringb" || name == "ring") return CORONATYPE_TORUS;
    if (name == "headlightline" || name == "line") return CORONATYPE_HEADLIGHTLINE;
    if (name == "none") return CORONATYPE_NONE;
    return CORONATYPE_SHINYSTAR;
}

std::string_view RenderUtil::GetCoronaTypeName(eCoronaType type)
{
    switch (type)
    {
    case CORONATYPE_TORUS: return "torus";
    case CORONATYPE_HEADLIGHTLINE: return "headlightline";
    case CORONATYPE_NONE: return "none";
    case CORONATYPE_SHINYSTAR:
    default:
        return "shinystar";
    }
}
