#include "pch.h"
#include "util.h"
#include <regex>
#include <CWeaponInfo.h>
#include <CCamera.h>
#include <CCoronas.h>
#include <CShadows.h>
#include <CBike.h>
#include <CWorld.h>
#include "texmgr.h"
#include "defines.h"

void Util::RegisterCorona(CVehicle *pVeh, int coronaID, CVector pos, CRGBA col, float size)
{
    if (!gConfig.ReadBoolean("VEHICLE_FEATURES", "LightCoronas", false))
    {
        return;
    }

    CCoronas::RegisterCorona(coronaID, pVeh, col.r, col.g, col.b, col.a, pos,
                             size * CORONA_SZ_MUL, 260.0f, CORONATYPE_SHINYSTAR, FLARETYPE_NONE, false, false, 0, 0.0f, false, 0.3f, 0, 30.0f, false, false);
};

float Util::NormalizeAngle(float angle)
{
    while (angle < 0.0f)
        angle += 360.0f;
    while (angle >= 360.0f)
        angle -= 360.0f;
    return angle;
}

void Util::RegisterCoronaWithAngle(CVehicle *pVeh, int coronaID, CVector posn, CRGBA col, float angle, float radius, float size)
{
    constexpr float RAD_TO_DEG = 180.0f / 3.141592653589793f;

    float vehicleAngle = NormalizeAngle(pVeh->GetHeading() * RAD_TO_DEG);
    float cameraAngle = NormalizeAngle(TheCamera.GetHeading() * RAD_TO_DEG);
    float dummyAngle = NormalizeAngle(vehicleAngle + angle);
    float fadeRange = 20.0f;
    float cutoff = (radius / 2.0f);
    float diffAngle = std::fabs(std::fmod(std::fabs(cameraAngle - dummyAngle) + 180.0f, 360.0f) - 180.0f);

    if (diffAngle < cutoff || diffAngle > (360.0f - cutoff))
    {
        return;
    }

    if (diffAngle < cutoff + fadeRange)
    {
        float adjustedAngle = cutoff - diffAngle;
        float mul = std::fabs(adjustedAngle / fadeRange);
        col.a *= mul;
    }
    else if (diffAngle > (360.0f - cutoff - fadeRange))
    {
        float adjustedAngle = fadeRange - (diffAngle - (360.0f - cutoff - fadeRange));
        float mul = std::fabs(adjustedAngle / fadeRange);
        col.a *= mul;
    }

    RegisterCorona(pVeh, coronaID, posn, col, size);
}

void Util::RegisterShadow(CVehicle *pVeh, CVector position, CRGBA col, float angle, float currentAngle, const std::string &shadwTexName, CVector2D shdwSz, CVector2D shdwOffset, RwTexture *pTexture)
{
    if (shdwSz.x == 0.0f || shdwSz.y == 0.0f || !gConfig.ReadBoolean("VEHICLE_FEATURES", "LightShadows", false))
    {
        return;
    }

    float fAngle = pVeh->GetHeading() + (DegToRad(angle + currentAngle + 180.0f));
    CVector vehPos = pVeh->GetPosition();

    CVector up = CVector(-sin(fAngle), cos(fAngle), 0.0f);
    CVector right = CVector(cos(fAngle), sin(fAngle), 0.0f);
    up *= shdwSz.y;
    right *= shdwSz.x;

    CVector center = pVeh->TransformFromObjectSpace(
        CVector(
            position.x + (shdwOffset.x + shdwSz.x * cos(DegToRad(90.0f - angle + currentAngle))),
            position.y + (shdwOffset.y + shdwSz.y * sin(DegToRad(90.0f - angle + currentAngle))),
            position.z));

    center.z = CWorld::FindGroundZFor3DCoord(center.x, center.y, center.z + 100, nullptr, nullptr) + 1.0f;

    // Fix issues like under bridges
    if (abs(center.z - vehPos.z) > 3.0f)
    {
        center.z = vehPos.z + position.z + 1.0f;
    }

    // Fix heli drawing shadow from sky
    if (abs(vehPos.z - center.z) > 15.0f)
    {
        return;
    }

    CShadows::StoreShadowToBeRendered(2, (pTexture != NULL ? pTexture : TextureMgr::Get(shadwTexName, 30)), &center,
                                      up.x, up.y,
                                      right.x, right.y,
                                      col.a, col.r, col.g, col.b,
                                      10.0f, false, 1.0f, 0, true);
}

