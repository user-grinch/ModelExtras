#pragma once
#include "plugin.h"
#include "../../interface/ifeature.hpp"
#include "../../weaponextender.h"

enum class eBloodOverlay {
  None,
  Low,
  Mid,
  High,
};

struct TextureData {
  eBloodOverlay m_nCurrent = eBloodOverlay::None;
  uint m_nKills = 0;

  TextureData(CWeapon*) {}
  ~TextureData() {}
};

class BloodRemapFeature : public IFeature {
  WeaponExtender<TextureData> texData;
  public:
    void Process(RwFrame* frame, CWeapon *pWeapon);
};

extern BloodRemapFeature BloodRemap;