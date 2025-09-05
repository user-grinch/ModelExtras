#include "Meter.h"
#include "datamgr.h"
#include "modelinfomgr.h"
#include "pch.h"
#include <CBike.h>

void GearMeter::Initialize() {
  ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame) {
    std::string name = GetFrameNodeName(pFrame);
    if (name.starts_with("x_gearmeter") || name.starts_with("fc_gm")) {
      VehData &data = vehData.Get(pVeh);
      data.pRoot = pFrame;
      FrameUtil::StoreChilds(pFrame, data.m_FrameList);
    }
  });

  ModelInfoMgr::RegisterRender([](CVehicle *pVeh) {
    if (!pVeh || !pVeh->GetIsOnScreen())
      return;

    VehData &data = vehData.Get(pVeh);
    if (!data.m_FrameList.empty() && pVeh->m_nCurrentGear != data.m_nCurrent) {
      FrameUtil::HideAllChilds(data.pRoot);
      if (data.m_FrameList.size() > static_cast<size_t>(data.m_nCurrent)) {
        FrameUtil::ShowAllAtomics(data.m_FrameList[data.m_nCurrent]);
      }
      data.m_nCurrent = pVeh->m_nCurrentGear;
    }
  });
}

void OdoMeter::Initialize() {
  ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame) {
    std::string name = GetFrameNodeName(pFrame);
    if (name.starts_with("x_odometer") || name.starts_with("fc_om")) {
      VehData &data = vehData.Get(pVeh);
      FrameUtil::StoreChilds(pFrame, data.m_FrameList);
      data.m_nPrevRot = 1234 + rand() % (57842 - 1234);

      auto &jsonData = DataMgr::Get(pVeh->m_nModelIndex);
      if (jsonData.contains("odometer")) {
        if (jsonData["odometer"].contains("kph")) {
          data.m_fMul = jsonData["odometer"]["kph"].get<bool>() ? 100 : 1;
        }
        if (jsonData["odometer"].contains("digital")) {
          data.m_bDigital = jsonData["odometer"]["digital"].get<bool>();
        }
      }
      data.pFrame = pFrame;
      data.m_bInitialized = true;
    }
  });

  ModelInfoMgr::RegisterRender([](CVehicle *pVeh) {
    if (!pVeh || !pVeh->GetIsOnScreen())
      return;

    VehData &data = vehData.Get(pVeh);
    if (data.m_bInitialized && data.pFrame) {
      if (data.m_FrameList.size() < 6) {
        LOG_VERBOSE("Vehicle ID: {}. {} odometer childs detected, 6 expected",
                    pVeh->m_nModelIndex, data.m_FrameList.size());
        return;
      }

      float curRot =
          (pVeh->m_nVehicleSubClass == VEHICLE_BIKE)
              ? static_cast<CBike *>(pVeh)->m_afWheelRotationX[1]
              : static_cast<CAutomobile *>(pVeh)->m_fWheelRotation[3];
      curRot /= (2.86 * data.m_fMul);

      int displayVal =
          std::stoi(data.m_ScreenText) + abs(data.m_nPrevRot - curRot);
      displayVal = plugin::Clamp(displayVal, 0, 999999);
      data.m_nPrevRot = curRot;

      std::stringstream ss;
      ss << std::setw(6) << std::setfill('0') << displayVal;
      std::string updatedText = ss.str();

      if (data.m_ScreenText != updatedText) {
        for (unsigned int i = 0; i < 6; i++) {
          if (updatedText[i] != data.m_ScreenText[i]) {
            float angle = (updatedText[i] - data.m_ScreenText[i]) * 36.0f;
            FrameUtil::SetRotationX(data.m_FrameList[i], angle);
          }
        }
        data.m_ScreenText = std::move(updatedText);
      }
    }
  });
}

void RpmMeter::Initialize() {
  ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame) {
    std::string name = GetFrameNodeName(pFrame);
    if (name.starts_with("x_rpm") || name.starts_with("fc_rpm") ||
        name.starts_with("tahook")) {
      VehData &data = vehData.Get(pVeh);
      auto &jsonData = DataMgr::Get(pVeh->m_nModelIndex);
      if (jsonData.contains("rpmmeter")) {
        if (jsonData["rpmmeter"].contains("maxrpm")) {
          data.m_nMaxRpm = jsonData["rpmmeter"].value("maxrpm", data.m_nMaxRpm);
        }
        if (jsonData["rpmmeter"].contains("maxrotation")) {
          data.m_fMaxRotation =
              jsonData["rpmmeter"].value("maxrotation", data.m_fMaxRotation);
        }
      }
      data.pFrame = pFrame;
      data.m_bInitialized = true;
    }
  });

  ModelInfoMgr::RegisterRender([](CVehicle *pVeh) {
    if (!pVeh || !pVeh->GetIsOnScreen())
      return;

    VehData &data = vehData.Get(pVeh);
    if (data.m_bInitialized && data.pFrame) {
      float delta = CTimer::ms_fTimeScale;
      float speed = Util::GetVehicleSpeedRealistic(pVeh);
      float rpm = 0.0f;

      if (pVeh->m_nCurrentGear > 0) {
        rpm = (speed / pVeh->m_nCurrentGear) * 100.0f;
      }

      if (pVeh->bEngineOn) {
        rpm = std::max(rpm, 0.1f * data.m_nMaxRpm);
      }

      rpm = plugin::Clamp(rpm, 0.0f, static_cast<float>(data.m_nMaxRpm));

      float targetRotation = (rpm / data.m_nMaxRpm) * data.m_fMaxRotation;
      targetRotation = plugin::Clamp(targetRotation, 0.0f, data.m_fMaxRotation);

      float change = (targetRotation - data.m_fCurRotation) * 0.25f * delta;
      FrameUtil::SetRotationY(data.pFrame, change);
      data.m_fCurRotation += change;
    }
  });
}

