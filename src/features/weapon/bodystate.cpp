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
    if (name.find("x_body_state") != std::string::npos) {
        bool isMuscle = GetStatValue(23) == 1000.0f;
        bool isFat = GetStatValue(21) == 1000.0f;
        bool isSlim = !(isMuscle && isFat);

        eBodyState bodyState = eBodyState::Slim;
        if (isMuscle) bodyState = eBodyState::Muscle;
        if (isFat) bodyState = eBodyState::Muscle;
        if (isSlim) bodyState = eBodyState::Slim;

        if (bodyState != data.prevBodyState) {
            Util::HideAllChilds(frame);
            if (isMuscle) { // muscle
                Util::ShowChildWithName(frame, "muscle");
            } else if (isMuscle) { // fat
                Util::ShowChildWithName(frame, "fat");
            } else { // slim
                Util::ShowChildWithName(frame, "slim");
            }
            data.prevBodyState = bodyState;

            auto play = FindPlayerPed();
            if (play->m_nWeaponModelId == PARACHUTE_MODEL) {
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

        bool isLarge = pPlayer->m_pPlayerData->m_pPedClothesDesc->m_anModelKeys[0] != 3139216588; //hoodyA model
        bool isUniform = pPlayer->m_pPlayerData->m_pPedClothesDesc->m_anTextureKeys[17] != 0; // default outfit
        bool isPlus = isLarge && !isUniform;

        eBodyState bodyState = eBodyState::Slim;
        if (isMuscle) bodyState = isPlus? eBodyState::Muscle : eBodyState::MusclePlus;
        if (isFat) bodyState = isPlus? eBodyState::Fat : eBodyState::FatPlus;
        if (isMuscle && isFat) bodyState = eBodyState::MuscleFat;
        if (isSlim) bodyState = isPlus? eBodyState::Slim : eBodyState::SlimPlus;
        
        if (bodyState != data.prevBodyState) {
            Util::HideAllChilds(frame);
            if (isFat && isMuscle) {
                Util::ShowChildWithName(frame, "fat_muscle");
            }
            
            if (isPlus) {
                if (isFat) { // fat
                    Util::ShowChildWithName(frame, "fat+");
                } else if (isMuscle) { // muscle
                    Util::ShowChildWithName(frame, "muscle+");
                } else if (isSlim) { // slim
                    Util::ShowChildWithName(frame, "slim+");
                }
            }
            data.prevBodyState = bodyState;
        }
    }
}