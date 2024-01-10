#pragma once
#include "plugin.h"
#include "../../interface/ifeature.hpp"
#include <vector>

class RandomizerFeature : public IFeature {
  protected:
  std::vector<RwFrame*> frameStore;    

  public:
    void Initialize(RwFrame* frame);
    void Process(RwFrame* frame, void* ptr, eNodeEntityType type);
};

extern RandomizerFeature Randomizer;