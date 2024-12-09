#include "pch.h"
#include "Meter.h"

void GearMeter::Process(void* ptr, RwFrame* frame, eModelEntityType type) {
    CVehicle *pVeh = static_cast<CVehicle*>(ptr);
    VehData &data = vehData.Get(pVeh);
    if (!data.m_bInitialized) {
        Util::StoreChilds(frame, data.m_FrameList);
        data.m_bInitialized = true;
    }

    if (pVeh->m_nCurrentGear != data.m_nCurrent) {
        Util::HideAllChilds(frame);
        if (data.m_FrameList.size() > static_cast<size_t>(data.m_nCurrent)) {
            Util::ShowAllAtomics(data.m_FrameList[data.m_nCurrent]);
        }
        data.m_nCurrent = pVeh->m_nCurrentGear;
    }
}

void OdoMeter::Process(void* ptr, RwFrame* frame, eModelEntityType type) {
    CVehicle *pVeh = static_cast<CVehicle*>(ptr);
    VehData &data = vehData.Get(pVeh);
    
    if (!data.m_bInitialized) {
        std::string name = GetFrameNodeName(frame);
        Util::StoreChilds(frame, data.m_FrameList);
        data.m_nPrevRot = 1234 + rand() % (57842 - 1234);

        if (NODE_FOUND(name, "_kph")) {
            data.m_fMul = 100;
        }
        data.m_bDigital = NODE_FOUND(name, "_digital");
        data.m_bInitialized = true;
    }

    if (data.m_FrameList.size() < 6) {
        gLogger->error("Vehicle ID: {}. {} odometer childs detected, 6 expected", pVeh->m_nModelIndex, data.m_FrameList.size());
        return;
    }

    float curRot = (pVeh->m_nVehicleSubClass == VEHICLE_BIKE) ? static_cast<CBike*>(pVeh)->m_afWheelRotationX[1] : static_cast<CAutomobile*>(pVeh)->m_fWheelRotation[3];
    curRot /= (2.86 * data.m_fMul);

    int displayVal = std::stoi(data.m_ScreenText) + abs(data.m_nPrevRot - curRot);
    displayVal = plugin::Clamp(displayVal, 0, 999999);
    data.m_nPrevRot = curRot;

    std::stringstream ss;
    ss << std::setw(6) << std::setfill('0') << displayVal;
    std::string updatedText = ss.str();

    if (data.m_ScreenText != updatedText) {
        for (unsigned int i = 0; i < 6; i++) {
            if (updatedText[i] != data.m_ScreenText[i]) {
                float angle = (updatedText[i] - data.m_ScreenText[i]) * 36.0f;
                Util::SetFrameRotationX(data.m_FrameList[i], angle);
            }
        }
        data.m_ScreenText = std::move(updatedText);
    }
}

void RpmMeter::Process(void* ptr, RwFrame* frame, eModelEntityType type) {
    CVehicle *pVeh = static_cast<CVehicle*>(ptr);
    VehData &data = vehData.Get(pVeh);
    
    if (!data.m_bInitialized) {
        std::string name = GetFrameNodeName(frame);
        data.m_nMaxRpm = std::stoi(Util::GetRegexVal(name, ".*m([0-9]+).*", "100"));
        data.m_fMaxRotation = std::stof(Util::GetRegexVal(name, ".*r([0-9]+).*", "100"));
        data.m_bInitialized = true;
    }

    float rpm = 0.0f;
    float speed = Util::GetVehicleSpeedRealistic(pVeh);
    float delta = CTimer::ms_fTimeScale;

    if (pVeh->m_nCurrentGear != 0) {
        rpm += 2.0f * delta * speed / pVeh->m_nCurrentGear;
    }

    if (pVeh->m_nVehicleFlags.bEngineOn) {
        rpm += 6.0f * delta;
    }

    float newRot = (data.m_fMaxRotation / data.m_nMaxRpm) * rpm * delta * 0.50f;
    newRot = plugin::Clamp(newRot, 0, data.m_fMaxRotation);

    float change = (newRot - data.m_fCurRotation) * 0.25f * delta;
    Util::SetFrameRotationY(frame, change);
    data.m_fCurRotation += change;
}

void SpeedMeter::Process(void* ptr, RwFrame* frame, eModelEntityType type) {
    CVehicle *pVeh = static_cast<CVehicle*>(ptr);
    VehData &data = vehData.Get(pVeh);
    if (!data.m_bInitialized) {
        std::string name = GetFrameNodeName(frame);
        if (NODE_FOUND(name, "_kph")) {
            data.m_fMul= 100.0f;
        }

        data.m_nMaxSpeed = std::stoi(Util::GetRegexVal(name, ".*m([0-9]+).*", "100"));
        data.m_fMaxRotation = std::stof(Util::GetRegexVal(name, ".*r([0-9]+).*", "100"));
        data.m_bInitialized = true;
    }

    float speed = Util::GetVehicleSpeedRealistic(pVeh);
    float delta = CTimer::ms_fTimeScale;

    float newRot = (data.m_fMaxRotation / data.m_nMaxSpeed) * speed * delta;
    newRot = plugin::Clamp(newRot, 0, data.m_fMaxRotation);

    float change = (newRot - data.m_fCurRotation) * 0.5f * delta;
    Util::SetFrameRotationY(frame, change);
    data.m_fCurRotation += change;
}

void GasMeter::Process(void* ptr, RwFrame* frame, eModelEntityType type) {
    CVehicle *pVeh = static_cast<CVehicle*>(ptr);
    VehData &data = vehData.Get(pVeh);
    if (!data.m_bInitialized) {
        std::string name = GetFrameNodeName(frame);
        Util::SetFrameRotationY(frame, RandomNumberInRange(20.0f, 70.0f));
        data.m_bInitialized = true;
    }
}