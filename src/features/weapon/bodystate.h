#pragma once
#include <plugin.h>
#include "utils/weaponextender.h"

enum class eBodyState
{
  Slim,
  SlimPlus,
  Muscle,
  MusclePlus,
  Fat,
  FatPlus,
  MuscleFat
};

class BodyState
{
private:
  struct xData
  {
    xData(CWeapon *) {}
    eBodyState prevBodyState;
  };

  static inline WeaponExtender<xData> wepData;

public:
  static void Process(void *ptr, RwFrame *frame, eModelEntityType type);
};