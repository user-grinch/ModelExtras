#include "pch.h"
#include "bodystate.h"
#define PARACHUTE_MODEL 371

float GetStatValue(unsigned short stat) {
    return plugin::CallAndReturn<float, 0x558E40, unsigned short>(stat);
}

void BodyState::Process(void* ptr, RwFrame* frame, eModelEntityType type) {
    CWeapon *pWeapon = static_cast<CWeapon*>(ptr);
    xData &data = wepData.Get(pWeapon);
    std::string name = GetFrameNodeName(frame);
    if (NODE_FOUND(name, "x_body_state")) {
        bool isMuscle = GetStatValue(23) == 1000;
        bool isFat = GetStatValue(21) == 1000;
        bool isSlim = !(isMuscle && isFat);
        CPlayerPed *pPlayer = FindPlayerPed();
        if (!pPlayer) {
            return;
        }

        bool isPlus = false;
        if (NODE_FOUND(name, "_zen")) {
            bool isLarge = pPlayer->m_pPlayerData->m_pPedClothesDesc->m_anModelKeys[0] != 3139216588; // hoodyA model
            bool isUniform = pPlayer->m_pPlayerData->m_pPedClothesDesc->m_anTextureKeys[17] != 0; // default outfit
            isPlus = isLarge && !isUniform;
        }

        eBodyState bodyState = eBodyState::Slim;
        if (isMuscle && isFat) bodyState = eBodyState::MuscleFat;
        else if (isMuscle) bodyState = (isPlus? eBodyState::MusclePlus : eBodyState::Muscle);
        else if (isFat) bodyState = (isPlus? eBodyState::FatPlus : eBodyState::Fat);
        else if (isSlim) bodyState = (isPlus? eBodyState::SlimPlus : eBodyState::Slim);
        
         if (bodyState != data.prevBodyState) {
            Util::HideAllChilds(frame);
            if (isFat && isMuscle) {
                Util::ShowChildWithName(frame, "muscle_fat");
            }
            else if (isFat) { 
                Util::ShowChildWithName(frame, isPlus? "fat+" : "fat");
            } else if (isMuscle) {
                Util::ShowChildWithName(frame, isPlus? "muscle+" : "muscle");
            } else if (isSlim) { 
                Util::ShowChildWithName(frame, isPlus? "slim+" : "slim");
            }
            auto play = FindPlayerPed();
            if (play && play->m_nWeaponModelId == PARACHUTE_MODEL) {
                plugin::Call<0x4395B0>();
            }
            data.prevBodyState = bodyState;
        }
    }
}