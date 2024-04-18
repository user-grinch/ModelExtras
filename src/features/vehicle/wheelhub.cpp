#include "pch.h"
#include "wheelhub.h"
#define ROTATION_VAL 45.0f

WheelHubFeature WheelHub;

void UpdateRotation(RwFrame *ori, RwFrame *tar) {
    if (ori && tar) {
        float rot = Util::GetMatrixRotationZ(&ori->modelling);
        Util::SetMatrixRotationZ(&tar->modelling, rot);
    }
}

void WheelHubFeature::Process(RwFrame* frame, CVehicle* pVeh) {
    std::string name = GetFrameNodeName(frame);

    VehData &data = xData.Get(pVeh);
    if (!(data.wheellf && data.wheellm && data.wheellb && data.wheelrf && data.wheelrm && data.wheelrb)) {
        std::smatch match;
        if (name[0] == 'w' && name[1] == 'h' && name[2] == 'e' && name[3] == 'e' && name[4] == 'l'
	    && std::regex_search(name, match, std::regex("wheel_([a-zA-Z]{2})_dummy"))) {
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
            return;
        }
    }

    if (!(data.hublf && data.hublm && data.hublb && data.hubrf && data.hubrm && data.hubrb)) {
        std::smatch match;
        if (name[0] == 'h' && name[1] == 'u' && name[2] == 'b'
	    && std::regex_search(name, match, std::regex("hub_([a-zA-Z]{2})"))) {
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
    }

    UpdateRotation(data.wheellf, data.hublf);
    UpdateRotation(data.wheellm, data.hublm);
    UpdateRotation(data.wheellb, data.hublb);
    UpdateRotation(data.wheelrf, data.hubrf);
    UpdateRotation(data.wheelrm, data.hubrm);
    UpdateRotation(data.wheelrb, data.hubrb);
}