float Util::GetVehicleSpeed(CVehicle *pVeh)
{
    return pVeh->m_vecMoveSpeed.Magnitude2D() * 50.0f;
}

std::string Util::GetRegexVal(const std::string &src, const std::string &&ptrn, const std::string &&def)
{
    std::smatch match;
    std::regex_search(src.begin(), src.end(), match, std::regex(ptrn));

    if (match.empty())
        return def;
    else
        return match[1];
}

void Util::SetFrameRotationX(RwFrame *frame, float angle)
{
    RwFrameRotate(frame, (RwV3d *)0x008D2E00, (RwReal)angle, rwCOMBINEPRECONCAT);
    RwFrameUpdateObjects(frame);
}

void Util::SetFrameRotationY(RwFrame *frame, float angle)
{
    RwFrameRotate(frame, (RwV3d *)0x008D2E0C, (RwReal)angle, rwCOMBINEPRECONCAT);
    RwFrameUpdateObjects(frame);
}

void Util::SetFrameRotationZ(RwFrame *frame, float angle)
{
    RwFrameRotate(frame, (RwV3d *)0x008D2E18, (RwReal)angle, rwCOMBINEPRECONCAT);
    RwFrameUpdateObjects(frame);
}

float GetATanOfXY(float x, float y)
{
    if (x > 0.0f)
    {
        return atan2(y, x);
    }
    else if (x < 0.0f)
    {
        if (y >= 0.0f)
        {
            return atan2(y, x) + 3.1416f;
        }
        else
        {
            return atan2(y, x) - 3.1416f;
        }
    }
    else
    { // x is 0.0f
        if (y > 0.0f)
        {
            return 0.5f * 3.1416f;
        }
        else if (y < 0.0f)
        {
            return -0.5f * 3.1416f;
        }
        else
        {
            // x and y are both 0, undefined result
            return 0.0f;
        }
    }
}

float Util::GetMatrixRotationX(RwMatrix *matrix)
{
    float x = matrix->right.x;
    float y = matrix->right.y;
    float z = matrix->right.z;
    float angle = GetATanOfXY(z, sqrt(x * x + y * y)) * 57.295776f - 90.0f;
    while (angle < 0.0)
        angle += 360.0;
    return angle;
}

float Util::GetMatrixRotationY(RwMatrix *matrix)
{
    float x = matrix->up.x;
    float y = matrix->up.y;
    float z = matrix->up.z;
    float angle = GetATanOfXY(z, sqrt(x * x + y * y)) * 57.295776f - 90.0f;
    while (angle < 0.0)
        angle += 360.0;

    return angle;
}

float Util::GetMatrixRotationZ(RwMatrix *matrix)
{
    if (!matrix)
        return 0.0f;

    float angle = GetATanOfXY(matrix->right.x, matrix->right.y) * 57.295776f - 90.0f;
    while (angle < 0.0)
        angle += 360.0;
    return angle;
}

void Util::ResetMatrixRotations(RwMatrix *matrix)
{
    matrix->right = {1.0f, 0.0f, 0.0f};
    matrix->up = {0.0f, 1.0f, 0.0f};
    matrix->at = {0.0f, 0.0f, 1.0f};
}

void Util::SetMatrixRotationX(RwMatrix *matrix, float angle)
{
    angle -= GetMatrixRotationX(matrix);

    // Ensure the angle is within [0, 360) range
    while (angle >= 360.0f)
        angle -= 360.0f;

    while (angle < 0.0f)
        angle += 360.0f;

    // Convert angle to radians
    float angleRad = angle / 57.295776f;

    // Calculate the sine and cosine of the angle
    float sinAngle = sin(angleRad);
    float cosAngle = cos(angleRad);

    // Store the existing up and at vectors
    RwV3d up = matrix->up;
    RwV3d at = matrix->at;

    // Update the up and at vectors for the X-axis rotation
    matrix->up.x = cosAngle * up.x + sinAngle * at.x;
    matrix->up.y = cosAngle * up.y + sinAngle * at.y;
    matrix->up.z = cosAngle * up.z + sinAngle * at.z;

    matrix->at.x = -sinAngle * up.x + cosAngle * at.x;
    matrix->at.y = -sinAngle * up.y + cosAngle * at.y;
    matrix->at.z = -sinAngle * up.z + cosAngle * at.z;

    // Normalize the vectors to ensure they remain orthogonal
    RwV3dNormalize(&matrix->up, &matrix->up);
    RwV3dNormalize(&matrix->at, &matrix->at);
}

