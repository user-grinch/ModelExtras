#include "pch.h"
#include "dummy.h"
#include "common.h"
#include "defines.h"
#include "datamgr.h"
#include "enums/dummypos.h"

VehicleDummy::VehicleDummy(CVehicle *pVeh, RwFrame *frame, std::string name, bool parent, eDummyPos type, RwRGBA color)
{
    CurrentAngle = 0.0f;
    Frame = frame;
    CVector pos = pVeh->GetPosition();
    Position = {Frame->ltm.pos.x - pos.x, Frame->ltm.pos.y - pos.y, Frame->ltm.pos.z - pos.z};
    ShdwPosition = Position;
    hasParent = parent;
    Color = color;
    Type = type;
    Size = 0.6f;
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

            Size = lights.value("CoronaSize", Size);
            PartType = eParentTypeFromString(lights.value("Parent", ""));
            LightType = GetLightType(lights.value("Type", "directional"));

            if (lights.contains("Shadow"))
            {
                auto &shadow = lights["Shadow"];
                shdwOffSet = {shadow.value("OffsetX", 0.0f), shadow.value("OffsetY", 0.0f)};
                shdowSize = {shadow.value("SizeX", 1.0f), shadow.value("SizeY", 1.0f)};
                angleVal = shadow.value("Rotation", 0.0f);
            }
        }
    }
    else
    {
        // Legacy support for ImVehFt vehicles
        size_t prmPos = name.find("_prm");

        if (prmPos != std::string::npos)
        {
            if (prmPos + 12 >= name.size())
            {
                gLogger->warn("Model {} has issue with node `{}`", pVeh->m_nModelIndex, name);
            }

            Color.r = VehicleDummy::ReadHex(name[prmPos + 4], name[prmPos + 5]);
            Color.g = VehicleDummy::ReadHex(name[prmPos + 6], name[prmPos + 7]);
            Color.b = VehicleDummy::ReadHex(name[prmPos + 8], name[prmPos + 9]);

            Type = static_cast<eDummyPos>(name[prmPos + 10] - '0');
            Size = static_cast<float>(name[prmPos + 11] - '0') / 10.0f;
            if (Size < 0.0f)
            {
                Size = 0.0f;
            }
            shdowSize = {static_cast<float>(name[prmPos + 12] - '0') / 7.5f, static_cast<float>(name[prmPos + 12] - '0') / 7.5f};
            if (shdowSize.x < 0.0f || shdowSize.y < 0.0f)
            {
                shdowSize = {0.0f, 0.0f};
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

    if (type == eDummyPos::MiddleLeft)
    {
        Angle = 90 - angleVal;
    }

    if (type == eDummyPos::MiddleRight)
    {
        Angle = 270 - angleVal;
    }

    if (type == eDummyPos::RearLeft)
    {
        Angle = 180 - angleVal;
    }

    if (type == eDummyPos::RearRight)
    {
        Angle = 180 - angleVal;
    }

    if (type == eDummyPos::Rear)
    {
        Angle = 180 - angleVal;
    }
}

int VehicleDummy::ReadHex(char a, char b)
{
    a = (a <= '9') ? a - '0' : (a & 0x7) + 9;
    b = (b <= '9') ? b - '0' : (b & 0x7) + 9;

    return (a << 4) + b;
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
}