void SpeedMeter::Initialize() {
  ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame) {
    std::string name = GetFrameNodeName(pFrame);
    if (name.starts_with("x_sm") || name.starts_with("fc_sm") ||
        name.starts_with("speedook")) {
      VehData &data = vehData.Get(pVeh);
      auto &jsonData = DataMgr::Get(pVeh->m_nModelIndex);
      if (jsonData.contains("speedmeter")) {
        if (jsonData["speedmeter"].contains("kph")) {
          data.m_fMul = jsonData["speedmeter"]["kph"].get<bool>() ? 100 : 1;
        }
        if (jsonData["speedmeter"].contains("maxspeed")) {
          data.m_nMaxSpeed =
              jsonData["speedmeter"].value("maxspeed", data.m_nMaxSpeed);
        }
        if (jsonData["speedmeter"].contains("maxrotation")) {
          data.m_fMaxRotation =
              jsonData["speedmeter"].value("maxrotation", data.m_fMaxRotation);
        }
      }
      data.pFrame = pFrame;
      data.m_bInitialized = true;
    }
  });

  ModelInfoMgr::RegisterRender([](CVehicle *pVeh) {
    if (!pVeh || !pVeh->GetIsOnScreen())
      return;

    VehData &data = vehData.Get(pVeh);
    if (data.m_bInitialized && data.pFrame) {
      float speed = Util::GetVehicleSpeedRealistic(pVeh);
      float delta = CTimer::ms_fTimeScale;

      float newRot = (data.m_fMaxRotation / data.m_nMaxSpeed) * speed * delta;
      newRot = plugin::Clamp(newRot, 0, data.m_fMaxRotation);

      float change = (newRot - data.m_fCurRotation) * 0.5f * delta;
      FrameUtil::SetRotationY(data.pFrame, change);
      data.m_fCurRotation += change;
    }
  });
}

void TurboMeter::Initialize() {
  ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame) {
    std::string name = GetFrameNodeName(pFrame);
    if (name.starts_with("x_tm")) {
      VehData &data = vehData.Get(pVeh);
      auto &jsonData = DataMgr::Get(pVeh->m_nModelIndex);
      if (jsonData.contains("TurboMeter")) {
        if (jsonData["TurboMeter"].contains("MaxTurbo")) {
          data.m_nMaxTurbo =
              jsonData["TurboMeter"].value("MaxTurbo", data.m_nMaxTurbo);
        }
        if (jsonData["TurboMeter"].contains("MaxRotation")) {
          data.m_fMaxRotation =
              jsonData["TurboMeter"].value("MaxRotation", data.m_fMaxRotation);
        }
      }
      data.pFrame = pFrame;
      data.m_bInitialized = true;
    }
  });

  ModelInfoMgr::RegisterRender([](CVehicle *pVeh) {
    if (!pVeh || !pVeh->GetIsOnScreen())
      return;

    VehData &data = vehData.Get(pVeh);
    if (data.m_bInitialized && data.pFrame) {
      static float prevSpeed = 0.0f;
      float speed = Util::GetVehicleSpeedRealistic(pVeh);
      float delta = CTimer::ms_fTimeScale;
      float turbo = abs(prevSpeed - speed);

      if (pVeh->m_nCurrentGear != 0) {
        turbo += 10.0f;
      }

      float newRot = (data.m_fMaxRotation / data.m_nMaxTurbo) *
                     abs(prevSpeed - speed) * delta * 1.0f;
      newRot = plugin::Clamp(newRot, 0, data.m_fMaxRotation);

      float change = (newRot - data.m_fCurRotation) * 0.25f * delta;
      FrameUtil::SetRotationY(data.pFrame, change);
      data.m_fCurRotation += change;
    }
  });
}

void GasMeter::Initialize() {
  ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame) {
    std::string name = GetFrameNodeName(pFrame);
    if (name == "x_gm" || name == "petrolok") {
      FrameUtil::SetRotationY(pFrame, RandomNumberInRange(20.0f, 70.0f));
    }
  });
}