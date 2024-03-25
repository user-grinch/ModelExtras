#include "pch.h"
#include "dummy.h"

VehicleDummy::VehicleDummy(RwFrame* frame, std::string name, bool parent, eDummyRotation type, RwRGBA color) {
    CurrentAngle = 0.0f;
    Frame = frame;
    Position = { frame->modelling.pos.x, frame->modelling.pos.y, frame->modelling.pos.z };
    hasParent = parent;

    float angle = (CGeneral::GetATanOfXY(frame->modelling.right.x, frame->modelling.right.y) * 57.295776f) - 180.0f;

    while (angle < 0.0)
        angle += 360.0;

    Angle = angle - 180.0f;

    if (frame->modelling.at.z <= 0.0 && Angle == 0.0) {
        Angle = frame->modelling.at.z * 180.0f;
        Angle = (Angle > 0.0f) ? (Angle - 180.0f) : (Angle);
    }

    Color = color;
    Type = type;
    Size = 0.3f;

    // if (start > name.size()) {
    //     gLogger->warn(std::string(name + ", " + std::to_string(start) + " exceeds " + std::to_string(name.size())).c_str());
    //     return;
    // }

    std::regex prmRegex("prm(\\w)(\\w)(\\w)(\\w)(\\w)(\\w)(\\w)(\\w)(\\w)");
    std::smatch match;

    if (std::regex_search(name, match, prmRegex)) {
        Color.red = VehicleDummy::ReadHex(*match[1].str().c_str(), *match[2].str().c_str());
        Color.green = VehicleDummy::ReadHex(*match[3].str().c_str(), *match[4].str().c_str());
        Color.blue = VehicleDummy::ReadHex(*match[5].str().c_str(), *match[6].str().c_str());
        
        Type = static_cast<eDummyRotation>(match[7].str()[0] - '0');
        Size = (static_cast<float>(match[9].str()[0] - '0')) / 10.0f;
    } else if (std::regex_search(name, match, std::regex("turn"))){
        gLogger->warn(std::string(name + ", pattern not matched").c_str());
        // return;
    }

    if (Type == eDummyRotation::Forward && type != eDummyRotation::Forward)
        Angle -= 180.0f;
    else if (Type != eDummyRotation::Forward && type == eDummyRotation::Forward)
        Angle += 180.0f;
}

int VehicleDummy::ReadHex(char a, char b) {
	a = (a <= '9') ? a - '0' : (a & 0x7) + 9;
	b = (b <= '9') ? b - '0' : (b & 0x7) + 9;

	return (a << 4) + b;
}

CVector VehicleDummy::GetPosition() {
	/*CVector vector = RwFrameGetMatrix(Frame)->pos;

	if (hasParent) {
		RwFrame* parent = RwFrameGetParent(Frame);

		if (parent)
			vector = RwFrameGetMatrix(parent)->pos + vector;
	}*/

	return { Frame->modelling.pos.x, Frame->modelling.pos.y, Frame->modelling.pos.z };
};