void Util::SetMatrixRotationY(RwMatrix *matrix, float angle)
{
    angle -= GetMatrixRotationY(matrix);

    // Ensure the angle is within [0, 360) range
    while (angle >= 360.0f)
        angle -= 360.0f;

    while (angle < 0.0f)
        angle += 360.0f;

    // Convert angle to radians
    float angleRad = angle / 57.295776f;

    // Calculate the sine and cosine of the angle
    float sinAngle = sin(angleRad);
    float cosAngle = cos(angleRad);

    // Store the existing right and at vectors
    RwV3d right = matrix->right;
    RwV3d at = matrix->at;

    // Update the right and at vectors for the Y-axis rotation
    matrix->right.x = cosAngle * right.x + sinAngle * at.x;
    matrix->right.y = cosAngle * right.y + sinAngle * at.y;
    matrix->right.z = cosAngle * right.z + sinAngle * at.z;

    matrix->at.x = -sinAngle * right.x + cosAngle * at.x;
    matrix->at.y = -sinAngle * right.y + cosAngle * at.y;
    matrix->at.z = -sinAngle * right.z + cosAngle * at.z;

    // Normalize the vectors to ensure they remain orthogonal
    RwV3dNormalize(&matrix->right, &matrix->right);
    RwV3dNormalize(&matrix->at, &matrix->at);
}

void Util::SetMatrixRotationZ(RwMatrix *matrix, float angle)
{
    angle -= GetMatrixRotationZ(matrix);

    // Ensure the angle is within [0, 360) range
    while (angle >= 360.0f)
        angle -= 360.0f;

    while (angle < 0.0f)
        angle += 360.0f;

    // Convert angle to radians
    float angleRad = angle / 57.295776f;

    // Calculate the sine and cosine of the angle
    float sinAngle = sin(angleRad);
    float cosAngle = cos(angleRad);

    // Store the existing right and up vectors
    RwV3d right = matrix->right;
    RwV3d up = matrix->up;

    // Set the right vector for the Z-axis rotation
    matrix->right.x = cosAngle * right.x - sinAngle * right.y;
    matrix->right.y = sinAngle * right.x + cosAngle * right.y;

    // Set the up vector for the Z-axis rotation
    matrix->up.x = cosAngle * up.x - sinAngle * up.y;
    matrix->up.y = sinAngle * up.x + cosAngle * up.y;

    // Normalize the vectors to ensure they remain orthogonal
    RwV3dNormalize(&matrix->right, &matrix->right);
    RwV3dNormalize(&matrix->up, &matrix->up);
}

uint32_t Util::GetChildCount(RwFrame *parent)
{
    RwFrame *child = parent->child;
    uint32_t count = 0U;
    if (child)
    {
        while (child)
        {
            ++count;
            child = child->next;
        }
        return count;
    }
    return 0U;
}

void Util::StoreChilds(RwFrame *parent, std::vector<RwFrame *> &store)
{
    RwFrame *child = parent->child;

    while (child)
    {
        store.push_back(child);
        child = child->next;
    }
}

void Util::ShowAllAtomics(RwFrame *frame)
{
    if (!rwLinkListEmpty(&frame->objectList))
    {
        RwObjectHasFrame *atomic;

        RwLLLink *current = rwLinkListGetFirstLLLink(&frame->objectList);
        RwLLLink *end = rwLinkListGetTerminator(&frame->objectList);

        do
        {
            atomic = rwLLLinkGetData(current, RwObjectHasFrame, lFrame);
            atomic->object.flags |= rpATOMICRENDER; // clear

            current = rwLLLinkGetNext(current);
        } while (current != end);
    }
}

void Util::HideAllAtomics(RwFrame *frame)
{
    if (!rwLinkListEmpty(&frame->objectList))
    {
        RwObjectHasFrame *atomic;

        RwLLLink *current = rwLinkListGetFirstLLLink(&frame->objectList);
        RwLLLink *end = rwLinkListGetTerminator(&frame->objectList);

        while (current != end)
        {
            atomic = rwLLLinkGetData(current, RwObjectHasFrame, lFrame);
            atomic->object.flags &= ~rpATOMICRENDER;

            current = rwLLLinkGetNext(current);
        }
    }
}

void Util::HideChildWithName(RwFrame *parent_frame, const char *name)
{
    RwFrame *child = parent_frame->child;
    while (child)
    {
        if (!strcmp(GetFrameNodeName(child), name))
        {
            Util::HideAllAtomics(child);
            return;
        }
        child = child->next;
    }
}

