#pragma once
#include <map>
#include <vector>
#include "core/base.h"

struct RemapData {
  bool bRemapsLoaded = false;
  CVehicle *pCurPtr = nullptr;
  std::map<std::string, std::vector<RwTexture *>> pTextures;
};

class Remap : public CBaseFeature
{
private: 
  static inline std::map<int, RemapData> xRemaps;
  static void LoadRemaps(CVehicle* vehicle);
  static void BeforeRender(CVehicle* vehicle);
  static void AfterRender(CVehicle* vehicle);

protected:
    void Init() override;

public:
    Remap() : CBaseFeature("TextureRemaper", "FEATURES", eFeatureMatrix::TextureRemapper) {}
};