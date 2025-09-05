#pragma once
#include <plugin.h>
#include <vector>

class RotateDoor {
protected:
  struct DoorConfig {
    RwFrame *frame = nullptr;
    float originalRot = 0.0f;
    bool reverse = false;
  };

  struct VehData {
    DoorConfig leftFront, rightFront;
    DoorConfig leftRear, rightRear;

    VehData(CVehicle *pVeh) {}
    ~VehData() {}
  };

  static inline VehicleExtendedData<VehData> xData;
  static void UpdateRotatingDoor(CVehicle *pVeh, DoorConfig &config,
                                 eDoors doorID);

public:
  static void Initialize();
};
