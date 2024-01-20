#pragma once
#include "plugin.h"
#include "../../interface/ifeature.hpp"
#include "../../weaponextender.h"
#include <map>

struct TextureData {
  bool m_bInit = false;
  uint m_nCurRemap = 0;
  uint m_nTotalRemaps = 0;
  std::vector<RwTexture*> m_pFrames;
};

struct WepData {
  std::string m_CurNode = "";
  CPed* m_pLastKilledEntity = nullptr;
  std::map<std::string, TextureData> m_Textures;

  WepData(CWeapon*) {}
  ~WepData() {}
};

class BloodRemapFeature : public IFeature {
  WeaponExtender<WepData> xData;
  public:
    void Initialize(RwFrame* frame, CWeapon* pWeapon);
    void Process(RwFrame* frame, CWeapon *pWeapon);
};

extern BloodRemapFeature BloodRemap;