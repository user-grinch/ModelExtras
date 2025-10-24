#include "pch.h"
#include "render.h"
#include "util.h"

#include <CCoronas.h>
#include <CShadows.h>
#include <CBike.h>
#include <CWorld.h>
#include "texmgr.h"
#include "defines.h"
#include "vehicle/core/dummy.h"
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
    shadowDir.Normalise();

    // Vector from dummy to vehicle center
    CVector toVehicle = vehicleCenter - dummyPos;
    toVehicle.z = 0.0f;
    toVehicle.Normalise();

    // If dot > 0, shadow is cast toward vehicle
    return DotProduct(shadowDir, toVehicle) > 0.0f;
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
    float alignment = DotProduct(forward, up);
    return alignment > 0.7f;
}

void RenderUtil::RegisterCorona(CEntity *pEntity, int coronaID, CVector pos, CRGBA col, float size)
{
    if (!gConfig.ReadBoolean("VEHICLE_FEATURES", "LightCoronas", false))
    {
        return;
    }

    CCoronas::RegisterCorona(coronaID, pEntity, col.r, col.g, col.b, col.a, pos,
                             size * CORONA_SZ_MUL, 260.0f, CORONATYPE_SHINYSTAR, FLARETYPE_NONE, true, false, 0, 0.0f, false, 0.3f, 0, 30.0f, false, false);
};

void RenderUtil::RegisterCoronaDirectional(const VehicleDummyConfig *pConfig, float angle, float radius, float szMul, bool checks, bool inversed)
{
    const float FADE_RANGE = 20.0f;
    float sz = pConfig->corona.size * szMul;
    CRGBA col = pConfig->corona.color;

    CMatrix mat = *(CMatrix *)&pConfig->frame->ltm;
    if (!IsDummyPointingUp(mat))
    {
        if (checks && IsShadowTowardVehicle((CMatrix *)&pConfig->frame->ltm, pConfig->pVeh->GetPosition()))
        {
            angle += 180.0f;
        }

        if (inversed)
        {
            angle += 180.0f;
        }

        float vehicleAngle = Util::NormalizeAngle(Util::RadToDeg(pConfig->pVeh->GetHeading()));
        float cameraAngle = Util::NormalizeAngle(Util::RadToDeg(TheCamera.GetHeading()));
        float dummyAngle = Util::NormalizeAngle(vehicleAngle + angle);
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
            col.a *= mul;
        }
        else if (diffAngle > (360.0f - cutoff - FADE_RANGE))
        {
            float adjustedAngle = FADE_RANGE - (diffAngle - (360.0f - cutoff - FADE_RANGE));
            float mul = std::fabs(adjustedAngle / FADE_RANGE);
            col.a *= mul;
        }

        if (IsShadowTowardVehicle(&mat, pConfig->pVeh->GetPosition()))
        {
            RotateMatrix180Z(mat);
        }
        CPointLights::AddLight(PLTYPE_SPOTLIGHT, mat.pos, mat.up, 10.0f, col.r / 255.0, col.g / 255.0, col.b / 255.0, 0, 0, 0);
    }
    RegisterCorona(pConfig->pVeh, reinterpret_cast<int32_t>(pConfig), pConfig->position, col, sz);
}

extern int gGlobalShadowIntensity;

void RenderUtil::RegisterShadowDirectional(const VehicleDummyConfig *pConfig, const std::string &shadwTexName, float shdwSz)
{
    const float SHDW_SZ_MUL = 2.0f;
    if (!pConfig->pVeh || !pConfig || shdwSz == 0.0f || !gConfig.ReadBoolean("VEHICLE_FEATURES", "LightShadows", false))
    {
        return;
    }

    float lightHeightLimit = gConfig.ReadBoolean("TWEAKS", "LightHeightLimit", false);
    if (lightHeightLimit != 0.0f && pConfig->frame->modelling.pos.z >= lightHeightLimit)
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
    CMatrix vehMat = *(CMatrix *)pConfig->pVeh->GetMatrix();
    CVector worldOffset = mat.pos - vehMat.pos; // world-space vector from vehicle to dummy

    // Apply inverse rotation manually
    CVector dummyOffset;
    dummyOffset.x = DotProduct(worldOffset, vehMat.right);
    dummyOffset.y = DotProduct(worldOffset, vehMat.up);
    dummyOffset.z = DotProduct(worldOffset, vehMat.at);

    if (pConfig->mirroredX)
    {
        dummyOffset.x *= -1.0f;
    }

    // Light direction from dummy (forward vector)
    CVector lightDir = mat.up; // up is forward in psdk
    lightDir.z = 0.0f;
    lightDir.Normalise();

    CVector rotatedLightDir = lightDir;

    // Rotate dummy offset into world space
    CVector2D localOffset(dummyOffset.x, dummyOffset.y);
    CVector2D rotatedOffset = Rotate2D(localOffset, heading);

    // Push shadow forward along light direction
    rotatedOffset += CVector(rotatedLightDir.x, rotatedLightDir.y, 0.0f) * (shdwSz * SHDW_SZ_MUL + 0.2f);

    CVector2D shdwFront(rotatedLightDir.x * (shdwSz * SHDW_SZ_MUL), rotatedLightDir.y * (shdwSz * SHDW_SZ_MUL));
    CVector2D perpVec(rotatedLightDir.x * shdwSz, rotatedLightDir.y * shdwSz);
    CVector2D shdwSide = GetPerpRight(perpVec);

    RwTexture *pTex = TextureMgr::Get(shadwTexName, gGlobalShadowIntensity);
    if (!pTex)
    {
        return;
    }

    CVector shdwPos = pConfig->pVeh->GetPosition() + CVector(rotatedOffset.x, rotatedOffset.y, 2.0f);
    CShadows::StoreCarLightShadow(
        pConfig->pVeh,
        reinterpret_cast<int32_t>(pConfig),
        pTex,
        &shdwPos,
        shdwFront.x, shdwFront.y,
        shdwSide.x, shdwSide.y,
        pConfig->shadow.color.r,
        pConfig->shadow.color.g,
        pConfig->shadow.color.b,
        7.0f);
}

void RenderUtil::RegisterShadow(CEntity *pEntity, CVector position, CRGBA col, float angle,
                                eDummyPos dummyPos, const std::string &shadwTexName,
                                CVector2D shdwSz, CVector2D shdwOffset, RwTexture *pTexture)
{
    if (shdwSz.x == 0.0f || shdwSz.y == 0.0f ||
        !gConfig.ReadBoolean("VEHICLE_FEATURES", "LightShadows", false))
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
    upDir.Normalise();
    rightDir.Normalise();

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