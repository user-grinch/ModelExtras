#include "pch.h"
#include "dummy.h"

inline std::string GetInfo(RwFrame* parent, const std::string& pattern, const std::string def) {
    if (!pattern.empty()) {
        std::regex re(pattern); 
        RwFrame* next = parent->child;
        while (next) {
            std::string name = GetFrameNodeName(next);
            std::smatch match;
            if (std::regex_search(name, match, re)) {
                if (match.size() > 1) {
                    return match[1].str(); 
                }
            }
            next = next->next;
        }
    }
    return def;
}

VehicleDummy::VehicleDummy(RwFrame* frame, std::string name, bool parent, eDummyPos type, RwRGBA color) {
    CurrentAngle = 0.0f;
    Frame = frame;
    Position = { frame->modelling.pos.x, frame->modelling.pos.y, frame->modelling.pos.z };
    ShdwPosition = Position;
    hasParent = parent;
    Color = color;
    Type = type;
    Size = 0.3f;

    // Calculate the angle based on the frame's orientation
    Angle = CGeneral::GetATanOfXY(frame->modelling.right.x, frame->modelling.right.y) * 57.295776f;
    
    // Normalize angle
    while (Angle < 0.0f) {
        Angle += 360.0f;
    }

    while (Angle > 360.0f) {
        Angle -= 360.0f;
    }

    if (type == eDummyPos::FrontLeft) {
        ShdwPosition.x -= std::stoi(GetInfo(frame, "offset:(\\d+)", "10")) / 10.0f; 
        Angle = 0 - std::stoi(GetInfo(frame, "rot:(-?\\d+)", "0"));
    }

    if (type == eDummyPos::FrontRight) {
        ShdwPosition.x += std::stoi(GetInfo(frame, "offset:(\\d+)", "5")) / 10.0f; 
        Angle = 0 - std::stoi(GetInfo(frame, "rot:(-?\\d+)", "0"));
    }

    if (type == eDummyPos::MiddleLeft) {
        ShdwPosition.x -= 2 * std::stoi(GetInfo(frame, "offset:(\\d+)", "5")) / 10.0f;
        Angle = 90 - std::stoi(GetInfo(frame, "rot:(-?\\d+)", "0"));
    }

    if (type == eDummyPos::MiddleRight) {
        ShdwPosition.x += 2 * std::stoi(GetInfo(frame, "offset:(\\d+)", "5")) / 10.0f;
        Angle = 270 - std::stoi(GetInfo(frame, "rot:(-?\\d+)", "0"));
    }

    if (type == eDummyPos::RearLeft) {
        ShdwPosition.x += -std::stoi(GetInfo(frame, "offset:(\\d+)", "5")) / 10.0f; 
        Angle = 180 - std::stoi(GetInfo(frame, "rot:(-?\\d+)", "0"));
    }

    if (type == eDummyPos::RearRight) {
        ShdwPosition.x += std::stoi(GetInfo(frame, "offset:(\\d+)", "5")) / 10.0f; 
        Angle = 180 - std::stoi(GetInfo(frame, "rot:(-?\\d+)", "0"));
    }

    if (type == eDummyPos::Rear) {
        Angle = 180 - std::stoi(GetInfo(frame, "rot:(-?\\d+)", "0"));
    }
    
    if (name.find("indicator_") != std::string::npos || name.find("turnl_") != std::string::npos) {
        std::string part = GetInfo(frame, "part:(\\D{2})", "");
        if (!part.empty()) {
            if (part == "fb") PartType = eDetachPart::FrontBumper;
            else if (part == "rb") PartType = eDetachPart::RearBumper;
            else if (part == "wl") PartType = eDetachPart::FrontLeftWing;
            else if (part == "wr") PartType = eDetachPart::FrontRightWing;
            else if (part == "yl") PartType = eDetachPart::FrontLeftWing;
            else if (part == "yr") PartType = eDetachPart::FrontRightWing;
            else PartType = eDetachPart::Unknown;
        }

        std::string col = GetInfo(frame, "col:([0-9A-Fa-f]{6})", "");
        if (!col.empty()) {
            Color.red = VehicleDummy::ReadHex(col[0], col[1]);
            Color.green = VehicleDummy::ReadHex(col[2], col[3]);
            Color.blue = VehicleDummy::ReadHex(col[4], col[5]);
        }

        std::string type = GetInfo(frame, "light_type:(\\d+)", "");
        if (!type.empty()) {
            Type = static_cast<eDummyPos>(std::stoi(type) - '0');
        }

        std::string sz = GetInfo(frame, "corona_sz:(\\d+)", "");
        if (!sz.empty()) {
            Size = static_cast<float>(std::stoi(sz) - '0');
        }
    }

    // Legacy support for ImVehFt vehicles
    size_t prmPos = name.find("_prm");
    if (prmPos != std::string::npos && prmPos + 11 <= name.size()) {
        Color.red = VehicleDummy::ReadHex(name[prmPos + 4], name[prmPos + 5]);
        Color.green = VehicleDummy::ReadHex(name[prmPos + 6], name[prmPos + 7]);
        Color.blue = VehicleDummy::ReadHex(name[prmPos + 8], name[prmPos + 9]);

        Type = static_cast<eDummyPos>(name[prmPos + 10] - '0');
        Size = static_cast<float>(name[prmPos + 11] - '0') / 10.0f;
    }
}

int VehicleDummy::ReadHex(char a, char b) {
	a = (a <= '9') ? a - '0' : (a & 0x7) + 9;
	b = (b <= '9') ? b - '0' : (b & 0x7) + 9;

	return (a << 4) + b;
}