#pragma once
#include "plugin.h"
#include "../../interface/ifeature.hpp"
#include "../../weaponextender.h"

struct TextureData {
  bool m_bInit = false;
  uint m_nKills = 0;

  TextureData(CWeapon*) {}
  ~TextureData() {}
};

class BloodRemapFeature : public IFeature {
  WeaponExtender<TextureData> xData;
  public:
    void Initialize(RwFrame* frame, CWeapon* pWeapon);
    void Process(RwFrame* frame, CWeapon *pWeapon);
};

extern BloodRemapFeature BloodRemap;