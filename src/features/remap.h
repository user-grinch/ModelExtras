#pragma once
#include <plugin.h>
#include "core/base.h"
#include <map>
#include <vector>
#include <string>

struct RemapData {
  bool bRemapsLoaded = false;
  std::map<std::string, std::vector<RwTexture *>> pTextures;
};

class Remap : public CBaseFeature
{
private: 
  static inline std::map<int, RemapData> xRemaps;
  static inline std::map<void *, int> pRandom;
  static inline bool m_bEnabled = false;
  static void LoadRemaps(CVehicle* vehicle);

protected:
  void Init() override;

public:
  Remap() : CBaseFeature("TextureRemaper", "FEATURES", eFeatureMatrix::TextureRemapper) {}
  static void ProcessTextures(CVehicle *pVeh, RpMaterial *pMat);
};