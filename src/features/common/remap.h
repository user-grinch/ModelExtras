#pragma once
#include <plugin.h>
#include <map>
#include <vector>

struct TextureVariant {
  RwTexture *pNormal, *pBlood;
};

struct RemapData {
  bool bRemapsLoaded = false;
  bool bUseBlood = false;
  void* pCurPtr = nullptr;
  std::map<std::string, std::vector<TextureVariant>> pTextures;
};

class Remap {
private: 
  static inline std::map<int, RemapData> xRemaps;
  static bool GetKilledState(CWeapon *pWeapon);
  static void LoadRemaps(CBaseModelInfo *pModelInfo, int model, eModelEntityType type);
  static void BeforeRender(void* ptr, eModelEntityType type);
  static void AfterRender(void* ptr, eModelEntityType type);

public:
  static void Initialize();
  
};