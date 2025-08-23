#include "pch.h"
#include "util.h"
#include <regex>
#include <CWeaponInfo.h>
#include <CCamera.h>
#include <CCoronas.h>
#include <CShadows.h>
#include <CBike.h>
#include <CWorld.h>
#include <CClock.h>
#include "texmgr.h"
#include "defines.h"
#include "vehicle/core/dummy.h"
#include "enums/lightoverride.h"
#include <math.h>

float Util::NormalizeAngle(float angle)
{
    while (angle < 0.0f)
        angle += 360.0f;
    while (angle >= 360.0f)
        angle -= 360.0f;
    return angle;
}

double Util::RadToDeg(double rad) {
    return rad * (180.0 / PI);
}

double Util::DegToRad(double deg) {
    return deg * (PI / 180.0);
}

float Util::GetVehicleSpeed(CVehicle *pVeh)
{
    return pVeh->m_vecMoveSpeed.Magnitude2D() * 50.0f;
}

bool Util::IsLightDamaged(CVehicle *pVeh, eLights light) {
    if (!pVeh || pVeh->m_nVehicleSubClass != VEHICLE_AUTOMOBILE) {
        return false;
    }
    CAutomobile *pAutoMobile = static_cast<CAutomobile*>(pVeh);
    if (!pAutoMobile) {
        return false;
    }

    return pAutoMobile->m_damageManager.GetLightStatus(light);
}

bool Util::IsDoorDamaged(CVehicle *pVeh, eDoors door) {
    if (!pVeh || pVeh->m_nVehicleSubClass != VEHICLE_AUTOMOBILE) {
        return false;
    }
    CAutomobile *pAutoMobile = static_cast<CAutomobile*>(pVeh);
    if (!pAutoMobile) {
        return false;
    }

    return pAutoMobile->m_damageManager.GetDoorStatus(door);
}

bool Util::IsPanelDamaged(CVehicle *pVeh, ePanels panel) {
    if (!pVeh || pVeh->m_nVehicleSubClass != VEHICLE_AUTOMOBILE) {
        return false;
    }
    CAutomobile *pAutoMobile = static_cast<CAutomobile*>(pVeh);
    if (!pAutoMobile) {
        return false;
    }

    return pAutoMobile->m_damageManager.GetPanelStatus(panel);
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
            model = pWeaponInfo->m_nModelId;
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

bool Util::IsNightTime()
{
	return CClock::GetIsTimeInRange(20, 6);
}

bool Util::IsEngineOff(CVehicle *pVeh)
{
	return !pVeh->bEngineOn || pVeh->bEngineBroken;
}

float mx1, my1, mz1;
float mx2, my2, mz2;

CVector Util::UpdateRelativeToBoundingBox(CVehicle *pVeh, eDummyPos dummyPos, CVector shdwPos, CVector up, CVector right) {
    CVehicleModelInfo* pInfo = (CVehicleModelInfo*)CModelInfo::GetModelInfo(pVeh->m_nModelIndex);
    CVector min = pInfo->m_pColModel->m_boundBox.m_vecMin + CVector(mx1, my1, mz1);
    CVector max = pInfo->m_pColModel->m_boundBox.m_vecMax + CVector(mx2, my2, mz2);

    shdwPos += (min + max) * 0.5f;
    CVector corner1 = shdwPos + up + right;
    CVector corner2 = shdwPos + up - right;
    CVector corner3 = shdwPos - up + right;
    CVector corner4 = shdwPos - up - right;

    // Compute min vector
    CVector minVec;
    minVec.x = std::min({ corner1.x, corner2.x, corner3.x, corner4.x });
    minVec.y = std::min({ corner1.y, corner2.y, corner3.y, corner4.y });
    minVec.z = std::min({ corner1.z, corner2.z, corner3.z, corner4.z });

    // Compute max vector
    CVector maxVec;
    maxVec.x = std::max({ corner1.x, corner2.x, corner3.x, corner4.x });
    maxVec.y = std::max({ corner1.y, corner2.y, corner3.y, corner4.y });
    maxVec.z = std::max({ corner1.z, corner2.z, corner3.z, corner4.z });

    if (dummyPos == eDummyPos::Front) {
        minVec.y = std::max(minVec.y, max.y);
    }
    else if (dummyPos == eDummyPos::Rear) {
        maxVec.y = std::min(maxVec.y, min.y);
    } 

    shdwPos = (maxVec + minVec) * 0.5f;
    shdwPos -= (min + max) * 0.5f;
    // else if (dummyPos == eDummyPos::Left) {
    //     pos.x = std::min(offset.x, min.x);
    // }
    // else if (dummyPos == eDummyPos::Right) {
    //     pos.x = std::max(offset.x, max.x);
    // }
    return shdwPos;
}

float Util::GetVehiclePitch(CVehicle* pVeh) {
    if (!pVeh || !pVeh->m_matrix) {
        return 0.0f;
	}

    CVector forward = pVeh->m_matrix->at;
    forward.Normalise();

    float pitchRad = asinf(forward.y);
    return pitchRad * 57.2957795f;
}

bool Util::IsVehicleDoingWheelie(CVehicle *pVeh) {
    return pVeh->m_nVehicleSubClass == VEHICLE_BIKE && (Util::GetVehiclePitch(pVeh) > 30.0f || pVeh->GetNumContactWheels() < 2);
}