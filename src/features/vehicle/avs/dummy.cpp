#include "pch.h"
#include "dummy.h"

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

    // Tweaked offsets
    float xOffset = 0.7f;
    float errorPadding = 2.0f;
    if (type == eDummyPos::FrontLeft) {
        ShdwPosition.x -= xOffset; 
        plugin::Clamp(Angle, 270.0f+errorPadding, 360.0f-errorPadding);
    }

    if (type == eDummyPos::FrontRight) {
        ShdwPosition.x += xOffset; 
        plugin::Clamp(Angle, 0.0f+errorPadding, 90.0f-errorPadding);
    }

    if (type == eDummyPos::MiddleLeft) {
        ShdwPosition.x -= 2 * xOffset;
        plugin::Clamp(Angle, 180.0f+errorPadding, 360.0f-errorPadding);
    }

    if (type == eDummyPos::MiddleRight) {
        ShdwPosition.x += 2 * xOffset;
        plugin::Clamp(Angle, 0.0f+errorPadding, 180.0f-errorPadding);
    }

    if (type == eDummyPos::RearLeft) {
        ShdwPosition.x += -xOffset; 
        Angle += 180.0f;
        plugin::Clamp(Angle, 180.0f+errorPadding, 270.0f-errorPadding);
    }

    if (type == eDummyPos::RearRight) {
        ShdwPosition.x += xOffset; 
        Angle += 180.0f;
        plugin::Clamp(Angle, 90.0f+errorPadding, 180.0f-errorPadding);
    }

    if (type == eDummyPos::Rear) {
        Angle = 180.0f;
    }

    size_t prmPos = name.find("_prm");
    if (prmPos != std::string::npos && prmPos + 11 <= name.size()) {
        Color.red = VehicleDummy::ReadHex(name[prmPos + 4], name[prmPos + 5]);
        Color.green = VehicleDummy::ReadHex(name[prmPos + 6], name[prmPos + 7]);
        Color.blue = VehicleDummy::ReadHex(name[prmPos + 8], name[prmPos + 9]);

        Type = static_cast<eDummyPos>(name[prmPos + 10] - '0');
        Size = static_cast<float>(name[prmPos + 11] - '0') / 10.0f;
    }

    if (name.find("_fb") != std::string::npos) PartType = eDetachPart::FrontBumper;
    if (name.find("_rb") != std::string::npos) PartType = eDetachPart::RearBumper;
    if (name.find("_wl") != std::string::npos) PartType = eDetachPart::FrontLeftWing;
    if (name.find("_wr") != std::string::npos) PartType = eDetachPart::FrontRightWing;
    if (name.find("_yl") != std::string::npos) PartType = eDetachPart::FrontLeftWing;
    if (name.find("_yr") != std::string::npos) PartType = eDetachPart::FrontRightWing;
}

int VehicleDummy::ReadHex(char a, char b) {
	a = (a <= '9') ? a - '0' : (a & 0x7) + 9;
	b = (b <= '9') ? b - '0' : (b & 0x7) + 9;

	return (a << 4) + b;
}