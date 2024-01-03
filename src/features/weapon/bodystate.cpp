#include "pch.h"
#include "bodystate.h"
#define PARACHUTE_MODEL 371

BodyStateFeature BodyState;

float GetStatValue(unsigned short stat) {
    return plugin::CallAndReturn<float, 0x558E40, unsigned short>(stat);
}

void BodyStateFeature::Process(RwFrame* frame, CWeapon *pWeapon) {
    xData &data = wepData.Get(pWeapon);
    std::string name = GetFrameNodeName(frame);
    if (name.find("x_body_state") != std::string::npos && name.find("x_body_state_zen") == std::string::npos) {
        bool isMuscle = GetStatValue(23) == 1000.0f;
        bool isFat = GetStatValue(21) == 1000.0f;
        bool isSlim = !(isMuscle && isFat);

        eBodyState bodyState = eBodyState::Slim;
        if (isMuscle) bodyState = eBodyState::Muscle;
        else if (isFat) bodyState = eBodyState::Fat;

        if (bodyState != data.prevBodyState) {
            Util::HideAllChilds(frame);
            if (isMuscle) { 
                Util::ShowChildWithName(frame, "muscle");
            } else if (isMuscle) {
                Util::ShowChildWithName(frame, "fat");
            } else { 
                Util::ShowChildWithName(frame, "slim");
            }
            data.prevBodyState = bodyState;

            auto play = FindPlayerPed();
            if (play && play->m_nWeaponModelId == PARACHUTE_MODEL) {
                plugin::Call<0x4395B0>();
            }
        }
    }
}

void BodyStateFeature::ProcessZen(RwFrame* frame, CWeapon *pWeapon) {
    xData &data = wepData.Get(pWeapon);
    std::string name = GetFrameNodeName(frame);
    if (name.find("x_body_state_zen") != std::string::npos) {
        bool isMuscle = GetStatValue(23) == 1000;
        bool isFat = GetStatValue(21) == 1000;
        bool isSlim = !(isMuscle && isFat);
        CPlayerPed *pPlayer = FindPlayerPed();
        if (!pPlayer) {
            return;
        }

        bool isLarge = pPlayer->m_pPlayerData->m_pPedClothesDesc->m_anModelKeys[0] != 3139216588; // hoodyA model
        bool isUniform = pPlayer->m_pPlayerData->m_pPedClothesDesc->m_anTextureKeys[17] != 0; // default outfit
        bool isPlus = isLarge && !isUniform;

        eBodyState bodyState = eBodyState::Slim;
        if (isMuscle && isFat) bodyState = eBodyState::MuscleFat;
        else if (isMuscle) bodyState = (isPlus? eBodyState::MusclePlus : eBodyState::Muscle);
        else if (isFat) bodyState = (isPlus? eBodyState::FatPlus : eBodyState::Fat);
        else if (isSlim) bodyState = (isPlus? eBodyState::SlimPlus : eBodyState::Slim);
        
         if (bodyState != data.prevBodyState) {
            Util::HideAllChilds(frame);
            if (isFat && isMuscle) {
                Util::ShowChildWithName(frame, "fat_muscle");
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