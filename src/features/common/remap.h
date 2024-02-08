#pragma once
#include "plugin.h"
#include "../../interface/ifeature.hpp"
#include "../../extender.h"
#include <map>

class RemapFeature : public IFeature {
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
  Extender<int, RemapData> xRemaps;

private:
  bool GetKilledState(CWeapon *pWeapon);

  void LoadRemaps(CBaseModelInfo *pModelInfo, int model, eModelEntityType type);

  void BeforeRender(void* ptr, eModelEntityType type);
  void AfterRender(void* ptr, eModelEntityType type);

public:
  RemapFeature () {};
  
  void Initialize();
  
};

extern RemapFeature Remap;