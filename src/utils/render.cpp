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
#include <CPointLights.h>

inline CVector2D GetPerpRight(const CVector2D &vec)
{
    return {vec.y, -vec.x};
}

inline CVector2D Rotate2D(const CVector2D &vec, float angle)
{
    float cosA = cos(angle);
    float sinA = sin(angle);
    return CVector2D(
        vec.x * cosA - vec.y * sinA,
        vec.x * sinA + vec.y * cosA);
}

bool IsShadowTowardVehicle(CMatrix *dummyMatrix, CVector vehicleCenter)
{
    // Dummy world position
    CVector dummyPos = dummyMatrix->pos;

    // Shadow direction (assume 'up' is forward in dummy frame)
    CVector shadowDir = dummyMatrix->up;
    shadowDir.z = 0.0f;
    shadowDir.Normalize();

    // Vector from dummy to vehicle center
    CVector toVehicle = vehicleCenter - dummyPos;
    toVehicle.z = 0.0f;
    toVehicle.Normalize();

    // If dot > 0, shadow is cast toward vehicle
    return CVector::Dot(shadowDir, toVehicle) > 0.0f;
}

void RotateMatrix180Z(CMatrix &mat)
{
    // Flip X and Y of right and up vectors
    mat.right.x = -mat.right.x;
    mat.right.y = -mat.right.y;

    mat.up.x = -mat.up.x;
    mat.up.y = -mat.up.y;

    // forward stays unchanged (Z axis)
}

bool IsDummyPointingUp(CMatrix mat)
{
    CVector forward = mat.up;
    CVector up = {0.0f, 0.0f, 1.0f};
    float alignment = CVector::Dot(forward, up);
    return alignment > 0.7f;
}

static bool gbLightCoronas = false;
static bool gbLightShadows = false;
static float gfCoronaDistanceMul = 0.0f;
static float gfCoronaNearClip = 0.45f;
static float gfLightHeightLimit = 0.0f;
static bool gbConfigInitialized = false;

static void EnsureConfigLoaded()
{
    if (!gbConfigInitialized)
    {
        gbLightCoronas = gConfig.ReadBoolean("LIGHTS", "LightCoronas", gConfig.ReadBoolean("FEATURES", "LightCoronas", false));
        gbLightShadows = gConfig.ReadBoolean("LIGHTS", "LightShadows", gConfig.ReadBoolean("FEATURES", "LightShadows", false));
        gfCoronaDistanceMul = gConfig.ReadFloat("LIGHTS", "CoronaDistanceMul", gConfig.ReadFloat("TWEAKS", "CoronaDistanceMul", 0.0f));
        gfCoronaNearClip = gConfig.ReadFloat("LIGHTS", "CoronaNearClip", gConfig.ReadFloat("TWEAKS", "CoronaNearClip", 0.45f));
        gfLightHeightLimit = gConfig.ReadFloat("LIGHTS", "LightHeightLimit", gConfig.ReadFloat("TWEAKS", "LightHeightLimit", 0.0f));
        gbConfigInitialized = true;
    }
}

void RenderUtil::RegisterCorona(CEntity *pEntity, int coronaID, CVector pos, CRGBA col, float size)
{
    EnsureConfigLoaded();
    if (!gbLightCoronas)
    {
        return;
    }

    float coronaSz = size;

    // Only during night time
    if (Util::IsNightTime() && gfCoronaDistanceMul != 0.0f) {
        // pEntity is null for unattached coronas, pos is already in world space then
        CVector refPos = pEntity ? pEntity->GetPosition() : pos;
        coronaSz *= CVector::Distance(TheCamera.GetPosition(), refPos) * gfCoronaDistanceMul;
    }

    CCoronas::RegisterCorona(coronaID, pEntity, col.r, col.g, col.b, col.a, pos,
                             coronaSz, 260.0f, CORONATYPE_SHINYSTAR, FLARETYPE_NONE, true, false, 0, 0.0f, false, gfCoronaNearClip, 0, 30.0f, false, false);
};

