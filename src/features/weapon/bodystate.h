#pragma once
#include "plugin.h"
#include "../../interface/ifeature.hpp"
#include "../../weaponextender.h"
#include <vector>

class BodyStateFeature : public IFeature {
  protected:
    struct WepData {
        bool m_bInitialized = false;
        RwFrame *pSlim = nullptr;
        RwFrame *pFat = nullptr;
        RwFrame *pMuscle = nullptr;

        // for zen version
        RwFrame *pSlimp = nullptr;
        RwFrame *pFatp = nullptr;
        RwFrame *pMusclep = nullptr;
        WepData(CWeapon *pWeapon) {}
        ~WepData() {}
    };

    WeaponExtender<WepData> wepData;

  public:
    void Initialize(RwFrame* frame, CWeapon *pWeapon);
    void Process(RwFrame* frame, CWeapon *pWeapon);
    void ProcessZen(RwFrame* frame, CWeapon *pWeapon);
};

extern BodyStateFeature BodyState;