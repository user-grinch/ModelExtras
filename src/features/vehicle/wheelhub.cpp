#include "pch.h"
#include "wheelhub.h"

void UpdateRotation(RwFrame *ori, RwFrame *tar) {
    if (ori && tar) {
        float rot = Util::GetMatrixRotationZ(&ori->modelling);
        Util::SetMatrixRotationZ(&tar->modelling, rot);
    }
}

void WheelHub::FindNodes(RwFrame * frame, CEntity* ptr) {
    if(frame) {
        const std::string name = GetFrameNodeName(frame);
        CVehicle *pVeh = static_cast<CVehicle*>(ptr);
        std::string name = GetFrameNodeName(frame);

        VehData &data = xData.Get(pVeh);
        std::smatch match;
        if (std::regex_search(name, match, std::regex("wheel_([a-zA-Z]{2})_dummy"))) {
            std::string str = match[1].str();
            if (std::toupper(str[0]) == 'R') {
                if (std::toupper(str[1]) == 'F') {
                    data.wheelrf = frame;
                } else if (std::toupper(str[1]) == 'M') {
                    data.wheelrm = frame;
                } else if (std::toupper(str[1]) == 'B') {
                    data.wheelrb = frame;
                }
            } else if (std::toupper(str[0]) == 'L') {
                if (std::toupper(str[1]) == 'F') {
                    data.wheellf = frame;
                } else if (std::toupper(str[1]) == 'M') {
                    data.wheellm = frame;
                } else if (std::toupper(str[1]) == 'B') {
                    data.wheellb = frame;
                }
            }
        }

        if (std::regex_search(name, match, std::regex("hub_([a-zA-Z]{2})"))) {
            std::string str = match[1].str();
            if (std::toupper(str[0]) == 'R') {
                if (std::toupper(str[1]) == 'F') {
                    data.hubrf = frame;
                } else if (std::toupper(str[1]) == 'M') {
                    data.hubrm = frame;
                } else if (std::toupper(str[1]) == 'B') {
                    data.hubrb = frame;
                }
            } else if (std::toupper(str[0]) == 'L') {
                if (std::toupper(str[1]) == 'F') {
                    data.hublf = frame;
                } else if (std::toupper(str[1]) == 'M') {
                    data.hublm = frame;
                } else if (std::toupper(str[1]) == 'B') {
                    data.hublb = frame;
                }
            }
        }

        if (RwFrame * newFrame = frame->child) {
            FindNodes(newFrame, ptr);
        }
        if (RwFrame * newFrame = frame->next) {
            FindNodes(newFrame, ptr);
        }
    }
    return;
}

void WheelHub::Process(RwFrame* frame, CEntity* ptr) {
    CVehicle *pVeh = static_cast<CVehicle*>(ptr);

    VehData &data = xData.Get(pVeh);
    if (!data.m_bInit) {
        FindNodes(frame, ptr);
        data.m_bInit = true;
    }

    UpdateRotation(data.wheellf, data.hublf);
    UpdateRotation(data.wheellm, data.hublm);
    UpdateRotation(data.wheellb, data.hublb);
    UpdateRotation(data.wheelrf, data.hubrf);
    UpdateRotation(data.wheelrm, data.hubrm);
    UpdateRotation(data.wheelrb, data.hubrb);
}