// Vanilla reach of the headlight spotlight
static const float HEADLIGHT_PLIGHT_RANGE = 20.0f;

void RenderUtil::RegisterHeadlightPointLight(const DummyConfig *pConfig, float rangeMul)
{
    extern bool gbLightPointLights;
    if (!gbLightPointLights || !pConfig || !pConfig->frame || !pConfig->pVeh)
    {
        return;
    }

    bool isBike = pConfig->pVeh->m_nVehicleSubClass == VEHICLE_BIKE;
    if (!isBike && pConfig->pVeh->GetIsOnScreen())
    {
        RwFrame *parent = pConfig->frame ? RwFrameGetParent(pConfig->frame) : nullptr;
        if (Util::IsFrameDamaged(pConfig->pVeh, parent) || !FrameUtil::IsOkAtomicVisible(parent))
        {
            return;
        }
    }

    bool isLeft = (pConfig->lightType == eMaterialType::HeadLightLeft || pConfig->position.x < 0.0f);
    eLights lightEnum = isLeft ? eLights::LIGHT_FRONT_LEFT : eLights::LIGHT_FRONT_RIGHT;
    ePanels wingEnum = isLeft ? ePanels::WING_FRONT_LEFT : ePanels::WING_FRONT_RIGHT;
    if (Util::IsLightDamaged(pConfig->pVeh, lightEnum) || Util::IsPanelDamaged(pConfig->pVeh, wingEnum))
    {
        return;
    }

    CMatrix mat = *(CMatrix *)&pConfig->frame->ltm;

    // An upward facing dummy isn't a headlight pointing down the road
    if (IsDummyPointingUp(mat))
    {
        return;
    }

    CVector lightPos = pConfig->pVeh->TransformFromObjectSpace(pConfig->shadow.position);

    CMatrix vehMat = pConfig->pVeh->GetMatrix();
    CVector localDir;
    localDir.x = CVector::Dot(mat.up, vehMat.right);
    localDir.y = CVector::Dot(mat.up, vehMat.up);
    localDir.z = CVector::Dot(mat.up, vehMat.at);
    if (pConfig->mirroredX) localDir.x = -localDir.x;
    if (localDir.y < 0.2f) localDir.y = 1.0f;
    if (localDir.z > -0.15f) localDir.z = -0.15f;
    localDir.Normalize();

    CVector lightDir = vehMat.right * localDir.x + vehMat.up * localDir.y + vehMat.at * localDir.z;
    lightDir.Normalize();

    CRGBA col = pConfig->corona.color;
    CPointLights::AddLight(PLTYPE_SPOTLIGHT, lightPos, lightDir, HEADLIGHT_PLIGHT_RANGE * rangeMul,
                           col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, 0, 0, 0);
}

