#include "Gear.h"
#include "datamgr.h"
#include "modelinfomgr.h"
#include "pch.h"
#include <extensions/ScriptCommands.h>

void Clutch::Initialize() {
  ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame) {
    std::string name = GetFrameNodeName(pFrame);
    if (name.starts_with("x_clutch") || name.starts_with("fc_cl")) {
      VehData &data = vehData.Get(pVeh);
      auto &jsonData = DataMgr::Get(pVeh->m_nModelIndex);
      if (jsonData.contains("clutch") &&
          jsonData["clutch"].contains("offset")) {
        data.m_nCurOffset =
            jsonData["clutch"].value("offset", data.m_nCurOffset);
      }
      data.m_nWaitTime = static_cast<unsigned int>(abs(data.m_nCurOffset / 10));
      data.pFrame = pFrame;
      data.m_bInitialized = true;
    }
  });

  ModelInfoMgr::RegisterRender([](CVehicle *pVeh) {
    if (!pVeh || !pVeh->GetIsOnScreen())
      return;

    auto &data = vehData.Get(pVeh);
    if (!data.m_bInitialized || !data.pFrame)
      return;

    uint now = CTimer::m_snTimeInMilliseconds;
    if (now - data.m_nLastFrameMS <= data.m_nWaitTime)
      return;

    const float clutchMax = float(data.m_nCurOffset);
    float &rot = data.m_fCurRotation;

    switch (data.m_eState) {
    case eFrameState::AtOrigin:
      if (pVeh->m_nCurrentGear != data.m_nLastGear) {
        data.m_nLastGear = pVeh->m_nCurrentGear;
        data.m_eState = eFrameState::IsMoving;
      }
      break;

    case eFrameState::IsMoving:
      rot += 1.0f;
      if (rot >= clutchMax) {
        rot = clutchMax;
        data.m_eState = eFrameState::AtOffset;
      }
      data.m_fCalVal = 1.0f;
      FrameUtil::SetRotationZ(data.pFrame, data.m_fCalVal);
      data.m_nLastFrameMS = now;
      break;

    case eFrameState::AtOffset:
      rot -= 1.0f;
      if (rot <= 0.0f) {
        rot = 0.0f;
        data.m_eState = eFrameState::AtOrigin;
      }
      data.m_fCalVal = -1.0f;
      FrameUtil::SetRotationZ(data.pFrame, data.m_fCalVal);
      data.m_nLastFrameMS = now;
      break;
    }
  });
}

void GearLever::Initialize() {
  ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame) {
    std::string name = GetFrameNodeName(pFrame);
    if (name.starts_with("x_gearlever") || name.starts_with("fc_gl")) {
      VehData &data = vehData.Get(pVeh);
      auto &jsonData = DataMgr::Get(pVeh->m_nModelIndex);
      if (jsonData.contains("gearlever") &&
          jsonData["gearlever"].contains("offset")) {
        data.m_nCurOffset =
            jsonData["gearlever"].value("offset", data.m_nCurOffset);
      }
      data.m_nWaitTime = static_cast<unsigned int>(abs(data.m_nCurOffset / 10));
      data.pFrame = pFrame;
      data.m_bInitialized = true;
    }
  });

  ModelInfoMgr::RegisterRender([](CVehicle *pVeh) {
    if (!pVeh || !pVeh->GetIsOnScreen())
      return;

    VehData &data = vehData.Get(pVeh);
    if (data.m_bInitialized && data.pFrame) {
      uint timer = CTimer::m_snTimeInMilliseconds;
      uint deltaTime = (timer - data.m_nLastFrameMS);

      if (deltaTime > data.m_nWaitTime) {
        if (data.m_eState == eFrameState::AtOrigin) {
          if (pVeh->m_nCurrentGear != data.m_nLastGear) {
            data.m_fCalVal =
                (pVeh->m_nCurrentGear > data.m_nLastGear) ? -1.0f : 1.0f;
            data.m_nLastGear = pVeh->m_nCurrentGear;
            data.m_eState = eFrameState::IsMoving;
          }
        } else {
          if (data.m_eState == eFrameState::AtOffset) {
            if (data.m_fCurRotation != 0) {
              if (data.m_fCurRotation > 0) {
                data.m_fCurRotation -= 1;
                data.m_fCalVal = -1;
              } else {
                data.m_fCurRotation += 1;
                data.m_fCalVal = 1;
              }
              FrameUtil::SetRotationX(data.pFrame, data.m_fCalVal);
            } else {
              data.m_eState = eFrameState::AtOrigin;
            }
          } else {
            if (data.m_nCurOffset != abs(data.m_fCurRotation)) {
              data.m_fCurRotation += data.m_fCalVal;
              FrameUtil::SetRotationX(data.pFrame, data.m_fCalVal);
            } else {
              data.m_eState = eFrameState::AtOffset;
            }
          }
          data.m_nLastFrameMS = timer;
        }
      }
    }
  });
}