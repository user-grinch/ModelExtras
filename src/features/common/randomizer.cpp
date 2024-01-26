#include "pch.h"
#include "randomizer.h"
#define NODE_ID "x_randomizer"

RandomizerFeature Randomizer;

void RandomizerFeature::Initialize() {
    weaponRemoveEvent.before += [this](CPed* pPed, int model) {
        CWeapon *pWeapon = &pPed->m_aWeapons[pPed->m_nActiveWeaponSlot];
        if (pWeapon) {
            if(m_pStoredRandoms.contains(pWeapon)) {
                m_pStoredRandoms.erase(m_pStoredRandoms.find(pWeapon));
            }
        }
    };
}

void RandomizerFeature::Process(RwFrame* frame, void* ptr, eNodeEntityType type) {
    std::string name = GetFrameNodeName(frame);

    if (NODE_FOUND(name, NODE_ID)) {
        int model = Util::GetEntityModel(ptr, type);
        if (m_pStoredFrames.find(model) == m_pStoredFrames.end()) {
            Util::StoreChilds(frame, m_pStoredFrames[model]);
        }

        if (m_pStoredRandoms.find(ptr) == m_pStoredRandoms.end() || m_pStoredRandoms[ptr].find(name) == m_pStoredRandoms[ptr].end()) {
            m_pStoredRandoms[ptr][name] = Random(0, Util::GetChildCount(frame)-1);
        }

        Util::ShowAllChilds(frame);
        for (size_t i = 0; i < m_pStoredFrames[model].size(); ++i) {
            if (i != m_pStoredRandoms[ptr][name]) {
                Util::HideAllAtomics(m_pStoredFrames[model][i]);
            }
        }
    }
}