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

    // Calculate the angle based on the frame's orientation
    Angle = Common::NormalizeAngle(CGeneral::GetATanOfXY(frame->modelling.right.x, frame->modelling.right.y) * 57.295776f);

    // Legacy support for ImVehFt vehicles
    size_t prmPos = name.find("_prm");
    if (prmPos != std::string::npos && prmPos + 11 <= name.size()) {
        Color.r = VehicleDummy::ReadHex(name[prmPos + 4], name[prmPos + 5]);
        Color.g = VehicleDummy::ReadHex(name[prmPos + 6], name[prmPos + 7]);
        Color.b = VehicleDummy::ReadHex(name[prmPos + 8], name[prmPos + 9]);

        Type = static_cast<eDummyPos>(name[prmPos + 10] - '0');
        Size = static_cast<float>(name[prmPos + 11] - '0') / 10.0f;
    }

    auto& jsonData = DataMgr::data[pVeh->m_nModelIndex];
    gLogger->error(DataMgr::data[445].dump());
    if (jsonData.contains("Lights")) {
        if (jsonData["Lights"].contains(name.c_str())) {
            auto& lights = jsonData["Lights"][name.c_str()];

            // Retrieve and set the color values
            if (lights.contains("Color")) {
                Color.r = lights["Color"].value("R", Color.r);
                Color.g = lights["Color"].value("G", Color.g);
                Color.b = lights["Color"].value("B", Color.b);
            }

            // Retrieve the size and position values
            Size = lights.value("CoronaSize", Size);
            Type = eDummyPosFromString(lights.value("DummyPos", ""));
            PartType = eParentTypeFromString(lights.value("Parent", ""));

            // Handle shadow data if available
            if (lights.contains("Shadow")) {
                auto& shadow = lights["Shadow"];
                ShdwPosition.x = shadow.value("Offset", ShdwPosition.x);
                Angle = shadow.value("Rotation", Angle); // Change "Angle" to "Rotation" based on your JSON
            }
        }
    }

}

int VehicleDummy::ReadHex(char a, char b) {
    a = (a <= '9') ? a - '0' : (a & 0x7) + 9;
    b = (b <= '9') ? b - '0' : (b & 0x7) + 9;

    return (a << 4) + b;
}