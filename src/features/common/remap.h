#pragma once
#include "plugin.h"
#include "../../interface/ifeature.hpp"
#include "../../extender.h"
#include <map>

class Remap {
private: 
  struct TextureVariant {
    RwTexture *m_pNormal, *m_pBlood;
  };

  struct RemapData {
    bool m_bRemapsLoaded = false;
    std::map<std::string, std::vector<TextureVariant>> m_pTextures;
    void* curPtr = nullptr;
    bool useBlood = false;
    RemapData(int) {}
    ~RemapData() {}
  };
  static inline Extender<int, RemapData> xRemaps;

private:
  static bool GetKilledState(CWeapon *pWeapon);

  static void LoadRemaps(CBaseModelInfo *pModelInfo, int model, eModelEntityType type);

  static void BeforeRender(void* ptr, eModelEntityType type);
  static void AfterRender(void* ptr, eModelEntityType type);

public:
  static void Initialize();
  
};