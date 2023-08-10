#include "pch.h"
#include "randomizer.h"

RandomizerFeature Randomizer;

void RandomizerFeature::Initialize(RwFrame* pFrame) {
    uint32_t total = Util::GetChildCount(pFrame);
    uint32_t frameNum = Random(1, total);
    std::vector<RwFrame*> childVec;
    Util::StoreChilds(pFrame, childVec);
    Util::ShowAllChilds(pFrame);
    for (size_t i = 0; i < childVec.size(); ++i) {
        if (i != frameNum) {
            Util::HideAllAtomics(childVec[i]);
        }
    }
}

void RandomizerFeature::Process(RwFrame* frame, void* ptr, eNodeEntityType type) {
    std::string name = GetFrameNodeName(frame);
    if (name.find("x_randomizer") != std::string::npos) {
        
        bool *pRandomized;
        if (type == eNodeEntityType::Vehicle) {
            VehData &data = vehData.Get(static_cast<CVehicle*>(ptr));
            pRandomized = &data.m_bInitialized;
        } else if (type == eNodeEntityType::Weapon) {
            WepData &data = wepData.Get(static_cast<CWeapon*>(ptr));
            pRandomized = &data.m_bInitialized;
        }

        if (!*pRandomized) {
            Initialize(frame);
            *pRandomized = true;
        }
    }
}