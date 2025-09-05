#include "randomizer.h"
#include "meevents.h"
#include "pch.h"

void Randomizer::Initialize() {
  MEEvents::weaponRemoveEvent.before += [](CPed *pPed, int model) {
    CWeapon *pWeapon = &pPed->m_aWeapons[pPed->m_nSelectedWepSlot];
    if (pWeapon) {
      if (m_pStoredRandoms.contains(pWeapon)) {
        m_pStoredRandoms.erase(m_pStoredRandoms.find(pWeapon));
      }
    }
  };
}

void Randomizer::Process(void *ptr, RwFrame *frame, eModelEntityType type) {
  std::string name = GetFrameNodeName(frame);
  int model = Util::GetEntityModel(ptr, type);
  if (m_pStoredFrames.find(model) == m_pStoredFrames.end()) {
    FrameUtil::StoreChilds(frame, m_pStoredFrames[model]);
  }

  if (m_pStoredRandoms.find(ptr) == m_pStoredRandoms.end() ||
      m_pStoredRandoms[ptr].find(name) == m_pStoredRandoms[ptr].end()) {
    m_pStoredRandoms[ptr][name] =
        RandomNumberInRange((uint32_t)0, FrameUtil::GetChildCount(frame) - 1);
  }

  FrameUtil::ShowAllChilds(frame);
  for (size_t i = 0; i < m_pStoredFrames[model].size(); ++i) {
    if (i != m_pStoredRandoms[ptr][name]) {
      FrameUtil::HideAllAtomics(m_pStoredFrames[model][i]);
    }
  }
}