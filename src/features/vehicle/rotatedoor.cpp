#include "pch.h"
#include "rotatedoor.h"
#include "datamgr.h"
#include "modelinfomgr.h"

void RotateDoor::UpdateSingleFrame(CVehicle* pVeh, DoorConfig& config, eDoors doorID, bool isBootBonnet)
{
    if (!config.frame) return;

    float ratio = pVeh->GetDooorAngleOpenRatio(doorID);
    float popFactor = std::min(1.0f, ratio * 5.0f);

    if (isBootBonnet) {
        // Boot/Bonnet Logic (Z-Translation, X-Rotation)
        config.frame->modelling.pos.z = popFactor * config.popOutAmount;
        float targetRot = ratio * config.mul * 45.0f;
        MatrixUtil::SetRotationXAbsolute(&config.frame->modelling, targetRot - config.prevRot);
        config.prevRot = targetRot;
    } else {
        // Side Door Logic (X-Translation, Z-Rotation)
        float sideMult = (doorID == DOOR_FRONT_LEFT || doorID == DOOR_REAR_LEFT) ? 1.0f : -1.0f;
        config.frame->modelling.pos.x = popFactor * config.popOutAmount * sideMult;
        float targetRot = ratio * config.mul * 90.0f * sideMult;
        MatrixUtil::SetRotationZAbsolute(&config.frame->modelling, targetRot - config.prevRot);
        config.prevRot = targetRot;
    }

    RwMatrixUpdate(&config.frame->modelling);
}

void RotateDoor::UpdateDoorGroup(CVehicle* pVeh, std::vector<DoorConfig>& configs, eDoors doorID, bool isBootBonnet)
{
    for (auto& config : configs) {
        UpdateSingleFrame(pVeh, config, doorID, isBootBonnet);
    }
}

void RotateDoor::Initialize()
{
    ModelInfoMgr::RegisterDummy([](CVehicle* pVeh, RwFrame* pFrame)
    {
        std::string name = GetFrameNodeName(pFrame);
        if (!name.starts_with("x_rd_")) return;

        auto& jsonData = DataMgr::Get(pVeh->m_nModelIndex);
        VehData& data = xData.Get(pVeh);

        float mul = 1.0f;
        float popOutAmount = 0.0f;

        if (jsonData.contains("doors") && jsonData["doors"].contains(name)) {
            mul = jsonData["doors"][name].value("mul", 1.0f);
            popOutAmount = jsonData["doors"][name].value("popout", 0.15f);
        }

        float orgRot = MatrixUtil::GetRotationZ(&pFrame->modelling);
        DoorConfig cfg = { pFrame, orgRot, mul, popOutAmount };

        if (name.starts_with("x_rd_lf"))      data.leftFront.push_back(cfg);
        else if (name.starts_with("x_rd_rf")) data.rightFront.push_back(cfg);
        else if (name.starts_with("x_rd_lr")) data.leftRear.push_back(cfg);
        else if (name.starts_with("x_rd_rr")) data.rightRear.push_back(cfg);
        else if (name.starts_with("x_rd_boot"))   data.boot.push_back(cfg);
        else if (name.starts_with("x_rd_bonnet")) data.bonnet.push_back(cfg);
    });

    ModelInfoMgr::RegisterRender([](CVehicle* pVeh)
    {
        if (!pVeh || !pVeh->GetIsOnScreen()) return;

        VehData& data = xData.Get(pVeh);

        UpdateDoorGroup(pVeh, data.leftFront,  DOOR_FRONT_LEFT,  false);
        UpdateDoorGroup(pVeh, data.rightFront, DOOR_FRONT_RIGHT, false);
        UpdateDoorGroup(pVeh, data.leftRear,   DOOR_REAR_LEFT,   false);
        UpdateDoorGroup(pVeh, data.rightRear,  DOOR_REAR_RIGHT,  false);
        UpdateDoorGroup(pVeh, data.boot,       BOOT,             true);
        UpdateDoorGroup(pVeh, data.bonnet,     BONNET,           true);
    });
}