#include "pch.h"
#include "dummy.h"

VehicleDummy::VehicleDummy(RwFrame* frame, std::string name, bool parent, eDummyPos type, RwRGBA color) {
    CurrentAngle = 0.0f;
    Frame = frame;
    Position = { frame->modelling.pos.x, frame->modelling.pos.y, frame->modelling.pos.z };
    hasParent = parent;
    Color = color;
    Type = type;
    Size = 0.3f;

    // Calculate the angle based on the frame's orientation
    Angle = CGeneral::GetATanOfXY(frame->modelling.right.x, frame->modelling.right.y) * 57.295776f;
    
    
    // Normalize angle
    if (Angle < 0.0f) Angle += 360.0f;

    // // Adjust for special cases based on frame's Z-axis orientation
    if (frame->modelling.at.z <= 0.0f && Angle == 0.0f) {
        Angle = frame->modelling.at.z * 180.0f;
        if (Angle > 0.0f) {
            Angle -= 180.0f;
        }
    }

    // Tweaked offsets
    float xOffset = 0.7f;
    if (type == eDummyPos::FrontLeft) {
        Position.x -= xOffset;
        if (Angle > 90.0f || Angle < -90.0f) {
            Angle += 180.0f;
        }
    }

    if (type == eDummyPos::MiddleLeft) {
        Position.x -= 2 * xOffset;
        Angle = 90.0f;
    }

    if (type == eDummyPos::RearLeft) {
        Position.x -= xOffset;
        if (Angle < 90.0f || Angle > -90.0f) {
            Angle += 180.0f;
        }
    }

    if (type == eDummyPos::FrontRight) {
        Position.x += xOffset;
    }

     if (type == eDummyPos::MiddleRight) {
        Position.x += 2 * xOffset;
        Angle = -90.0f;
    }

    if (type == eDummyPos::RearRight) {
        Position.x += xOffset;
    }

    // Parse params using regex
    std::regex prmRegex("prm(\\w)(\\w)(\\w)(\\w)(\\w)(\\w)(\\w)(\\w)(\\w)");
    std::smatch match;
    if (std::regex_search(name, match, prmRegex)) {
        Color.red = VehicleDummy::ReadHex(*match[1].str().c_str(), *match[2].str().c_str());
        Color.green = VehicleDummy::ReadHex(*match[3].str().c_str(), *match[4].str().c_str());
        Color.blue = VehicleDummy::ReadHex(*match[5].str().c_str(), *match[6].str().c_str());

        Type = static_cast<eDummyPos>(match[7].str()[0] - '0');
        Size = static_cast<float>(match[9].str()[0] - '0') / 10.0f;
    } else if (std::regex_search(name, match, std::regex("turn"))) {
        gLogger->warn((name + ", pattern not matched").c_str());
    }
}

int VehicleDummy::ReadHex(char a, char b) {
	a = (a <= '9') ? a - '0' : (a & 0x7) + 9;
	b = (b <= '9') ? b - '0' : (b & 0x7) + 9;

	return (a << 4) + b;
}