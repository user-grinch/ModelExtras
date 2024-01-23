#include "pch.h"
#include "randomizer.h"
#define NODE_ID "x_randomizer"

RandomizerFeature Randomizer;

void RandomizerFeature::Initialize(RwFrame* pFrame) {
    uint32_t total = Util::GetChildCount(pFrame);
    uint32_t frameNum = Random(0, total-1);
    std::vector<RwFrame*> childVec;
    Util::StoreChilds(pFrame, childVec);
    for (size_t i = 0; i < childVec.size(); ++i) {
        if (i != frameNum) {
            Util::HideAllAtomics(childVec[i]);
        }
    }
}

void RandomizerFeature::Process(RwFrame* frame, void* ptr, eNodeEntityType type) {
    std::string name = GetFrameNodeName(frame);
    if (NODE_FOUND(name, NODE_ID)) {
        if (std::find(frameStore.begin(), frameStore.end(), frame) == frameStore.end()) {
            Initialize(frame);
            frameStore.push_back(frame);
        }
    }
}