void Util::ShowChildWithName(RwFrame *parent_frame, const char *name)
{
    RwFrame *child = parent_frame->child;
    while (child)
    {
        if (!strcmp(GetFrameNodeName(child), name))
        {
            Util::ShowAllAtomics(child);
            return;
        }
        child = child->next;
    }
}

void Util::HideAllChilds(RwFrame *parent_frame)
{
    RwFrame *child = parent_frame->child;
    while (child)
    {
        Util::HideAllAtomics(child);
        child = child->next;
    }
    Util::HideAllAtomics(parent_frame);
}

void Util::ShowAllChilds(RwFrame *parent_frame)
{
    RwFrame *child = parent_frame->child;
    while (child)
    {
        Util::ShowAllAtomics(child);
        child = child->next;
    }
    Util::ShowAllAtomics(parent_frame);
}

// Taken from vehfuncs
float Util::GetVehicleSpeedRealistic(CVehicle *vehicle)
{
    float wheelSpeed = 0.0;
    CVehicleModelInfo *vehicleModelInfo = (CVehicleModelInfo *)CModelInfo::GetModelInfo(vehicle->m_nModelIndex);
    if (vehicle->m_nVehicleSubClass == VEHICLE_BIKE || vehicle->m_nVehicleSubClass == VEHICLE_BMX)
    {
        CBike *bike = (CBike *)vehicle;
        wheelSpeed = ((bike->m_fWheelSpeed[0] * vehicleModelInfo->m_fWheelSizeFront) +
                      (bike->m_fWheelSpeed[1] * vehicleModelInfo->m_fWheelSizeRear)) /
                     2.0f;
    }
    else if (vehicle->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || vehicle->m_nVehicleSubClass == VEHICLE_MTRUCK || vehicle->m_nVehicleSubClass == VEHICLE_QUAD)
    {
        CAutomobile *automobile = (CAutomobile *)vehicle;
        wheelSpeed = ((automobile->m_fWheelSpeed[0] + automobile->m_fWheelSpeed[1] * vehicleModelInfo->m_fWheelSizeFront) +
                      (automobile->m_fWheelSpeed[2] + automobile->m_fWheelSpeed[3] * vehicleModelInfo->m_fWheelSizeRear)) /
                     4.0f;
    }
    else
    {
        return (Util::GetVehicleSpeed(vehicle)) * 3.6f;
    }
    wheelSpeed /= 2.45f;   // tweak based on distance (manually testing)
    wheelSpeed *= -186.0f; // tweak based on km/h

    return wheelSpeed;
}

unsigned int Util::GetEntityModel(void *ptr, eModelEntityType type)
{
    int model = 0;
    if (type == eModelEntityType::Weapon)
    {
        CWeaponInfo *pWeaponInfo = CWeaponInfo::GetWeaponInfo(reinterpret_cast<CWeapon *>(ptr)->m_eWeaponType,
                                                              FindPlayerPed()->GetWeaponSkill(reinterpret_cast<CWeapon *>(ptr)->m_eWeaponType));
        if (pWeaponInfo)
        {
            model = pWeaponInfo->m_nModelId1;
        }
    }
    else
    {
        model = reinterpret_cast<CEntity *>(ptr)->m_nModelIndex;
    }
    return model;
}

void Util::GetModelsFromIni(std::string &line, std::vector<int> &vec)
{
    std::stringstream ss(line);
    while (ss.good())
    {
        std::string model;
        getline(ss, model, ',');
        vec.push_back(std::stoi(model));
    }
}

std::optional<int> Util::GetDigitsAfter(const std::string &str, const std::string &prefix)
{
    if (str.rfind(prefix, 0) == 0)
    {
        std::string numberPart = str.substr(prefix.size());
        if (!numberPart.empty() &&
            std::all_of(numberPart.begin(), numberPart.end(), [](char c)
                        { return std::isdigit(c) || c == '.'; }) &&
            std::count(numberPart.begin(), numberPart.end(), '.') <= 1)
        {
            try
            {
                return std::stoi(numberPart);
            }
            catch (...)
            {
                return std::nullopt;
            }
        }
    }
    return std::nullopt;
}

std::optional<std::string> Util::GetCharsAfterPrefix(const std::string &str, const std::string &prefix, size_t num_chars)
{
    if (str.size() > prefix.size() && str.substr(0, prefix.size()) == prefix)
    {
        std::string suffix = str.substr(prefix.size(), num_chars);
        std::transform(suffix.begin(), suffix.end(), suffix.begin(), ::toupper);
        return suffix;
    }
    return std::nullopt;
}
