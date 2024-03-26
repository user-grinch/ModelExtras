#pragma once
#include "plugin.h"
#include "../../interface/ifeature.hpp"
#include "../../weaponExtender.h"
#include <vector>

enum class eBodyState {
  Slim,
  SlimPlus,
  Muscle,
  MusclePlus,
  Fat,
  FatPlus,
  MuscleFat
};

class BodyStateFeature : public IFeature {
  private:
  struct xData {
    xData(CWeapon*){}
    eBodyState prevBodyState;
  };

  WeaponExtender<xData> wepData;

  public:
    void Process(RwFrame* frame, CWeapon *pWeapon, std::string &name);
    void ProcessZen(RwFrame* frame, CWeapon *pWeapon, std::string &name);
};

extern BodyStateFeature BodyState;