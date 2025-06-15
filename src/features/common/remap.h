#pragma once
#include <plugin.h>
#include "../../interface/ifeature.hpp"
#include "../../extender.h"
#include <map>

class Remap {
private: 
  struct RemapData {
    bool m_bRemapsLoaded = false;
    std::map<std::string, std::vector<RwTexture *>> m_pTextures;
    void* curPtr = nullptr;
    RemapData(int) {}
    ~RemapData() {}
  };
  static inline Extender<int, RemapData> xRemaps;

private:
  static void LoadRemaps(CBaseModelInfo *pModelInfo, int model, eModelEntityType type);

  static void BeforeRender(void* ptr, eModelEntityType type);
  static void AfterRender(void* ptr, eModelEntityType type);

public:
  static void Initialize();
  
};