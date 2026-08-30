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
    if (!pConfig || !pConfig->frame || !pConfig->pVeh)
    {
        return;
    }

    CMatrix mat = *(CMatrix *)&pConfig->frame->ltm;

    // An upward facing dummy isn't a headlight pointing down the road
    if (IsDummyPointingUp(mat))
    {
        return;
    }

    CVector localPos = pConfig->frame->modelling.pos;
    if (pConfig->mirroredX)
    {
        localPos.x *= -1.0f;
    }

    CVector lightPos = pConfig->pVeh->TransformFromObjectSpace(localPos);
    CVector lightDir = pConfig->pVeh->GetMatrix().up;

    CRGBA col = pConfig->corona.color;
    CPointLights::AddLight(PLTYPE_SPOTLIGHT, lightPos, lightDir, HEADLIGHT_PLIGHT_RANGE * rangeMul,
                           col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, 0, 0, 0);
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
    if (gbProperShadersDetected && (pConfig->lightType == eMaterialType::HeadLightLeft || pConfig->lightType == eMaterialType::HeadLightRight))
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

    float heading = pConfig->pVeh->GetHeading();
    CMatrix mat = *(CMatrix *)&pConfig->frame->ltm;
    if (pConfig->shadow.rotationChecks && IsShadowTowardVehicle((CMatrix *)&pConfig->frame->ltm, pConfig->pVeh->GetPosition()))
    {
        RotateMatrix180Z(mat);
    }

    // Dummy offset in local space
    CMatrix vehMat = pConfig->pVeh->GetMatrix();
    CVector worldOffset = mat.pos - vehMat.pos; // world-space vector from vehicle to dummy

    // Apply inverse rotation manually
    CVector dummyOffset;
    dummyOffset.x = CVector::Dot(worldOffset, vehMat.right);
    dummyOffset.y = CVector::Dot(worldOffset, vehMat.up);
    dummyOffset.z = CVector::Dot(worldOffset, vehMat.at);

    if (pConfig->mirroredX)
    {
        dummyOffset.x *= -1.0f;
    }

    // Light direction from dummy (forward vector)
    CVector lightDir = mat.up; // up is forward in psdk
    lightDir.z = 0.0f;
    lightDir.Normalize();

    CVector rotatedLightDir = lightDir;

    // Rotate dummy offset into world space
    CVector2D localOffset(dummyOffset.x, dummyOffset.y);
    CVector2D rotatedOffset = Rotate2D(localOffset, heading);

    // Rotate2D only applies yaw, so a bike's roll never reaches the ground position and
    // the shadow stays put while it banks. shadow.position is the lean free local offset,
    // so expanding it through the lean matrix gives the light's actual leaned position.
    if (pConfig->pVeh->m_nVehicleSubClass == VEHICLE_BIKE && pConfig->leanAffected)
    {
        CBike *pBike = static_cast<CBike *>(pConfig->pVeh);
        bool wasCalculated = pBike->m_bLeanMatrixCalculated;
        if (!wasCalculated)
        {
            pBike->CalculateLeanMatrix();
        }

        CVector leaned = pBike->m_mLeanMatrix * pConfig->shadow.position;
        pBike->m_bLeanMatrixCalculated = wasCalculated;

        rotatedOffset = CVector2D(leaned.x - vehMat.pos.x, leaned.y - vehMat.pos.y);
    }

    // Push shadow forward along light direction
    rotatedOffset += CVector2D(rotatedLightDir.x, rotatedLightDir.y) * (shdwSz * SHDW_SZ_MUL + 0.2f);

    CVector2D shdwFront(rotatedLightDir.x * (shdwSz * SHDW_SZ_MUL), rotatedLightDir.y * (shdwSz * SHDW_SZ_MUL));
    CVector2D perpVec(rotatedLightDir.x * shdwSz, rotatedLightDir.y * shdwSz);
    CVector2D shdwSide = GetPerpRight(perpVec);

    RwTexture *pTex = TextureMgr::Get(shadwTexName, gGlobalShadowIntensity);
    if (!pTex)
    {
        return;
    }

    CVector shdwPos = pConfig->pVeh->GetPosition() + CVector(rotatedOffset.x, rotatedOffset.y, 2.0f);

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
    if (shdwSz.x == 0.0f || shdwSz.y == 0.0f || !gbLightShadows)
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