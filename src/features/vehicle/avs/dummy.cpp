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
    Angle = 90.0f * static_cast<int>(Type);

    // Tweaked offsets
    if (type == eDummyPos::Left) {
        Position.x -= 1.0f;
    }

    if (type == eDummyPos::Right) {
        Position.x += 1.0f;
    }

    // Read params
    std::regex prmRegex("prm(\\w)(\\w)(\\w)(\\w)(\\w)(\\w)(\\w)(\\w)(\\w)");
    std::smatch match;
    if (std::regex_search(name, match, prmRegex)) {
        Color.red = VehicleDummy::ReadHex(*match[1].str().c_str(), *match[2].str().c_str());
        Color.green = VehicleDummy::ReadHex(*match[3].str().c_str(), *match[4].str().c_str());
        Color.blue = VehicleDummy::ReadHex(*match[5].str().c_str(), *match[6].str().c_str());
        
        Type = static_cast<eDummyPos>(match[7].str()[0] - '0');
        Size = (static_cast<float>(match[9].str()[0] - '0')) / 10.0f;
    } else if (std::regex_search(name, match, std::regex("turn"))){
        gLogger->warn(std::string(name + ", pattern not matched").c_str());
    }
}

int VehicleDummy::ReadHex(char a, char b) {
	a = (a <= '9') ? a - '0' : (a & 0x7) + 9;
	b = (b <= '9') ? b - '0' : (b & 0x7) + 9;

	return (a << 4) + b;
}