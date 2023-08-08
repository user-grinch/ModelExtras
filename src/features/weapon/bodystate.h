#pragma once
#include "plugin.h"
#include "../../interface/ifeature.hpp"
#include "../../weaponextender.h"
#include <vector>

class BodyStateFeature : public IFeature {
  public:
    void Process(RwFrame* frame, CWeapon *pWeapon);
    void ProcessZen(RwFrame* frame, CWeapon *pWeapon);
};

extern BodyStateFeature BodyState;