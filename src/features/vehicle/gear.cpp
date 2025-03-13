#include "pch.h"
#include "Gear.h"
#include "bass.h"
#include "../../audio/soundsystem.h"
#include "../../audio/audiostream.h"

void Clutch::Process(void* ptr, RwFrame* frame, eModelEntityType type) {
    CVehicle* pVeh = static_cast<CVehicle*>(ptr);
    VehData& data = vehData.Get(pVeh);
    std::string name = GetFrameNodeName(frame);

    if (!data.m_bInitialized) {
        data.m_nCurOffset = std::stoi(Util::GetRegexVal(name, ".*_(-?[0-9]+).*", "0"));
        data.m_nWaitTime = static_cast<unsigned int>(abs(data.m_nCurOffset / 10));
        data.m_bInitialized = true;
    }

    uint timer = CTimer::m_snTimeInMilliseconds;
    uint deltaTime = (timer - data.m_nLastFrameMS);

    if (deltaTime > data.m_nWaitTime) {
        if (data.m_eState == eFrameState::AtOrigin) {
            if (pVeh->m_nCurrentGear != data.m_nLastGear) {
                data.m_nLastGear = pVeh->m_nCurrentGear;
                data.m_eState = eFrameState::IsMoving;
            }
        }
        else {
            if (data.m_eState == eFrameState::AtOffset) {
                if (data.m_fCurRotation == 0) {
                    data.m_eState = eFrameState::AtOrigin;
                }
                else {
                    if (data.m_nCurOffset > 0) {
                        data.m_fCurRotation -= 1;
                        data.m_fCalVal = -1;
                    }
                    else {
                        data.m_fCurRotation += 1;
                        data.m_fCalVal = 1;
                    }

                    Util::SetFrameRotationZ(frame, data.m_fCalVal);
                    data.m_nLastFrameMS = timer;
                }
            }
            else {
                if (data.m_fCurRotation == data.m_nCurOffset) {
                    data.m_eState = eFrameState::AtOffset;
                }
                else {
                    if (data.m_nCurOffset < data.m_fCurRotation) {
                        data.m_fCurRotation -= 1;
                        data.m_fCalVal = -1;
                    }
                    else {
                        data.m_fCurRotation += 1;
                        data.m_fCalVal = 1;
                    }

                    Util::SetFrameRotationZ(frame, data.m_fCalVal);
                    data.m_nLastFrameMS = timer;
                }
            }
        }
    }
}

void GearLever::Process(void* ptr, RwFrame* frame, eModelEntityType type) {
    CVehicle* pVeh = static_cast<CVehicle*>(ptr);
    VehData& data = vehData.Get(pVeh);
    std::string name = GetFrameNodeName(frame);
    if (!data.m_bInitialized) {
        data.m_nCurOffset = std::stoi(Util::GetRegexVal(name, ".*_(-?[0-9]+).*", "0"));
        if (data.m_nCurOffset == 0) {
            data.m_nCurOffset = std::stoi(Util::GetRegexVal(name, ".*_o(-?[0-9]+).*", "0"));
        }
        data.m_nWaitTime = static_cast<unsigned int>(abs(data.m_nCurOffset / 10));
        data.m_bInitialized = true;
    }
    uint timer = CTimer::m_snTimeInMilliseconds;
    uint deltaTime = (timer - data.m_nLastFrameMS);

    if (deltaTime > data.m_nWaitTime) {
        if (data.m_eState == eFrameState::AtOrigin) {
            if (pVeh->m_nCurrentGear != data.m_nLastGear) {
                data.m_fCalVal = (pVeh->m_nCurrentGear > data.m_nLastGear) ? -1.0f : 1.0f;
                data.m_nLastGear = pVeh->m_nCurrentGear;
                data.m_eState = eFrameState::IsMoving;
            }
        }
        else {
            if (data.m_eState == eFrameState::AtOffset) {
                if (data.m_fCurRotation != 0) {
                    if (data.m_fCurRotation > 0) {
                        data.m_fCurRotation -= 1;
                        data.m_fCalVal = -1;
                    }
                    else {
                        data.m_fCurRotation += 1;
                        data.m_fCalVal = 1;
                    }
                    Util::SetFrameRotationX(frame, data.m_fCalVal);
                }
                else {
                    data.m_eState = eFrameState::AtOrigin;
                }
            }
            else {
                if (data.m_nCurOffset != abs(data.m_fCurRotation)) {
                    data.m_fCurRotation += data.m_fCalVal;
                    Util::SetFrameRotationX(frame, data.m_fCalVal);
                }
                else {
                    data.m_eState = eFrameState::AtOffset;
                }
            }

            data.m_nLastFrameMS = timer;
        }
    }
}

void GearSound::Process(void* ptr, RwFrame* frame, eModelEntityType type) {
    CVehicle* pVeh = static_cast<CVehicle*>(ptr);
    std::string name = GetFrameNodeName(frame);
    VehData& data = vehData.Get(pVeh);
    if (!data.m_bInitialized) {
        std::string upPath = MOD_DATA_PATH_S(std::format("audio/gear/{}.wav", Util::GetRegexVal(name, "x_gs_(.*$)", "")));
        data.m_pUpAudio = SoundSystem.CreateStream(upPath.c_str(), false);
        data.m_pUpAudio->SetVolume(0.5f);
        data.m_bInitialized = true;
    }
    if (data.m_nCurGear != pVeh->m_nCurrentGear) {
        data.m_pUpAudio->Play();
        data.m_nCurGear = pVeh->m_nCurrentGear;
    }
}