void RenderUtil::RegisterPointLight(const DummyConfig *pConfig, CRGBA col, float radius, bool isSpotlight)
{
    extern bool gbLightPointLights;
    if (!gbLightPointLights || !pConfig || !pConfig->frame || !pConfig->pVeh || radius <= 0.0f)
    {
        return;
    }

    float distToCam = CVector::Distance(pConfig->pVeh->GetPosition(), TheCamera.GetPosition());
    if (distToCam > 75.0f)
    {
        return;
    }

    bool isBike = pConfig->pVeh->m_nVehicleSubClass == VEHICLE_BIKE;
    if (!isBike && pConfig->pVeh->GetIsOnScreen())
    {
        RwFrame *parent = pConfig->frame ? RwFrameGetParent(pConfig->frame) : nullptr;
        if (Util::IsFrameDamaged(pConfig->pVeh, parent) || !FrameUtil::IsOkAtomicVisible(parent))
        {
            return;
        }
    }

    bool isSirenFlashing = pConfig->pVeh->bSirenOrAlarm;

    // Front Left
    if (pConfig->lightType == eMaterialType::HeadLightLeft || pConfig->lightType == eMaterialType::FogLightLeft)
    {
        if (Util::IsLightDamaged(pConfig->pVeh, eLights::LIGHT_FRONT_LEFT) || Util::IsPanelDamaged(pConfig->pVeh, ePanels::WING_FRONT_LEFT))
            return;
    }
    else if (pConfig->lightType == eMaterialType::IndicatorLightLeftFront)
    {
        if (!isSirenFlashing && (Util::IsLightDamaged(pConfig->pVeh, eLights::LIGHT_FRONT_LEFT) || Util::IsPanelDamaged(pConfig->pVeh, ePanels::WING_FRONT_LEFT)))
            return;
    }
    // Front Right
    else if (pConfig->lightType == eMaterialType::HeadLightRight || pConfig->lightType == eMaterialType::FogLightRight)
    {
        if (Util::IsLightDamaged(pConfig->pVeh, eLights::LIGHT_FRONT_RIGHT) || Util::IsPanelDamaged(pConfig->pVeh, ePanels::WING_FRONT_RIGHT))
            return;
    }
    else if (pConfig->lightType == eMaterialType::IndicatorLightRightFront)
    {
        if (!isSirenFlashing && (Util::IsLightDamaged(pConfig->pVeh, eLights::LIGHT_FRONT_RIGHT) || Util::IsPanelDamaged(pConfig->pVeh, ePanels::WING_FRONT_RIGHT)))
            return;
    }
    // Rear Left (TailLight, BrakeLight, ReverseLight, Rear Indicator, NABrakeLight, STTLight)
    else if (pConfig->lightType == eMaterialType::TailLightLeft || pConfig->lightType == eMaterialType::BrakeLightLeft ||
             pConfig->lightType == eMaterialType::ReverseLightLeft || pConfig->lightType == eMaterialType::IndicatorLightLeftRear ||
             pConfig->lightType == eMaterialType::NABrakeLightLeft || pConfig->lightType == eMaterialType::STTLightLeft)
    {
        if (Util::IsLightDamaged(pConfig->pVeh, eLights::LIGHT_REAR_LEFT) || Util::IsPanelDamaged(pConfig->pVeh, ePanels::WING_REAR_LEFT))
            return;
    }
    // Rear Right (TailLight, BrakeLight, ReverseLight, Rear Indicator, NABrakeLight, STTLight)
    else if (pConfig->lightType == eMaterialType::TailLightRight || pConfig->lightType == eMaterialType::BrakeLightRight ||
             pConfig->lightType == eMaterialType::ReverseLightRight || pConfig->lightType == eMaterialType::IndicatorLightRightRear ||
             pConfig->lightType == eMaterialType::NABrakeLightRight || pConfig->lightType == eMaterialType::STTLightRight)
    {
        if (Util::IsLightDamaged(pConfig->pVeh, eLights::LIGHT_REAR_RIGHT) || Util::IsPanelDamaged(pConfig->pVeh, ePanels::WING_REAR_RIGHT))
            return;
    }
    // Side Indicators & Side Lights
    else if (pConfig->lightType == eMaterialType::SideLightLeft || pConfig->lightType == eMaterialType::IndicatorLightLeftMiddle)
    {
        if (Util::IsPanelDamaged(pConfig->pVeh, ePanels::WING_FRONT_LEFT) || Util::IsPanelDamaged(pConfig->pVeh, ePanels::WING_REAR_LEFT))
            return;
    }
    else if (pConfig->lightType == eMaterialType::SideLightRight || pConfig->lightType == eMaterialType::IndicatorLightRightMiddle)
    {
        if (Util::IsPanelDamaged(pConfig->pVeh, ePanels::WING_FRONT_RIGHT) || Util::IsPanelDamaged(pConfig->pVeh, ePanels::WING_REAR_RIGHT))
            return;
    }

    CMatrix mat = *(CMatrix *)&pConfig->frame->ltm;
    if (IsDummyPointingUp(mat))
    {
        return;
    }

    CMatrix vehMat = pConfig->pVeh->GetMatrix();
    CVector lightPos = pConfig->pVeh->TransformFromObjectSpace(pConfig->shadow.position);

    // Extract dummy forward vector directly in vehicle space (respecting modder's 3D rotation)
    CVector localDir;
    localDir.x = CVector::Dot(mat.up, vehMat.right);
    localDir.y = CVector::Dot(mat.up, vehMat.up);
    localDir.z = CVector::Dot(mat.up, vehMat.at);

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
    if (isExplicitRearType || (pConfig->shadow.position.y < -0.3f && localDir.y > 0.7f && std::abs(localDir.x) < 0.3f))
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
    if (pConfig->lightType == eMaterialType::FogLightLeft || pConfig->lightType == eMaterialType::FogLightRight)
    {
        pushDist = 0.65f;
    }
    else if (pConfig->lightType == eMaterialType::ReverseLightLeft || pConfig->lightType == eMaterialType::ReverseLightRight)
    {
        pushDist = 0.55f;
    }
    else if (pConfig->lightType == eMaterialType::BrakeLightLeft || pConfig->lightType == eMaterialType::BrakeLightRight ||
             pConfig->lightType == eMaterialType::NABrakeLightLeft || pConfig->lightType == eMaterialType::NABrakeLightRight)
    {
        pushDist = 0.5f;
    }
    else if (pConfig->lightType == eMaterialType::IndicatorLightLeftFront || pConfig->lightType == eMaterialType::IndicatorLightRightFront ||
             pConfig->lightType == eMaterialType::IndicatorLightLeftRear || pConfig->lightType == eMaterialType::IndicatorLightRightRear ||
             pConfig->lightType == eMaterialType::IndicatorLightLeftMiddle || pConfig->lightType == eMaterialType::IndicatorLightRightMiddle)
    {
        pushDist = 0.4f;
    }
    else if (pConfig->lightType == eMaterialType::SideLightLeft || pConfig->lightType == eMaterialType::SideLightRight ||
             pConfig->lightType == eMaterialType::AllDayLight || pConfig->lightType == eMaterialType::DayLight || pConfig->lightType == eMaterialType::NightLight)
    {
        pushDist = 0.25f;
    }

    CVector plightPos = lightPos + lightDir * pushDist;

    float r = std::clamp(col.r / 255.0f, 0.0f, 1.0f);
    float g = std::clamp(col.g / 255.0f, 0.0f, 1.0f);
    float b = std::clamp(col.b / 255.0f, 0.0f, 1.0f);

    unsigned char lightType = isSpotlight ? PLTYPE_SPOTLIGHT : PLTYPE_POINTLIGHT;
    CPointLights::AddLight(lightType, plightPos, lightDir, radius, r, g, b, 0, false, nullptr);
}

