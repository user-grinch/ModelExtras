#pragma once
#include "plugin.h"
#include "../../interface/ifeature.hpp"
#include "../../extender.h"
#include <map>

class RandomRemapFeature : public IFeature {
private: 
  struct RemapData {
    bool m_bRemapsLoaded = false;
    std::map<std::string, std::vector<RwTexture*>> m_pTextures;
    void* curPtr = nullptr;
    RemapData(int) {}
    ~RemapData() {}
  };
  Extender<int, RemapData> xRemaps;

private:
  void LoadRemaps(CBaseModelInfo *pModelInfo, int model, eNodeEntityType type);

  void BeforeRender(void* ptr, eNodeEntityType type);
  void AfterRender(void* ptr, eNodeEntityType type);

public:
  RandomRemapFeature () {};
  
  void Initialize();
  
};

extern RandomRemapFeature RandomRemap;