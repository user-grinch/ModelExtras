#pragma once
#include "plugin.h"
#include "../../interface/ifeature.hpp"
#include "../../extender.h"

class RandomRemapFeature : public IFeature {
private: 
  struct FrameData {
    bool m_bInit = false;
    FrameData(RwFrame*) {}
    ~FrameData() {}
  };
  Extender<RwFrame*, FrameData> xFrame;

public:
  RandomRemapFeature () {};
 
  void Process(RwFrame* frame, void* ptr, eNodeEntityType type);
};

extern RandomRemapFeature RandomRemap;