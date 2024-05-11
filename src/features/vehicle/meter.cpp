#include "pch.h"
#include "Meter.h"

void GearMeter::Process(RwFrame* frame, CEntity* ptr) {
    CVehicle *pVeh = static_cast<CVehicle*>(ptr);
    VehData &data = vehData.Get(pVeh);
    std::string name = GetFrameNodeName(frame);
    if (!data.m_bInitialized) {
        Util::StoreChilds(frame, data.m_FrameList);
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

void OdoMeter::Process(RwFrame* frame, CEntity* ptr) {
    CVehicle *pVeh = static_cast<CVehicle*>(ptr);
    VehData &data = vehData.Get(pVeh);
    std::string name = GetFrameNodeName(frame);
    if (!data.m_bInitialized) {
        Util::StoreChilds(frame, data.m_FrameList);
        data.m_nTempVal = 1234 + rand() % (57842 - 1234);

        if (NODE_FOUND(name, "_kph")) {
            data.m_fMul = 100;
        }
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
                    Util::SetFrameRotationX(data.m_FrameList[i], angle);
                }
            }
            data.m_ScreenText = showStr;
        }
    }
}

void RpmMeter::Process(RwFrame* frame, CEntity* ptr) {
    CVehicle *pVeh = static_cast<CVehicle*>(ptr);
    std::string name = GetFrameNodeName(frame);
    VehData &data = vehData.Get(pVeh);
    if (!data.m_bInitialized) {
        data.m_nMaxRpm = std::stoi(Util::GetRegexVal(name, ".*m([0-9]+).*", "100"));
        data.m_fMaxRotation = std::stof(Util::GetRegexVal(name, ".*r([0-9]+).*", "100"));
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
    Util::SetFrameRotationY(frame, change);
    data.m_fCurRotation += change;
}

void SpeedMeter::Process(RwFrame* frame, CEntity* ptr) {
    CVehicle *pVeh = static_cast<CVehicle*>(ptr);
    std::string name = GetFrameNodeName(frame);
    VehData &data = vehData.Get(pVeh);
    if (!data.m_bInitialized) {
        if (NODE_FOUND(name, "_kph")) {
            data.m_fMul= 100.0f;
        }

        data.m_nMaxSpeed = std::stoi(Util::GetRegexVal(name, ".*m([0-9]+).*", "100"));
        data.m_fMaxRotation = std::stof(Util::GetRegexVal(name, ".*r([0-9]+).*", "100"));
        data.m_bInitialized = true;
    }
    float speed = Util::GetVehicleSpeedRealistic(pVeh);
    float delta = CTimer::ms_fTimeScale;

    float totalRot = (data.m_fMaxRotation / data.m_nMaxSpeed) * speed * delta;
    totalRot = totalRot > data.m_fMaxRotation ? data.m_fMaxRotation : totalRot;
    totalRot = totalRot < 0 ? 0 : totalRot;

    float change = (totalRot - data.m_fCurRotation) * 0.5f * delta;

    Util::SetFrameRotationY(frame, change);

    data.m_fCurRotation += change;
}

void TachoMeter::Process(RwFrame* frame, CEntity* ptr) {
    CVehicle *pVeh = static_cast<CVehicle*>(ptr);
    std::string name = GetFrameNodeName(frame);
    VehData &data = vehData.Get(pVeh);
    if (!data.m_bInitialized) {
        data.m_nMaxVal = std::stoi(Util::GetRegexVal(name, ".*m([0-9]+).*", "50"));
        data.m_fMaxRotation = std::stof(Util::GetRegexVal(name, ".*r([0-9]+).*", "100"));
        data.m_bInitialized = true;
    }
    float reading = Util::GetVehicleSpeedRealistic(pVeh) / 5.0f;
    float delta = CTimer::ms_fTimeScale;

    float totalRot = (data.m_fMaxRotation / data.m_nMaxVal) * reading * delta;
    totalRot = totalRot > data.m_fMaxRotation ? data.m_fMaxRotation : totalRot;
    totalRot = totalRot < 0 ? 0 : totalRot;

    float change = (totalRot - data.m_fCurRotation) * 0.5f * delta;

    Util::SetFrameRotationY(frame, change);

    data.m_fCurRotation += change;
}

void GasMeter::Process(RwFrame* frame, CEntity* ptr) {
    CVehicle *pVeh = static_cast<CVehicle*>(ptr);
    VehData &data = vehData.Get(pVeh);
    if (data.m_bInitialized) {
        return;
    }

    std::string name = GetFrameNodeName(frame);
    Util::SetFrameRotationY(frame, Random(20.0f, 70.0f));
    data.m_bInitialized = true;
}