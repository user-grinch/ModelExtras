#pragma once
#include "plugin.h"
#include "../../weaponExtender.h"
#include <map>

class BloodRemap {
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

  static inline WeaponExtender<FrameData> xData;
  static void Initialize(RwFrame* frame, CWeapon* pWeapon);

  public:
    static void Process(void* ptr, RwFrame* frame, eModelEntityType type);
};