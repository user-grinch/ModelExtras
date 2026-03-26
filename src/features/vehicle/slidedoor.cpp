#include "pch.h"
#include "slidedoor.h"
#include "datamgr.h"
#include "modelinfomgr.h"

void SlideDoor::UpdateDoorGroup(CVehicle *pVeh, std::vector<DoorConfig> &configs, eDoors doorID)
{
    if (configs.empty())
        return;

    float ratio = pVeh->GetDooorAngleOpenRatio(doorID);
    float sideMult = (doorID == DOOR_FRONT_LEFT || doorID == DOOR_REAR_LEFT) ? -1.0f : 1.0f;
    float popFactor = std::min(1.0f, ratio * 5.0f);

    for (auto &config : configs)
    {
        if (!config.frame)
            continue;

        // Sliding movement (Y axis)
        config.frame->modelling.pos.y = config.mul * ratio * -1.0f;

        // Pop-out movement (X axis)
        config.frame->modelling.pos.x = popFactor * config.popOutAmount * sideMult;

        RwMatrixUpdate(&config.frame->modelling);
    }
}

void SlideDoor::Initialize()
{
    ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame)
                                {
        std::string name = GetFrameNodeName(pFrame);

        bool isLF = name.starts_with("dvan_l") || name.starts_with("dmbus_l") || name.starts_with("x_sd_lf");
        bool isRF = name.starts_with("dvan_r") || name.starts_with("dmbus_r") || name.starts_with("x_sd_rf");
        bool isLR = name.starts_with("x_sd_lr");
        bool isRR = name.starts_with("x_sd_rr");

        if (!isLF && !isRF && !isLR && !isRR) return;

        auto& jsonData = DataMgr::Get(pVeh->m_nModelIndex);
        VehData& data = xData.Get(pVeh);

        float mul = 1.0f;
        float popOutAmount = 0.15f;

        if (jsonData.contains("doors") && jsonData["doors"].contains(name)) {
            mul = jsonData["doors"][name].value("movmul", 1.0f);
            popOutAmount = jsonData["doors"][name].value("popout", 0.15f);
        }

        DoorConfig cfg = { pFrame, mul, popOutAmount };

        if (isLF)      data.leftFront.push_back(cfg);
        else if (isRF) data.rightFront.push_back(cfg);
        else if (isLR) data.leftRear.push_back(cfg);
        else if (isRR) data.rightRear.push_back(cfg); });

    ModelInfoMgr::RegisterRender([](CVehicle *pVeh)
                                 {
        if (!pVeh || !pVeh->GetIsOnScreen()) return;

        VehData& data = xData.Get(pVeh);
        UpdateDoorGroup(pVeh, data.leftFront,  eDoors::DOOR_FRONT_LEFT);
        UpdateDoorGroup(pVeh, data.rightFront, eDoors::DOOR_FRONT_RIGHT);
        UpdateDoorGroup(pVeh, data.leftRear,   eDoors::DOOR_REAR_LEFT);
        UpdateDoorGroup(pVeh, data.rightRear,  eDoors::DOOR_REAR_RIGHT); });
}