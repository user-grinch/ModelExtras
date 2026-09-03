#pragma once
#include <plugin.h>
#include "core/base.h"
#include <unordered_map>
#include <vector>
#include <string>

struct RemapVehData {
  int randomId = -1;

  RemapVehData(CVehicle *pVeh) {}
  ~RemapVehData() {}
};

struct RemapData {
  bool bRemapsLoaded = false;
  std::unordered_map<std::string, std::vector<RwTexture *>> pTextures;
};

class Remap : public CVehFeature<RemapVehData>
{
private: 
  static inline std::unordered_map<int, RemapData> xRemaps;
  static inline bool m_bEnabled = false;
  static void LoadRemaps(CVehicle* vehicle);

protected:
  void Init() override;
  void ReloadConfig() override;
  void Reload(CVehicle *pVeh) override { ReloadConfig(); }

public:
  Remap() : CVehFeature<RemapVehData>("TextureRemaper", "FEATURES", eFeatureMatrix::TextureRemapper) {}
  static void ProcessTextures(CVehicle *pVeh, RpMaterial *pMat);
};