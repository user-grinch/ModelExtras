#pragma once
#include "plugin.h"
#include "../../interface/ifeature.hpp"
#include "../../weaponextender.h"

class RandomizerFeature : public IFeature {
  protected:
    struct VehData {
        bool m_bInitialized = false;

        VehData(CVehicle *pVeh) {}
        ~VehData() {}
    };

    struct WepData {
        bool m_bInitialized = false;

        WepData(CWeapon *pWeapon) {}
        ~WepData() {}
    };

    VehicleExtendedData<VehData> vehData;
    WeaponExtender<WepData> wepData;

  public:
    void Initialize(RwFrame* frame);
    void Process(RwFrame* frame, void* ptr, eNodeEntityType type);
};

extern RandomizerFeature Randomizer;