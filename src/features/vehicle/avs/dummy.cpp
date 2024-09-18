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

    // // Adjust for special cases based on frame's Z-axis orientation
    // if (frame->modelling.at.z <= 0.0f && Angle == 0.0f) {
    //     Angle = frame->modelling.at.z * 180.0f;
    //     if (Angle > 0.0f) {
    //         Angle -= 180.0f;
    //     }
    // }

    // Tweaked offsets
    float xOffset = 0.7f;
    float errorPadding = 2.0f;
    if (type == eDummyPos::FrontLeft ) {
        ShdwPosition.x -= xOffset; 
        if (Angle > 0.0f+errorPadding && Angle < 270.0f-errorPadding) {
            Angle += 180.0f;
        }
        Angle -= 180.0f;
    }

    if (type == eDummyPos::FrontRight) {
        ShdwPosition.x += xOffset; 
        if (Angle > 90.0f+errorPadding) {
            Angle += 180.0f;
        }
        Angle -= 180.0f;
    }

    if (type == eDummyPos::MiddleLeft) {
        ShdwPosition.x -= 2 * xOffset;
        if (Angle > 0.0f+errorPadding && Angle < 180.0f-errorPadding) {
            Angle += 180.0f;
        }
    }

    if (type == eDummyPos::MiddleRight) {
        ShdwPosition.x += 2 * xOffset;
        if (Angle > 180.0f+errorPadding) { 
            Angle += 180.0f;
        }
    }

    if (type == eDummyPos::RearLeft) {
        ShdwPosition.x += -xOffset; 
        if (Angle < 90.0f-errorPadding || Angle > 180.0f+errorPadding) {
            Angle += 180.0f;
        }
    }

    if (type == eDummyPos::RearRight) {
        ShdwPosition.x += xOffset; 
        if (Angle < 180.0f+errorPadding || Angle > 270.0f-errorPadding) {
            Angle += 180.0f;
        }
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
}

int VehicleDummy::ReadHex(char a, char b) {
	a = (a <= '9') ? a - '0' : (a & 0x7) + 9;
	b = (b <= '9') ? b - '0' : (b & 0x7) + 9;

	return (a << 4) + b;
}