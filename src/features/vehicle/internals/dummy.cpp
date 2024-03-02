#include "pch.h"
#include "dummy.h"

VehicleDummy::VehicleDummy(RwFrame* frame, std::string name, int start, bool parent, eDummyRotation type, RwRGBA color) {
	CurrentAngle = 0.0f;

	Frame = frame;

	Position = { frame->modelling.pos.x, frame->modelling.pos.y, frame->modelling.pos.z };

	//Position = ;

	hasParent = parent;

	/*if (hasParent) {
		if (RwFrame* parent = RwFrameGetParent(frame)) {
			Position = { parent->modelling.pos.x, parent->modelling.pos.y, parent->modelling.pos.z };
		}
	}*/

	//PluginMultiplayer::AddChatMessage(std::string(name + " is at: " + std::to_string(frame->modelling.pos.x) + ", " + std::to_string(frame->modelling.pos.y) + ", " + std::to_string(frame->modelling.pos.z)).c_str());

	Angle = ((CGeneral::GetATanOfXY(frame->modelling.right.x, frame->modelling.right.y) * 57.295776f) - 180.0f);

	// there's probably a better way of doing this (there is)
	// but I'm not good with maths, this took me way too long

	while (Angle < 0.0) Angle += 360.0;

	Angle -= 180.0f;

	if (frame->modelling.at.z <= 0.0 && Angle == 0.0) {
		Angle = frame->modelling.at.z * 180.0f;

		Angle = (Angle > 0.0f) ? (Angle - 180.0f) : (Angle);
	}

	Color = color;

	Type = type;
	Size = 0.3f;

	// if (Type == eDummyRotation::Backward)
	// 	Angle -= 180.0f;
	
	int size = name.size();

	if (start > size) {
		gLogger->warn(std::string(name + ", " + std::to_string(start) + " exceeds " + std::to_string(size)).c_str());
		return;
	}

	if (!(name[start] == 'p' && name[start + 1] == 'r' && name[start + 2] == 'm')) {
		if (size >= start + 5) {
			Color.red = VehicleDummy::ReadHex(name[start], name[start + 1]);
			Color.green = VehicleDummy::ReadHex(name[start + 2], name[start + 3]);
			Color.blue = VehicleDummy::ReadHex(name[start + 4], name[start + 5]);
		}

		return;
	}

	if ((size - start) != 12) {
		gLogger->warn(std::string(name + ", 12 is not " + std::to_string(size - start)).c_str());
		return;
	}

	Color.red = VehicleDummy::ReadHex(name[start + 3], name[start + 4]);
	Color.green = VehicleDummy::ReadHex(name[start + 5], name[start + 6]);
	Color.blue = VehicleDummy::ReadHex(name[start + 7], name[start + 8]);

	Type = static_cast<eDummyRotation>(name[start + 9] - '0');
	Size = ((float)(name[start + 10] - '0')) / 10.0f;

	if (Type == eDummyRotation::Forward && type != eDummyRotation::Forward)
		Angle -= 180.0f;
	else if (Type != eDummyRotation::Forward && type == eDummyRotation::Forward)
		Angle += 180.0f;
};

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
