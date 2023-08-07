#include "pch.h"
#include "Meter.h"

GearMeterFeature GearMeter;
void GearMeterFeature::Initialize(RwFrame* pFrame, CVehicle* pVeh) {
    VehData &data = vehData.Get(pVeh);
    Util::StoreChilds(pFrame, data.m_FrameList);

}

void GearMeterFeature::Process(RwFrame* frame, CVehicle* pVeh) {
    VehData &data = vehData.Get(pVeh);
    std::string name = GetFrameNodeName(frame);
    if (name.find("x_gearmeter") != std::string::npos) {
        if (!data.m_bInitialized) {
            Initialize(frame, pVeh);
            data.m_bInitialized = true;
        }

        if (pVeh->m_nCurrentGear != data.m_nCurrent) {
            data.m_nCurrent = pVeh->m_nCurrentGear;

            Util::HideAllChilds(frame);
            if (data.m_FrameList.size() > static_cast<size_t>(data.m_nCurrent)) {
                Util::ShowAllAtomics(data.m_FrameList[data.m_nCurrent]);
            }
        }
    }
}

OdoMeterFeature OdoMeter;

void OdoMeterFeature::Initialize(RwFrame* pFrame, CVehicle* pVeh) {
    VehData &data = vehData.Get(pVeh);
    Util::StoreChilds(pFrame, data.m_FrameList);
    data.m_nTempVal = 1234 + rand() % (57842 - 1234);

    std::string name = GetFrameNodeName(pFrame);
    if (name.find("_kph") != std::string::npos) {
        data.m_fMul = 100;
    }

}

void OdoMeterFeature::Process(RwFrame* frame, CVehicle* pVeh) {
    VehData &data = vehData.Get(pVeh);
    std::string name = GetFrameNodeName(frame);
    if (name.find("x_ometer") != std::string::npos) {
        if (!data.m_bInitialized) {
            Initialize(frame, pVeh);
            data.m_bInitialized = true;
        }

        // Calculate new value
        int rot = 0;
        if (pVeh->m_nVehicleSubClass == VEHICLE_BIKE) {
            CBike *bike = (CBike*)pVeh;
            rot = static_cast<int>(bike->m_afWheelRotationX[1]);
        } else {
            CAutomobile *am = (CAutomobile*)pVeh;
            rot = static_cast<int>(am->m_fWheelRotation[3]);
        }

        int rotVal = static_cast<int>((rot / (2.86* data.m_fMul)));
        int val = std::stoi(data.m_ScreenText) + abs(data.m_nTempVal - rotVal);
        data.m_nTempVal = rotVal;

        if (val < 999999) {
            std::string showStr = std::to_string(val);

            // 1 -> 000001
            while (showStr.size() < 6) {
                showStr = "0" + showStr;
            }

            if (data.m_ScreenText != showStr) {
                // Update odometer value
                for (unsigned int i = 0; i < 6; i++) {
                    if (showStr[i] != data.m_ScreenText[i]) {
                        float angle = (std::stof(std::to_string(showStr[i])) - std::stof(std::to_string(data.m_ScreenText[i]))) * 36.0f;
                        Util::RotateFrameX(data.m_FrameList[i], angle);
                    }
                }
                data.m_ScreenText = showStr;
            }
        }
    }
}

RpmMeterFeature RpmMeter;

void RpmMeterFeature::Initialize(RwFrame* pFrame, CVehicle* pVeh) {
    VehData &data = vehData.Get(pVeh);
    std::string name = GetFrameNodeName(pFrame);
    data.m_nMaxRpm = std::stoi(Util::GetRegexVal(name, ".*m([0-9]+).*", "100"));
    data.m_fMaxRotation = std::stof(Util::GetRegexVal(name, ".*r([0-9]+).*", "100"));

}

void RpmMeterFeature::Process(RwFrame* frame, CVehicle* pVeh) {
    std::string name = GetFrameNodeName(frame);
    if (name.find("x_rpm") != std::string::npos) {
        VehData &data = vehData.Get(pVeh);
        if (!data.m_bInitialized) {
            Initialize(frame, pVeh);
            data.m_bInitialized = true;
        }

        float rpm = 0.0f;
        float speed = Util::GetVehicleSpeedRealistic(pVeh);
        float delta = CTimer::ms_fTimeScale;

        if (pVeh->m_nCurrentGear != 0)
            rpm += 2.0f * delta * speed / pVeh->m_nCurrentGear;

        if (pVeh->m_nVehicleFlags.bEngineOn)
            rpm += 6.0f * delta;

        float new_rot = (data.m_fMaxRotation / data.m_nMaxRpm) * rpm * delta * 0.50f;
        new_rot = new_rot > data.m_fMaxRotation ? data.m_fMaxRotation : new_rot;
        new_rot = new_rot < 0 ? 0 : new_rot;

        float change = (new_rot - data.m_fCurRotation) * 0.25f * delta;
        Util::RotateFrameY(frame, change);
        data.m_fCurRotation += change;
    }
}

SpeedMeterFeature SpeedMeter;

void SpeedMeterFeature::Initialize(RwFrame* pFrame, CVehicle* pVeh) {
    VehData &data = vehData.Get(pVeh);
    std::string name = GetFrameNodeName(pFrame);
    if (name.find("_kph") != std::string::npos) {
        data.m_fMul= 100.0f;
    }

    data.m_nMaxSpeed = std::stoi(Util::GetRegexVal(name, ".*m([0-9]+).*", "100"));
    data.m_fMaxRotation = std::stof(Util::GetRegexVal(name, ".*r([0-9]+).*", "100"));
}

void SpeedMeterFeature::Process(RwFrame* frame, CVehicle* pVeh) {
    std::string name = GetFrameNodeName(frame);
    if (name.find("x_sm") != std::string::npos) {
        VehData &data = vehData.Get(pVeh);
        float speed = Util::GetVehicleSpeedRealistic(pVeh);
        float delta = CTimer::ms_fTimeScale;

        float totalRot = (data.m_fMaxRotation / data.m_nMaxSpeed) * speed * delta;
        totalRot = totalRot > data.m_fMaxRotation ? data.m_fMaxRotation : totalRot;
        totalRot = totalRot < 0 ? 0 : totalRot;

        float change = (totalRot - data.m_fCurRotation) * 0.5f * delta;

        Util::RotateFrameY(frame, change);

        data.m_fCurRotation += change;
    }
}