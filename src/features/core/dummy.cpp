#include "pch.h"
#include "dummy.h"
#include "defines.h"
#include "utils/datamgr.h"
#include "enums/dummypos.h"
#include <CWorld.h>
#include <CBike.h>
#include <CModelInfo.h>

extern float gfGlobalCoronaSize;
extern int gGlobalCoronaIntensity;
extern int gGlobalShadowIntensity;

int ReadHex(char a, char b)
{
    a = (a <= '9') ? a - '0' : (a & 0x7) + 9;
    b = (b <= '9') ? b - '0' : (b & 0x7) + 9;

    return (a << 4) + b;
}

VehicleDummy::VehicleDummy(const DummyConfig& config)
{
    data = config;
    float angleVal = 0.0f;

    // Calculate the heading angle (0 = Forward, 180 = Rear, 90 = Left, 270 = Right) based on frame's forward orientation
    float dir_x = data.frame->modelling.up.x;
    float dir_y = data.frame->modelling.up.y;

    // Handle pitch-flipped / mirrored frames (at.z < 0 and right.x < 0) from 3ds Max/ZModeler
    if (data.frame->modelling.at.z < 0.0f && data.frame->modelling.right.x < 0.0f)
    {
        dir_x = -dir_x;
        dir_y = -dir_y;
    }

    float rad = std::atan2(-dir_x, dir_y);
    data.rotation.angle = Util::NormalizeAngle(static_cast<float>(Util::RadToDeg(rad)));

    auto &jsonData = DataMgr::Get(data.pVeh->m_nModelIndex);
    std::string name = GetFrameNodeName(data.frame);

    std::string parentName = GetFrameNodeName(RwFrameGetParent(data.frame));
    if (parentName.ends_with("_dummy")) {
        data.isParentDummy = true;
    }

    // The bike lean is applied to chassis_dummy, so only frames somewhere below it get
    // rolled with the bike. Lights parented straight to the root keep the plain entity
    // matrix, and using the lean matrix on those would tilt them the wrong way instead.
    for (RwFrame *pParent = RwFrameGetParent(data.frame); pParent; pParent = RwFrameGetParent(pParent)) {
        if (std::string(GetFrameNodeName(pParent)).ends_with("_dummy")) {
            data.leanAffected = true;
            break;
        }
    }

    if (jsonData.contains("lights"))
    {
        std::string newName = name.substr(0, name.find("_prm"));
        if (jsonData["lights"].contains(newName.c_str()))
        {
            auto &lights = jsonData["lights"][newName.c_str()];

            if (lights.contains("corona"))
            {
                auto &coronaSec = lights["corona"];
                if (coronaSec.contains("color"))
                {
                    data.corona.color.r = coronaSec["color"].value("red", data.corona.color.r);
                    data.corona.color.g = coronaSec["color"].value("green", data.corona.color.g);
                    data.corona.color.b = coronaSec["color"].value("blue", data.corona.color.b);
                    data.corona.color.a = coronaSec["color"].value("alpha", gGlobalCoronaIntensity);
                }
                data.corona.size = coronaSec.value("size", gfGlobalCoronaSize);
                data.corona.lightingType = GetLightingMode(coronaSec.value("type", "directional"));
            }

            if (lights.contains("shadow"))
            {
                auto &shadow = lights["shadow"];
                if (shadow.contains("color"))
                {
                    data.shadow.color.r = shadow["color"].value("red", data.shadow.color.r);
                    data.shadow.color.g = shadow["color"].value("green", data.shadow.color.g);
                    data.shadow.color.b = shadow["color"].value("blue", data.shadow.color.b);
                    data.shadow.color.a = shadow["color"].value("alpha", gGlobalShadowIntensity);
                }
                data.shadow.size = shadow.value("size", 1.0f);
                data.shadow.texture = shadow.value("texture", "");
                data.shadow.rotationChecks = shadow.value("rotationchecks", true);

                // shadows will be force enabled if there is JSON data for it.
                data.shadow.render = true;
            }

            // Only for StrobeLights
            if (lights.contains("strobedelay"))
            {
                data.strobe.delay = lights.value("strobedelay", 1000);
            }
        }
    }
    else
    {
        // Legacy support for ImVehFt vehicles
        size_t prmPos = name.find("_prm");
        if (prmPos != std::string::npos)
        {
            if (prmPos + 9 < name.size())
            {
                data.shadow.color.r = data.corona.color.r = ReadHex(name[prmPos + 4], name[prmPos + 5]);
                data.shadow.color.g = data.corona.color.g = ReadHex(name[prmPos + 6], name[prmPos + 7]);
                data.shadow.color.b = data.corona.color.b = ReadHex(name[prmPos + 8], name[prmPos + 9]);
            }
            else
            {
                LOG_VERBOSE("Model {} has issue with node `{}`: invalid color format", data.pVeh->m_nModelIndex, name);
            }

            if (prmPos + 10 < name.size())
            {
                int type = name[prmPos + 10] - '0';
                if (type == 2)
                    data.corona.lightingType = eLightingMode::NonDirectional;
                else if (type == 1)
                    data.corona.lightingType = eLightingMode::Inversed;
                else
                    data.corona.lightingType = eLightingMode::Directional;
            }
            else
            {
                data.corona.lightingType = eLightingMode::NonDirectional;
                LOG_VERBOSE("Model {} has issue with node `{}`: invalid light type", data.pVeh->m_nModelIndex, name);
            }

            if (prmPos + 11 < name.size())
            {
                data.corona.size = static_cast<float>(name[prmPos + 11] - '0') / 10.0f;
                if (data.corona.size < 0.0f)
                {
                    data.corona.size = 0.0f;
                }
            }
            else
            {
                data.corona.size = 0.0f;
                LOG_VERBOSE("Model {} has issue with node `{}`: invalid corona size", data.pVeh->m_nModelIndex, name);
            }

            if (prmPos + 12 < name.size())
            {
                float shadowValue = static_cast<float>(name[prmPos + 12] - '0') / 7.5f;
                data.shadow.size = std::max(shadowValue, 0.0f);

                if (data.shadow.size > 0.0f) {
                    data.shadow.render = true;
                }
            }
            else
            {
                data.shadow.size = 0.0f;
                LOG_VERBOSE("Model {} has issue with node `{}`: invalid shadow size", data.pVeh->m_nModelIndex, name);
            }
        }
    }
}

