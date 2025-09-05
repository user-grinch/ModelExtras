#pragma once
#include "RenderWare.h"
#include "dummyconfig.h"
#include "enums/dummypos.h"
#include "enums/lightingmode.h"
#include "game_sa/CGeneral.h"
#include "modelinfomgr.h"

class VehicleDummy {
private:
  VehicleDummyConfig data;

public:
  VehicleDummy(const VehicleDummyConfig &config);

  const VehicleDummyConfig &GetRef() { return data; }

  VehicleDummyConfig &Get() { return data; }

  // Rotators
  void ResetAngle() {
    if (data.rotation.currentAngle != 0.0f) {
      ReduceAngle(data.rotation.currentAngle);
    }
  };

  void AddAngle(float angle) {
    if (angle != 0.0f) {
      RwFrameRotate(data.frame, (RwV3d *)0x008D2E18, angle, rwCOMBINEPRECONCAT);
      data.rotation.currentAngle += angle;
    }
  };

  void ReduceAngle(float angle) {
    if (angle != 0.0f) {
      RwFrameRotate(data.frame, (RwV3d *)0x008D2E18, -angle,
                    rwCOMBINEPRECONCAT);
      data.rotation.currentAngle -= angle;
    }
  };

  void SetAngle(float angle) {
    ResetAngle();
    AddAngle(angle);
  }

  void Update();
};
