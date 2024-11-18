#include "pch.h"
#include "steerwheel.h"
#define ROTATION_VAL 45.0f

void SteerWheel::Process(void* ptr, RwFrame* frame, eModelEntityType type) {
    CVehicle *pVeh = static_cast<CVehicle*>(ptr);
    VehData &data = xData.Get(pVeh);
    float angle = pVeh->m_fSteerAngle * (-1.666666f);
    float maxAngle = ROTATION_VAL;
    const std::string name = GetFrameNodeName(frame);

    if (name[0] == 'f') { // vehfuncs 
        if (isdigit(name[7])) {
            angle *= (float)std::stoi(&name[7]) / 2;
        } else {
            angle *= ROTATION_VAL;
        }
    }
    else {
        float maxAngle = 1.0f;
        if (name.length() > 8 && name[8] == '_') {
            if (isdigit(name[9])) {
                maxAngle = std::stof(&name[9]);
            }
        }
        angle *= ROTATION_VAL;
        angle *= maxAngle;
    }
    angle /= 2;
    Util::SetFrameRotationY(frame, (angle-data.prevAngle)*3);
    data.prevAngle = angle;
}