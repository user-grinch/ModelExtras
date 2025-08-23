#pragma once
#include <plugin.h>
#include <vector>

class SlideDoor
{
protected:
    struct DoorConfig {
      RwFrame* frame = nullptr;
      bool reverse = false;
    };

    struct VehData {
      DoorConfig leftFront, rightFront;
      DoorConfig leftRear, rightRear;

      VehData(CVehicle* pVeh) {}
      ~VehData() {}
    };

    static inline VehicleExtendedData<VehData> xData;
    static void UpdateSlidingDoor(CVehicle* pVeh, DoorConfig& config, eDoors doorID);

public:
    static void Initialize();
};
