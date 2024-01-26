#pragma once
#include "plugin.h"
#include "../../interface/ifeature.hpp"
#include <vector>
#include <map>

class RandomizerFeature : public IFeature {
private:
  std::map<int, std::vector<RwFrame*>> m_pStoredFrames;
  std::map<void*, std::map<std::string, int>> m_pStoredRandoms;
  
public:
  void Initialize();
  void Process(RwFrame* frame, void* ptr, eNodeEntityType type);
};

extern RandomizerFeature Randomizer;