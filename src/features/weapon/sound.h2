#pragma once
#include "plugin.h"
#include "../../interface/ifeature.hpp"
#include "../../weaponExtender.h"
#include <vector>

class WeaponSoundFeature : public IFeature {
  private:
  struct xData {
    xData(CWeapon*){}
  };

  WeaponExtender<xData> wepData;

  public:
    void Initialize();
};

extern WeaponSoundFeature WeaponSound;