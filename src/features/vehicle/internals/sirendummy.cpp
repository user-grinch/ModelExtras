#include "pch.h"
#include "sirendummy.h"

VehicleSirenDummy::VehicleSirenDummy(RwFrame* frame, std::string name, int start, bool parent, int type, RwRGBA color) {
	CurrentAngle = 0.0f;
	Frame = frame;
	Position = { frame->modelling.pos.x, frame->modelling.pos.y, frame->modelling.pos.z };
	hasParent = parent;


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

	if (Type == 1)
		Angle -= 180.0f;
	
	int size = name.size();

	if (start > size) {
		return;
	}

	if (!(name[start] == 'p' && name[start + 1] == 'r' && name[start + 2] == 'm')) {
		if (size >= start + 5) {
			Color.red = VehicleSirenDummy::ReadHex(name[start], name[start + 1]);
			Color.green = VehicleSirenDummy::ReadHex(name[start + 2], name[start + 3]);
			Color.blue = VehicleSirenDummy::ReadHex(name[start + 4], name[start + 5]);
		}

		return;
	}

	if ((size - start) != 12) {
		return;
	}

	Color.red = VehicleSirenDummy::ReadHex(name[start + 3], name[start + 4]);
	Color.green = VehicleSirenDummy::ReadHex(name[start + 5], name[start + 6]);
	Color.blue = VehicleSirenDummy::ReadHex(name[start + 7], name[start + 8]);

	Type = name[start + 9] - '0';
	Size = ((float)(name[start + 10] - '0')) / 10.0f;

	if (Type == 1 && type != 1)
		Angle -= 180.0f;
	else if (Type != 1 && type == 1)
		Angle += 180.0f;
};

int VehicleSirenDummy::ReadHex(char a, char b) {
	a = (a <= '9') ? a - '0' : (a & 0x7) + 9;
	b = (b <= '9') ? b - '0' : (b & 0x7) + 9;

	return (a << 4) + b;
}

CVector VehicleSirenDummy::GetPosition() {
	return { Frame->modelling.pos.x, Frame->modelling.pos.y, Frame->modelling.pos.z };
};