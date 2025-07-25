#include "pch.h"
#include "dummy.h"
#include "defines.h"
#include "datamgr.h"
#include "enums/dummypos.h"

int ReadHex(char a, char b)
{
    a = (a <= '9') ? a - '0' : (a & 0x7) + 9;
    b = (b <= '9') ? b - '0' : (b & 0x7) + 9;

    return (a << 4) + b;
}

VehicleDummy::VehicleDummy(CVehicle *pVeh, RwFrame *frame, std::string name, eDummyPos type, CRGBA color, size_t dummyIdx, bool directionalByDef, bool mirroredX)
{
    Frame = frame;
    CVector pos = pVeh->GetPosition();
    Position = {Frame->ltm.pos.x - pos.x, Frame->ltm.pos.y - pos.y, Frame->ltm.pos.z - pos.z};

    if (mirroredX)
    {
        this->mirroredX = mirroredX;
        Position.x *= -1;
    }
    shdwCol = coronaCol = color;
    DummyType = type;
    DummyIdx = dummyIdx;
    float angleVal = 0.0f;

    if (directionalByDef)
    {
        LightType = eLightingMode::Directional;
    }

    // Calculate the angle based on the frame's orientation
    float modelAngle = Util::NormalizeAngle(CGeneral::GetATanOfXY(frame->modelling.right.x, frame->modelling.right.y) * 57.295776f);
    if (modelAngle != 0.0f)
    {
        if (modelAngle == 0.0f)
        {
            DummyType = eDummyPos::Front;
        }
        if (modelAngle == 90.0f)
        {
            DummyType = eDummyPos::Left;
        }
        if (modelAngle == 180.0f)
        {
            DummyType = eDummyPos::Rear;
        }
        if (modelAngle == 270.0f)
        {
            DummyType = eDummyPos::Right;
        }
    }

    auto &jsonData = DataMgr::Get(pVeh->m_nModelIndex);
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
                    coronaCol.r = coronaSec["color"].value("red", coronaCol.r);
                    coronaCol.g = coronaSec["color"].value("green", coronaCol.g);
                    coronaCol.b = coronaSec["color"].value("blue", coronaCol.b);
                    coronaCol.a = coronaSec["color"].value("alpha", coronaCol.a);
                }
                coronaSize = coronaSec.value("size", coronaSize);
                LightType = GetLightingMode(coronaSec.value("type", "directional"));
            }

            PartType = eParentTypeFromString(lights.value("parent", ""));

            if (lights.contains("shadow"))
            {
                auto &shadow = lights["shadow"];
                if (shadow.contains("color"))
                {
                    shdwCol.r = shadow["color"].value("red", shdwCol.r);
                    shdwCol.g = shadow["color"].value("green", shdwCol.g);
                    shdwCol.b = shadow["color"].value("blue", shdwCol.b);
                    shdwCol.a = shadow["color"].value("alpha", shdwCol.a);
                }
                shdwOffSet = {shadow.value("offsetx", 0.0f), shadow.value("offsety", 0.0f)};

                // This needs to be like this
                shdowSize = {shadow.value("width", 1.0f), shadow.value("height", 1.0f)};
                angleVal = shadow.value("angleoffset", 0.0f);
                shdwTex = shadow.value("texture", "");
            }

            // Only for StrobeLights
            if (lights.contains("strobedelay"))
            {
                strobeDelay = lights.value("strobedelay", 1000);
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
                shdwCol.r = coronaCol.r = ReadHex(name[prmPos + 4], name[prmPos + 5]);
                shdwCol.g = coronaCol.g = ReadHex(name[prmPos + 6], name[prmPos + 7]);
                shdwCol.b = coronaCol.b = ReadHex(name[prmPos + 8], name[prmPos + 9]);
            }
            else
            {
                LOG_VERBOSE("Model {} has issue with node `{}`: invalid color format", pVeh->m_nModelIndex, name);
            }

            if (prmPos + 10 < name.size())
            {
                LightType = static_cast<eLightingMode>(name[prmPos + 10] - '0');
            }
            else
            {
                LOG_VERBOSE("Model {} has issue with node `{}`: invalid light type", pVeh->m_nModelIndex, name);
            }

            if (prmPos + 11 < name.size())
            {
                coronaSize = static_cast<float>(name[prmPos + 11] - '0') / 10.0f;
                if (coronaSize < 0.0f)
                {
                    coronaSize = 0.0f;
                }
            }
            else
            {
                LOG_VERBOSE("Model {} has issue with node `{}`: invalid corona size", pVeh->m_nModelIndex, name);
            }

            if (prmPos + 12 < name.size())
            {
                float shadowValue = static_cast<float>(name[prmPos + 12] - '0') / 7.5f;
                shdowSize = {shadowValue, shadowValue};
                if (shdowSize.x < 0.0f || shdowSize.y < 0.0f)
                {
                    shdowSize = {0.0f, 0.0f};
                }
            }
            else
            {
                LOG_VERBOSE("Model {} has issue with node `{}`: invalid shadow size", pVeh->m_nModelIndex, name);
            }
        }
    }

    if (DummyType == eDummyPos::Front)
    {
        Angle = 0 - angleVal;
    }

    if (DummyType == eDummyPos::Left)
    {
        Angle = 90 - angleVal;
    }

    if (DummyType == eDummyPos::Right)
    {
        Angle = 270 - angleVal;
    }

    if (DummyType == eDummyPos::Rear)
    {
        Angle = 180 - angleVal;
    }

    if (DummyType == eDummyPos::None)
    {
        Angle = modelAngle;
    }
}

void VehicleDummy::Update(CVehicle *pVeh)
{
    CMatrix &vehMatrix = *(CMatrix *)pVeh->GetMatrix();
    CVector pos = pVeh->GetPosition();
    CVector dummyPos = Frame->ltm.pos;
    CVector offset = dummyPos - pos;

    // Transform to local space using  transpose of the rotation matrix
    Position.x = vehMatrix.right.x * offset.x + vehMatrix.right.y * offset.y + vehMatrix.right.z * offset.z;
    Position.y = vehMatrix.up.x * offset.x + vehMatrix.up.y * offset.y + vehMatrix.up.z * offset.z;
    Position.z = vehMatrix.at.x * offset.x + vehMatrix.at.y * offset.y + vehMatrix.at.z * offset.z;

    if (mirroredX)
    {
        Position.x *= -1;
    }
}