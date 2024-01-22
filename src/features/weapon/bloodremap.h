#pragma once
#include "plugin.h"
#include "../../interface/ifeature.hpp"
#include "../../weaponExtender.h"
#include <map>

struct TextureData {
  bool m_bInit = false;
  uint m_nCurRemap = 0;
  uint m_nTotalRemaps = 0;
  std::vector<RwTexture*> m_pFrames;
};

struct FrameData {
  std::string m_CurNode = "";
  CPed* m_pLastKilledEntity = nullptr;
  std::map<std::string, TextureData> m_Textures;

  FrameData(CWeapon*) {}
  ~FrameData() {}
};

class BloodRemapFeature : public IFeature {
  WeaponExtender<FrameData> xData;
  public:
    void Initialize(RwFrame* frame, CWeapon* pWeapon);
    void Process(RwFrame* frame, CWeapon *pWeapon);
};

extern BloodRemapFeature BloodRemap;