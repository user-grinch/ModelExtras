#pragma once
#include "core/base.h"

struct ChainData
{
  RwFrame *m_pRootFrame = nullptr;
  std::vector<RwFrame *> m_FrameList;
  short m_nCurChain = 0;

  ChainData(CVehicle *pVeh) {}
  ~ChainData() {}
};

class ChainFeature : public CVehFeature<ChainData>
{
protected:
    void Init() override;

public:
    ChainFeature() : CVehFeature<ChainData>("AnimatedChain", "FEATURES", eFeatureMatrix::AnimatedChain) {}
};