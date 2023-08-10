#include "pch.h"
#include "util.h"
#include <regex>

std::string Util::GetRegexVal(const std::string& src, const std::string&& ptrn, const std::string&& def) {
    std::smatch match;
    std::regex_search(src.begin(), src.end(), match, std::regex(ptrn));

    if (match.empty())
        return def;
    else
        return match[1];

}

void Util::RotateFrameX(RwFrame* frame, float angle) {
    RwFrameRotate(frame, (RwV3d *)0x008D2E00, (RwReal)angle, rwCOMBINEPRECONCAT);
    RwFrameUpdateObjects(frame);
}

void Util::RotateFrameY(RwFrame* frame, float angle) {
    RwFrameRotate(frame, (RwV3d *)0x008D2E0C, (RwReal)angle, rwCOMBINEPRECONCAT);
    RwFrameUpdateObjects(frame);
}

void Util::RotateFrameZ(RwFrame* frame, float angle) {
    RwFrameRotate(frame, (RwV3d *)0x008D2E18, (RwReal)angle, rwCOMBINEPRECONCAT);
    RwFrameUpdateObjects(frame);
}

uint32_t Util::GetChildCount(RwFrame* parent) {
    RwFrame* child = parent->child;
    uint32_t count = 0U;
    if (child) {
        while (child) {
            ++count;
            child = child->next;
        }
        return count;
    }
    return 0U;
}

void Util::StoreChilds(RwFrame * parent_frame, std::vector<RwFrame*>& frame) {
    RwFrame* child = parent_frame->child;
    while (child) {
        const std::string name = GetFrameNodeName(child);

        frame.push_back(child);
        child = child->next;
    }
}


void Util::ShowAllAtomics(RwFrame * frame) {
    if (!rwLinkListEmpty(&frame->objectList)) {
        RwObjectHasFrame * atomic;

        RwLLLink * current = rwLinkListGetFirstLLLink(&frame->objectList);
        RwLLLink * end = rwLinkListGetTerminator(&frame->objectList);

        while (current != end) {
            atomic = rwLLLinkGetData(current, RwObjectHasFrame, lFrame);
            atomic->object.flags |= rpATOMICRENDER; // clear

            current = rwLLLinkGetNext(current);
        }
    }
    return;
}

void Util::HideAllAtomics(RwFrame * frame) {
    if (!rwLinkListEmpty(&frame->objectList)) {
        RwObjectHasFrame * atomic;

        RwLLLink * current = rwLinkListGetFirstLLLink(&frame->objectList);
        RwLLLink * end = rwLinkListGetTerminator(&frame->objectList);

        while (current != end) {
            atomic = rwLLLinkGetData(current, RwObjectHasFrame, lFrame);
            atomic->object.flags &= ~rpATOMICRENDER;

            current = rwLLLinkGetNext(current);
        }
    }
    return;
}

void Util::HideChildWithName(RwFrame *parent_frame, const char* name) {
    RwFrame* child = parent_frame->child;
    while (child) {
        if (!strcmp(GetFrameNodeName(child), name)) {
            Util::HideAllAtomics(child);
            return;
        }
        child = child->next;
    }
}

void Util::ShowChildWithName(RwFrame *parent_frame, const char* name) {
    RwFrame* child = parent_frame->child;
    while (child) {
        if (!strcmp(GetFrameNodeName(child), name)) {
            Util::ShowAllAtomics(child);
            return;
        }
        child = child->next;
    }
}

void Util::HideAllChilds(RwFrame *parent_frame) {
    RwFrame* child = parent_frame->child;
    while (child) {
        Util::HideAllAtomics(child);
        child = child->next;
    }
}

void Util::ShowAllChilds(RwFrame *parent_frame) {
    RwFrame* child = parent_frame->child;
    while (child) {
        Util::ShowAllAtomics(child);
        child = child->next;
    }
}

// Taken from vehfuncs
float Util::GetVehicleSpeedRealistic(CVehicle * vehicle) {
    float wheelSpeed = 0.0;
    CVehicleModelInfo * vehicleModelInfo = (CVehicleModelInfo *)CModelInfo::GetModelInfo(vehicle->m_nModelIndex);
    if (vehicle->m_nVehicleSubClass == VEHICLE_BIKE || vehicle->m_nVehicleSubClass == VEHICLE_BMX) {
        CBike * bike = (CBike *)vehicle;
        wheelSpeed = ((bike->m_fWheelSpeed[0] * vehicleModelInfo->m_fWheelSizeFront) +
                      (bike->m_fWheelSpeed[1] * vehicleModelInfo->m_fWheelSizeRear)) / 2.0f;
    } else if (vehicle->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || vehicle->m_nVehicleSubClass == VEHICLE_MTRUCK || vehicle->m_nVehicleSubClass == VEHICLE_QUAD) {
        CAutomobile * automobile = (CAutomobile *)vehicle;
        wheelSpeed = ((automobile->m_fWheelSpeed[0] + automobile->m_fWheelSpeed[1] * vehicleModelInfo->m_fWheelSizeFront) +
                      (automobile->m_fWheelSpeed[2] + automobile->m_fWheelSpeed[3] * vehicleModelInfo->m_fWheelSizeRear)) / 4.0f;
    } else {
        return (vehicle->m_vecMoveSpeed.Magnitude() * 50.0f) * 3.6f;
    }
    wheelSpeed /= 2.45f; // tweak based on distance (manually testing)
    wheelSpeed *= -186.0f; // tweak based on km/h

    return wheelSpeed;
}