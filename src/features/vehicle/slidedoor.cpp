#include "SlideDoor.h"
#include "datamgr.h"
#include "modelinfomgr.h"
#include "pch.h"

void SlideDoor::UpdateSlidingDoor(CVehicle *pVeh, DoorConfig &config,
                                  eDoors doorID) {
  if (!config.frame)
    return;

  float ratio = pVeh->GetDooorAngleOpenRatio(doorID);
  config.frame->modelling.pos.y = config.reverse ? ratio : -ratio;
  RwMatrixUpdate(&config.frame->modelling);
}

void SlideDoor::Initialize() {
  ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame) {
    std::string name = GetFrameNodeName(pFrame);
    if (!name.starts_with("dvan_") && !name.starts_with("dmbus_") &&
        !name.starts_with("x_sd_"))
      return;

    auto &jsonData = DataMgr::Get(pVeh->m_nModelIndex);
    VehData &data = xData.Get(pVeh);

    bool reverse = jsonData["doors"].contains(name)
                       ? jsonData["doors"][name].value("reverse", false)
                       : false;

    if (name == "dvan_l" || name == "dmbus_l" || name == "x_sd_lf") {
      data.leftFront = {pFrame, reverse};
    } else if (name == "dvan_r" || name == "dmbus_r" || name == "x_sd_rf") {
      data.rightFront = {pFrame, reverse};
    } else if (name == "x_sd_lr") {
      data.leftRear = {pFrame, reverse};
    } else if (name == "x_sd_rr") {
      data.rightRear = {pFrame, reverse};
    }
  });

  ModelInfoMgr::RegisterRender([](CVehicle *pVeh) {
    if (!pVeh || !pVeh->GetIsOnScreen())
      return;

    VehData &data = xData.Get(pVeh);
    UpdateSlidingDoor(pVeh, data.leftFront, eDoors::DOOR_FRONT_LEFT);
    UpdateSlidingDoor(pVeh, data.rightFront, eDoors::DOOR_FRONT_RIGHT);
    UpdateSlidingDoor(pVeh, data.leftRear, eDoors::DOOR_REAR_LEFT);
    UpdateSlidingDoor(pVeh, data.rightRear, eDoors::DOOR_REAR_RIGHT);
  });
}
