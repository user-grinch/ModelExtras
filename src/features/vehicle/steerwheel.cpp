#include "pch.h"
#include "steerwheel.h"
#define ROTATION_VAL 45.0f

SteerWheelFeature SteerWheel;

void SteerWheelFeature::Process(RwFrame* frame, CVehicle* pVeh, std::string& name) {
    
    if (NODE_FOUND(name, "steer")) {
        VehData &data = xData.Get(pVeh);
        float angle = pVeh->m_fSteerAngle * 180 / 3.1416f;
            
        if (angle > 0.1f) { // Right
            float rot = 0.0f;

            if (data.m_eRotation == eSteerWheelRotation::Left) {
                rot = -ROTATION_VAL * 2.0f;
            }else if (data.m_eRotation == eSteerWheelRotation::Default) {
                rot = -ROTATION_VAL;
            }

            if (rot != 0.0f) {
                Util::SetFrameRotationY(frame, rot);
            }
            data.m_eRotation = eSteerWheelRotation::Right;

        } else if (angle < -0.1f) { // Left
            float rot = 0.0f;

            if (data.m_eRotation == eSteerWheelRotation::Right) {
                rot = ROTATION_VAL * 2.0f;
            }else if (data.m_eRotation == eSteerWheelRotation::Default) {
                rot = ROTATION_VAL;
            }

            if (rot != 0.0f) {
                Util::SetFrameRotationY(frame, rot);
            }
            data.m_eRotation = eSteerWheelRotation::Left;

        } else if (data.m_eRotation != eSteerWheelRotation::Default) {
            float rot = data.m_eRotation == eSteerWheelRotation::Right ? ROTATION_VAL: -ROTATION_VAL;
            Util::SetFrameRotationY(frame, rot);
            data.m_eRotation = eSteerWheelRotation::Default;
        }
    }
}