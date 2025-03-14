#include "pch.h"
#include "dummy.h"
#include "common.h"
#include "defines.h"
#include "datamgr.h"
#include "enums/dummypos.h"

VehicleDummy::VehicleDummy(CVehicle* pVeh, RwFrame* frame, std::string name, bool parent, eDummyPos type, RwRGBA color) {
    CurrentAngle = 0.0f;
    Frame = frame;
    Position = { frame->modelling.pos.x, frame->modelling.pos.y, frame->modelling.pos.z };
    ShdwPosition = Position;
    hasParent = parent;
    Color = color;
    Type = type;
    Size = 0.6f;
    float offSetVal = 0.0f;
    float angleVal = 0.0f;

    // Calculate the angle based on the frame's orientation
    Angle = Common::NormalizeAngle(CGeneral::GetATanOfXY(frame->modelling.right.x, frame->modelling.right.y) * 57.295776f);

    auto& jsonData = DataMgr::Get(pVeh->m_nModelIndex);
    if (jsonData.contains("Lights")) {
w        std::string newName = name.substr(0, name.find("_prm"));
        if (jsonData["Lights"].contains(newName.c_str())) {
            auto& lights = jsonData["Lights"][newName.c_str()];

            if (lights.contains("Color")) {
                Color.r = lights["Color"].value("R", Color.r);
                Color.g = lights["Color"].value("G", Color.g);
                Color.b = lights["Color"].value("B", Color.b);
            }

            Size = lights.value("CoronaSize", Size);
            PartType = eParentTypeFromString(lights.value("Parent", ""));

            if (lights.contains("Shadow")) {
                auto& shadow = lights["Shadow"];
                hasShadow = shadow.value("Enabled", false);
                offSetVal = shadow.value("Offset", 0.0f);
                angleVal = shadow.value("Rotation", 0.0f);
            }
        }
    }
    else {
         // Legacy support for ImVehFt vehicles
        size_t prmPos = name.find("_prm");
        if (prmPos != std::string::npos && prmPos + 11 <= name.size()) {
            Color.r = VehicleDummy::ReadHex(name[prmPos + 4], name[prmPos + 5]);
            Color.g = VehicleDummy::ReadHex(name[prmPos + 6], name[prmPos + 7]);
            Color.b = VehicleDummy::ReadHex(name[prmPos + 8], name[prmPos + 9]);

            Type = static_cast<eDummyPos>(name[prmPos + 10] - '0');
            Size = static_cast<float>(name[prmPos + 11] - '0') / 10.0f;
        }
    }
    if (type == eDummyPos::FrontLeft) {
        ShdwPosition.x -= offSetVal / 10.0f;
        Angle = 0 - angleVal;
    }

    if (type == eDummyPos::FrontRight) {
        ShdwPosition.x += offSetVal / 10.0f;
        Angle = 0 - angleVal;
    }

    if (type == eDummyPos::MiddleLeft) {
        ShdwPosition.x -= 2 * offSetVal / 10.0f;
        Angle = 90 - angleVal;
    }

    if (type == eDummyPos::MiddleRight) {
        ShdwPosition.x += 2 * offSetVal / 10.0f;
        Angle = 270 - angleVal;
    }

    if (type == eDummyPos::RearLeft) {
        ShdwPosition.x += -offSetVal / 10.0f;
        Angle = 180 - angleVal;
    }

    if (type == eDummyPos::RearRight) {
        ShdwPosition.x += offSetVal / 10.0f;
        Angle = 180 - angleVal;
    }

    if (type == eDummyPos::Rear) {
        Angle = 180 - angleVal;
    }
}

int VehicleDummy::ReadHex(char a, char b) {
    a = (a <= '9') ? a - '0' : (a & 0x7) + 9;
    b = (b <= '9') ? b - '0' : (b & 0x7) + 9;

    return (a << 4) + b;
}