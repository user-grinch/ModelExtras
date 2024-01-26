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

  void LoadRemaps(CBaseModelInfo *pModelInfo, int model, eNodeEntityType type);

  void BeforeRender(void* ptr, eNodeEntityType type);
  void AfterRender(void* ptr, eNodeEntityType type);

public:
  RemapFeature () {};
  
  void Initialize();
  
};

extern RemapFeature Remap;