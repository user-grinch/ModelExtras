#include "pch.h"
#include "rotatedoor.h"
#include "datamgr.h"
#include "modelinfomgr.h"

void RotateDoor::UpdateRotatingDoor(CVehicle* pVeh, DoorConfig& config, eDoors doorID)
{
    if (!config.frame) return;

    float ratio = pVeh->GetDooorAngleOpenRatio(doorID);
    float targetRot = ratio * (config.reverse ? 1.0f : -1.0f) * 90.0f - config.originalRot;

    float currentRot = MatrixUtil::GetRotationZ(&config.frame->modelling) - config.originalRot;
    MatrixUtil::SetRotationZ(&config.frame->modelling, -currentRot);
    MatrixUtil::SetRotationZ(&config.frame->modelling, targetRot);
    RwMatrixUpdate(&config.frame->modelling);
}

void RotateDoor::Initialize()
{
    ModelInfoMgr::RegisterDummy([](CVehicle* pVeh, RwFrame* pFrame)
    {
        std::string name = GetFrameNodeName(pFrame);
        if (!name.starts_with("x_rd_")) return;

        auto& jsonData = DataMgr::Get(pVeh->m_nModelIndex);
        VehData& data = xData.Get(pVeh);

        bool reverse = jsonData["doors"].contains(name)
                        ? jsonData["doors"][name].value("reverse", false)
                        : false;

        float orgRot = MatrixUtil::GetRotationZ(&pFrame->modelling);
        if (name == "x_rd_lf") {
            data.leftFront = { pFrame, orgRot, reverse };
        } else if (name == "x_rd_rf") {
            data.rightFront = { pFrame, orgRot, reverse };
        } else if (name == "x_rd_lr") {
            data.leftRear = { pFrame, orgRot, reverse };
        } else if (name == "x_rd_rr") {
            data.rightRear = { pFrame, orgRot, reverse };
        }
    });

    ModelInfoMgr::RegisterRender([](CVehicle* pVeh)
    {
        if (!pVeh || !pVeh->GetIsOnScreen()) return;

        VehData& data = xData.Get(pVeh);
        UpdateRotatingDoor(pVeh, data.leftFront, eDoors::DOOR_FRONT_LEFT);
        UpdateRotatingDoor(pVeh, data.rightFront, eDoors::DOOR_FRONT_RIGHT);
        UpdateRotatingDoor(pVeh, data.leftRear, eDoors::DOOR_REAR_LEFT);
        UpdateRotatingDoor(pVeh, data.rightRear, eDoors::DOOR_REAR_RIGHT);
    });
}
