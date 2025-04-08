#include "pch.h"
#include "dummy.h"
#include "common.h"
#include "defines.h"
#include "datamgr.h"
#include "enums/dummypos.h"

int ReadHex(char a, char b)
{
    a = (a <= '9') ? a - '0' : (a & 0x7) + 9;
    b = (b <= '9') ? b - '0' : (b & 0x7) + 9;

    return (a << 4) + b;
}

VehicleDummy::VehicleDummy(CVehicle *pVeh, RwFrame *frame, std::string name, eDummyPos type, CRGBA color, size_t dummyIdx)
{
    Frame = frame;
    CVector pos = pVeh->GetPosition();
    Position = {Frame->ltm.pos.x - pos.x, Frame->ltm.pos.y - pos.y, Frame->ltm.pos.z - pos.z};
    ShdwPosition = Position;
    Color = color;
    DummyType = type;
    DummyIdx = dummyIdx;
    float angleVal = 0.0f;

    // Calculate the angle based on the frame's orientation
    Angle = Common::NormalizeAngle(CGeneral::GetATanOfXY(frame->modelling.right.x, frame->modelling.right.y) * 57.295776f);

    auto &jsonData = DataMgr::Get(pVeh->m_nModelIndex);
    if (jsonData.contains("Lights"))
    {
        std::string newName = name.substr(0, name.find("_prm"));
        if (jsonData["Lights"].contains(newName.c_str()))
        {
            auto &lights = jsonData["Lights"][newName.c_str()];

            if (lights.contains("Color"))
            {
                Color.r = lights["Color"].value("R", Color.r);
                Color.g = lights["Color"].value("G", Color.g);
                Color.b = lights["Color"].value("B", Color.b);
                Color.a = lights["Color"].value("A", Color.a);
            }

            coronaSize = lights.value("CoronaSize", coronaSize);
            PartType = eParentTypeFromString(lights.value("Parent", ""));
            LightType = GetLightType(lights.value("Type", "directional"));

            if (lights.contains("Shadow"))
            {
                auto &shadow = lights["Shadow"];
                shdwOffSet = {shadow.value("OffsetX", 0.0f), shadow.value("OffsetY", 0.0f)};
                shdowSize = {shadow.value("SizeX", 1.0f), shadow.value("SizeY", 1.0f)};
                angleVal = shadow.value("Rotation", 0.0f);
            }

            // Only for StrobeLights
            if (lights.contains("StrobeDelay"))
            {
                strobeDelay = lights.value("StrobeDelay", 1000);
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
                Color.r = ReadHex(name[prmPos + 4], name[prmPos + 5]);
                Color.g = ReadHex(name[prmPos + 6], name[prmPos + 7]);
                Color.b = ReadHex(name[prmPos + 8], name[prmPos + 9]);
            }
            else
            {
                gLogger->warn("Model {} has issue with node `{}`: invalid color format", pVeh->m_nModelIndex, name);
            }

            if (prmPos + 10 < name.size())
            {
                LightType = static_cast<eLightType>(name[prmPos + 10] - '0');
            }
            else
            {
                gLogger->warn("Model {} has issue with node `{}`: invalid light type", pVeh->m_nModelIndex, name);
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
                gLogger->warn("Model {} has issue with node `{}`: invalid corona size", pVeh->m_nModelIndex, name);
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
                gLogger->warn("Model {} has issue with node `{}`: invalid shadow size", pVeh->m_nModelIndex, name);
            }
        }

        else
        {
            // Make indicator lights directional by default
            if (name.find("turnl_") != std::string::npos || name.find("indicator_") != std::string::npos)
            {
                LightType = eLightType::Directional;
            }
        }
    }
    if (type == eDummyPos::FrontLeft)
    {
        Angle = 0 - angleVal;
    }

    if (type == eDummyPos::FrontRight)
    {
        Angle = 0 - angleVal;
    }

    if (type == eDummyPos::RearLeft)
    {
        Angle = 180 - angleVal;
    }

    if (type == eDummyPos::RearRight)
    {
        Angle = 180 - angleVal;
    }

    if (type == eDummyPos::Left)
    {
        Angle = 90 - angleVal;
    }

    if (type == eDummyPos::Right)
    {
        Angle = 270 - angleVal;
    }

    if (type == eDummyPos::Rear)
    {
        Angle = 180 - angleVal;
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
    ShdwPosition = Position;

    if (DummyType == eDummyPos::Left)
    {
        ShdwPosition.x -= 1.25f;
    }

    if (DummyType == eDummyPos::Right)
    {
        ShdwPosition.x += 1.25f;
    }
}