void RenderUtil::RegisterCoronaDirectional(const DummyConfig *pConfig, float angle, float radius, float szMul, bool inversed, bool skipCheck)
{
    const float FADE_RANGE = 20.0f;
    float sz = pConfig->corona.size * szMul;
    CRGBA col = pConfig->corona.color;

    CMatrix mat = *(CMatrix *)&pConfig->frame->ltm;
    if (!IsDummyPointingUp(mat))
    {
        float targetAngle = angle;
        if (pConfig->lightType == eMaterialType::HeadLightLeft || pConfig->lightType == eMaterialType::HeadLightRight
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

        if (diffAngle < cutoff || diffAngle > (360.0f - cutoff))
        {
            return;
        }

        if (diffAngle < cutoff + FADE_RANGE)
        {
            float adjustedAngle = cutoff - diffAngle;
            float mul = std::fabs(adjustedAngle / FADE_RANGE);
            col.a = static_cast<unsigned char>(col.a * mul);
        }
        else if (diffAngle > (360.0f - cutoff - FADE_RANGE))
        {
            float adjustedAngle = FADE_RANGE - (diffAngle - (360.0f - cutoff - FADE_RANGE));
            float mul = std::fabs(adjustedAngle / FADE_RANGE);
            col.a = static_cast<unsigned char>(col.a * mul);
        }
    }
    RegisterCorona(pConfig->pVeh, reinterpret_cast<int32_t>(pConfig), pConfig->position, col, sz);
}

extern int gGlobalShadowIntensity;

void RenderUtil::RegisterShadowDirectional(const DummyConfig *pConfig, const std::string &shadwTexName, float shdwSz)
{
    EnsureConfigLoaded();
    const float SHDW_SZ_MUL = 2.0f;
    const float SHDW_MAX_DIST = 120.0f;
    const float SHDW_FADE_DIST = 70.0f;
    if (!pConfig || !pConfig->pVeh || shdwSz == 0.0f || !gbLightShadows)
    {
        return;
    }

    // Cull first, the shadow isn't visible past this range anyway
    float distToCam = CVector::Distance(pConfig->pVeh->GetPosition(), TheCamera.GetPosition());
    if (distToCam > SHDW_MAX_DIST)
    {
        return;
    }

    extern bool gbProperShadersDetected;
    extern bool gbLightPointLights;
    if (gbProperShadersDetected && (gbLightPointLights || pConfig->lightType == eMaterialType::HeadLightLeft || pConfig->lightType == eMaterialType::HeadLightRight))
    {
        return;
    }

    if (gfLightHeightLimit != 0.0f && pConfig->frame->modelling.pos.z >= gfLightHeightLimit)
    {
        return;
    }

    if (IsDummyPointingUp(*(CMatrix *)&pConfig->frame->ltm))
    {
        return;
    }

    CMatrix mat = *(CMatrix *)&pConfig->frame->ltm;
    if (pConfig->shadow.rotationChecks && IsShadowTowardVehicle((CMatrix *)&pConfig->frame->ltm, pConfig->pVeh->GetPosition()))
    {
        RotateMatrix180Z(mat);
    }

    CMatrix vehMat = pConfig->pVeh->GetMatrix();

    // Calculate correct world position using shadow.position (which contains mirroredX translation)
    CVector worldPos = pConfig->pVeh->TransformFromObjectSpace(pConfig->shadow.position);

    // Expand through bike lean matrix when available so roll angle matches the chassis
    if (pConfig->pVeh->m_nVehicleSubClass == VEHICLE_BIKE && pConfig->leanAffected)
    {
        CBike *pBike = static_cast<CBike *>(pConfig->pVeh);
        bool wasCalculated = pBike->m_bLeanMatrixCalculated;
        if (!wasCalculated)
        {
            pBike->CalculateLeanMatrix();
        }

        worldPos = pBike->m_mLeanMatrix * pConfig->shadow.position;
        pBike->m_bLeanMatrixCalculated = wasCalculated;
    }

    // 3D light direction vectors directly from dummy matrix (retaining full 3D roll, pitch, and yaw)
    CVector lightDir = mat.up;
    CVector rightDir = mat.right;
    if (pConfig->mirroredX)
    {
        rightDir = -rightDir;
    }

    // Push shadow forward along light direction
    CVector shdwCenter = worldPos + lightDir * (shdwSz * SHDW_SZ_MUL + 0.2f);

    CVector2D shdwFront(lightDir.x * (shdwSz * SHDW_SZ_MUL), lightDir.y * (shdwSz * SHDW_SZ_MUL));
    CVector2D shdwSide(rightDir.x * shdwSz, rightDir.y * shdwSz);

    RwTexture *pTex = TextureMgr::Get(shadwTexName, gGlobalShadowIntensity);
    if (!pTex)
    {
        return;
    }

    CVector shdwPos(shdwCenter.x, shdwCenter.y, pConfig->pVeh->GetPosition().z + 2.0f);

    // Fade towards the cutoff so distant shadows don't pop in and out
    float alphaMul = 1.0f;
    if (distToCam > SHDW_FADE_DIST)
    {
        alphaMul = (SHDW_MAX_DIST - distToCam) / (SHDW_MAX_DIST - SHDW_FADE_DIST);
    }

    CShadows::StoreShadowToBeRendered(
        2,
        pTex,
        &shdwPos,
        shdwFront.x, shdwFront.y,
        shdwSide.x, shdwSide.y,
        static_cast<short>(255 * alphaMul),
        pConfig->shadow.color.r,
        pConfig->shadow.color.g,
        pConfig->shadow.color.b,
        7.0f,
        false,
        1.0f,
        0,
        true);
}

void RenderUtil::RegisterShadow(CEntity *pEntity, CVector position, CRGBA col, float angle,
                                eDummyPos dummyPos, const std::string &shadwTexName,
                                CVector2D shdwSz, CVector2D shdwOffset, RwTexture *pTexture)
{
    EnsureConfigLoaded();
    extern bool gbProperShadersDetected;
    extern bool gbLightPointLights;
    extern bool gbSirenPointLights;
    if (shdwSz.x == 0.0f || shdwSz.y == 0.0f || !gbLightShadows)
    {
        return;
    }
    if (gbProperShadersDetected && (gbLightPointLights && gbSirenPointLights))
    {
        return;
    }

    const float angleRad = DegToRad(angle);
    const CVector vehPos = pEntity->GetPosition();
    const CMatrix &entityMatrix = *(CMatrix *)pEntity->m_matrix;

    auto RotateVector2D = [angleRad](const CVector &v) -> CVector
    {
        return {
            v.x * cos(angleRad) - v.y * sin(angleRad),
            v.x * sin(angleRad) + v.y * cos(angleRad),
            v.z};
    };
    CVector upDir = entityMatrix.up;
    CVector rightDir = entityMatrix.right;

    upDir.z = rightDir.z = 0.0f; // Flatten vertical influence
    upDir.Normalize();
    rightDir.Normalize();

    CVector up = RotateVector2D(upDir * shdwSz.y);
    CVector right = RotateVector2D(rightDir * shdwSz.x);

    CVector nSize = {0.0f, 0.0f, 0.0f};
    switch (dummyPos)
    {
    case eDummyPos::Right:
        nSize = {shdwSz.y, 0.0f, 0.0f};
        break;
    case eDummyPos::Left:
        nSize = {-shdwSz.y, 0.0f, 0.0f};
        break;
    case eDummyPos::Front:
        nSize = {0.0f, shdwSz.y, 0.0f};
        break;
    case eDummyPos::Rear:
        nSize = {0.0f, -shdwSz.y, 0.0f};
        break;
    default:
        break;
    }

    CVector nOffset = {
        shdwOffset.x * cos(angleRad) - shdwOffset.y * sin(angleRad),
        shdwOffset.x * sin(angleRad) + shdwOffset.y * cos(angleRad),
        0.0f};

    CVector shdwPos = pEntity->TransformFromObjectSpace(position + nOffset + nSize);
    if (pEntity->m_nType == ENTITY_TYPE_VEHICLE && static_cast<CVehicle *>(pEntity)->m_nVehicleSubClass == VEHICLE_BIKE)
    {
        CBike *pBike = static_cast<CBike *>(pEntity);
        bool wasCalculated = pBike->m_bLeanMatrixCalculated;
        if (!wasCalculated)
        {
            pBike->CalculateLeanMatrix();
        }
        shdwPos = pBike->m_mLeanMatrix * (position + nOffset + nSize);
        pBike->m_bLeanMatrixCalculated = wasCalculated;
    }
    shdwPos.z = CWorld::FindGroundZFor3DCoord(shdwPos.x, shdwPos.y, shdwPos.z + 100.0f, NULL, &pEntity) + 2.0f;

    const float zDiff = abs(shdwPos.z - vehPos.z);
    if (zDiff > 3.0f)
    {
        shdwPos.z = vehPos.z + position.z + 1.0f;
    }

    if (abs(vehPos.z - shdwPos.z) > 15.0f)
        return;

    RwTexture *pTex = (pTexture != nullptr)
                          ? pTexture
                          : TextureMgr::Get(shadwTexName, gGlobalShadowIntensity);

    if (pTex)
    {
        CShadows::StoreShadowToBeRendered(2, pTex, &shdwPos,
                                          up.x, up.y,
                                          right.x, right.y,
                                          col.a, col.r, col.g, col.b,
                                          6.0f, false, 1.0f, 0, true);
    }
}