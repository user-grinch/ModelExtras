#include "pch.h"
#include "Brakes.h"
#include "datamgr.h"


void FrontBrake::Initialize(RwFrame* pFrame, CEntity* ptr) {
    CVehicle* pVeh = static_cast<CVehicle*>(ptr);
    VehData& data = vehData.Get(pVeh);
    std::string name = GetFrameNodeName(pFrame);
    data.m_nMaxRotation = std::stoi(Util::GetRegexVal(name, ".*_(-?[0-9]+).*", "0"));
    if (data.m_nMaxRotation) {
        data.m_nMaxRotation = std::stoi(Util::GetRegexVal(name, ".*_az(-?[0-9]+).*", "0"));
    }
    auto& jsonData = DataMgr::Get(pVeh->m_nModelIndex);
    if (jsonData.contains("FrontBrake") && jsonData["FrontBrake"].contains("MaxRotation")) {
        data.m_nMaxRotation = jsonData["FrontBrake"].value("MaxRotation", 360.0f);
    }
    data.m_nWaitTime = static_cast<unsigned int>(abs(data.m_nMaxRotation / 5));
}

void RearBrake::Initialize(RwFrame* pFrame, CEntity* ptr) {
    CVehicle* pVeh = static_cast<CVehicle*>(ptr);
    VehData& data = vehData.Get(pVeh);
    std::string name = GetFrameNodeName(pFrame);
    data.m_nMaxRotation = std::stoi(Util::GetRegexVal(name, ".*_(-?[0-9]+).*", "0"));
    if (data.m_nMaxRotation) {
        data.m_nMaxRotation = std::stoi(Util::GetRegexVal(name, ".*_ax(-?[0-9]+).*", "0"));
    }
    auto& jsonData = DataMgr::Get(pVeh->m_nModelIndex);
    if (jsonData.contains("RearBrake") && jsonData["RearBrake"].contains("MaxRotation")) {
        data.m_nMaxRotation = jsonData["RearBrake"].value("MaxRotation", 360.0f);
    }
    data.m_nWaitTime = static_cast<unsigned int>(abs(data.m_nMaxRotation / 5));

}

void FrontBrake::Process(void* ptr, RwFrame* frame, eModelEntityType type) {
    CVehicle* pVeh = static_cast<CVehicle*>(ptr);
    VehData& data = vehData.Get(pVeh);

    if (!data.m_bInitialized) {
        Initialize(frame, pVeh);
        data.m_bInitialized = true;
    }

    uint timer = static_cast<int>(CTimer::m_snTimeInMilliseconds * CTimer::ms_fTimeScale);
    uint deltaTime = (timer - data.m_nLastFrameMS);

    if (deltaTime > data.m_nWaitTime) {
        float temp;
        if (pVeh->m_nVehicleFlags.bIsHandbrakeOn && data.m_nCurRotation != data.m_nMaxRotation) {
            if (data.m_nMaxRotation < data.m_nCurRotation) {
                data.m_nCurRotation -= 1;
                temp = -1;
            }
            else {
                data.m_nCurRotation += 1;
                temp = 1;
            }

            Util::SetFrameRotationZ(frame, temp);
            data.m_nLastFrameMS = timer;
        }
        else {
            if (data.m_nCurRotation != 0) {
                if (data.m_nMaxRotation > 0) {
                    data.m_nCurRotation -= 1;
                    temp = -1;
                }
                else {
                    data.m_nCurRotation += 1;
                    temp = 1;
                }

                Util::SetFrameRotationZ(frame, temp);
                data.m_nLastFrameMS = timer;
            }
        }
    }
}

void RearBrake::Process(void* ptr, RwFrame* frame, eModelEntityType type) {
    CVehicle* pVeh = static_cast<CVehicle*>(ptr);
    VehData& data = vehData.Get(pVeh);

    if (!data.m_bInitialized) {
        Initialize(frame, pVeh);
        data.m_bInitialized = true;
    }

    uint timer = static_cast<int>(CTimer::m_snTimeInMilliseconds * CTimer::ms_fTimeScale);
    uint deltaTime = (timer - data.m_nLastFrameMS);

    if (deltaTime > data.m_nWaitTime) {
        float temp;
        if (pVeh->m_fBreakPedal && data.m_nCurRotation != data.m_nMaxRotation) {
            if (data.m_nMaxRotation < data.m_nCurRotation) {
                data.m_nCurRotation -= 1;
                temp = -1;
            }
            else {
                data.m_nCurRotation += 1;
                temp = 1;
            }

            Util::SetFrameRotationX(frame, temp);
            data.m_nLastFrameMS = timer;
        }
        else {
            if (data.m_nCurRotation != 0) {
                if (data.m_nMaxRotation > 0) {
                    data.m_nCurRotation -= 1;
                    temp = -1;
                }
                else {
                    data.m_nCurRotation += 1;
                    temp = 1;
                }

                Util::SetFrameRotationX(frame, temp);
                data.m_nLastFrameMS = timer;
            }
        }
    }
}