#include "pch.h"
#include "bodystate.h"

BodyStateFeature BodyState;

float GetStatValue(unsigned short stat) {
    return plugin::CallAndReturn<float, 0x558E40, unsigned short>(stat);
}

void BodyStateFeature::Process(RwFrame* frame, CWeapon *pWeapon) {
    std::string name = GetFrameNodeName(frame);
    if (name.find("x_body_state") != std::string::npos) {
        bool isMuscle = GetStatValue(23) == 1000.0f;
        bool isFat = GetStatValue(21) == 1000.0f;
        bool isSlim = !(isMuscle && isFat);

        Util::HideAllChilds(frame);
        if (isMuscle) { // muscle
            Util::ShowChildWithName(frame, "muscle");
        } else if (isMuscle) { // fat
            Util::ShowChildWithName(frame, "fat");
        } else { // slim
            Util::ShowChildWithName(frame, "slim");
        }
    }
}

void BodyStateFeature::ProcessZen(RwFrame* frame, CWeapon *pWeapon) {
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
        
        Util::HideAllChilds(frame);
        if (isFat && isMuscle) {
            Util::ShowChildWithName(frame, "fat_muscle");
        } else if (isFat) { // fat
            Util::ShowChildWithName(frame, isPlus? "fat+" : "fat");
        } else if (isMuscle) { // muscle
            Util::ShowChildWithName(frame, isPlus? "muscle+" : "muscle");
        } else { // slim
            Util::ShowChildWithName(frame, isPlus? "slim+" : "slim");
        }
    }
}