void VehicleDummy::Update() {
    // The corona is expanded again through the entity matrix, so the offset has to be
    // taken apart with the matrix that placed the frame. On a bike the lights sit under
    // chassis_dummy, which is rolled by m_mLeanMatrix, so using the entity matrix here
    // leaves the lean in the offset and the corona tilts with it. The lean matrix is
    // only valid for the frame that calculated it, so refresh it when the flag is down
    // and put the flag back so the game still recalculates it when it needs to.
    CMatrix basis = data.pVeh->GetMatrix();
    if (data.pVeh->m_nVehicleSubClass == VEHICLE_BIKE && data.leanAffected)
    {
        CBike *pBike = static_cast<CBike *>(data.pVeh);
        bool wasCalculated = pBike->m_bLeanMatrixCalculated;
        if (!wasCalculated)
        {
            pBike->CalculateLeanMatrix();
        }

        basis = pBike->m_mLeanMatrix;
        pBike->m_bLeanMatrixCalculated = wasCalculated;
    }

    CVector offset = data.frame->ltm.pos - basis.pos;

    // Transform to local space using  transpose of the rotation matrix
    data.shadow.position.x = data.position.x = basis.right.x * offset.x + basis.right.y * offset.y + basis.right.z * offset.z;
    data.shadow.position.y = data.position.y = basis.up.x * offset.x + basis.up.y * offset.y + basis.up.z * offset.z;
    data.shadow.position.z = data.position.z = basis.at.x * offset.x + basis.at.y * offset.y + basis.at.z * offset.z;

    // Protection against buggy DFF models where child dummies are exported with root-space coordinates
    // (causing RenderWare hierarchy translation to double-offset the position far beyond the vehicle body)
    CVehicleModelInfo *pInfo = static_cast<CVehicleModelInfo *>(CModelInfo::GetModelInfo(data.pVeh->m_nModelIndex));
    if (pInfo && pInfo->m_pColModel)
    {
        const auto &box = pInfo->m_pColModel->m_boundBox;
        const float MARGIN = 0.5f;
        if (data.position.y > box.m_vecMax.y + MARGIN || data.position.y < box.m_vecMin.y - MARGIN
            || data.position.x > box.m_vecMax.x + MARGIN || data.position.x < box.m_vecMin.x - MARGIN
            || data.position.z > box.m_vecMax.z + MARGIN || data.position.z < box.m_vecMin.z - MARGIN)
        {
            data.position = data.frame->modelling.pos;
            data.shadow.position = data.frame->modelling.pos;
        }
    }

    if (data.mirroredX)
    {
        data.position.x *= -1;
        data.shadow.position.x *= -1;
    }
}