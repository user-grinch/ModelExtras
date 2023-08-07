#include "pch.h"
#include "bodystate.h"

BodyStateFeature BodyState;

float GetStatValue(unsigned short stat) {
    return plugin::CallAndReturn<float, 0x558E40, unsigned short>(stat);
}

void BodyStateFeature::Initialize(RwFrame* pFrame, CWeapon *pWeapon) {
    WepData &data = wepData.Get(pWeapon);

    RwFrame* child = pFrame->child;
    while (child) {
        const std::string name = GetFrameNodeName(child);

        if (name == "slim") {
            data.pSlim = child;
        } else if (name == "fat") {
            data.pFat = child;
        } else if (name == "muscle") {
            data.pMuscle = child;
        } else if (name == "slim+") {
            data.pSlimp = child;
        } else if (name == "fat+") {
            data.pFatp = child;
        } else if (name == "muscle+") {
            data.pMusclep = child;
        }
        child = child->next;
    }
}

void BodyStateFeature::Process(RwFrame* frame, CWeapon *pWeapon) {
    std::string name = GetFrameNodeName(frame);
    if (name.find("x_body_state_zen") != std::string::npos) {
        WepData &data = wepData.Get(pWeapon);
        if (!data.m_bInitialized) {
            Initialize(frame, pWeapon);
            data.m_bInitialized = true;
        }

        bool isMuscle = GetStatValue(23) == 1000.0f;
        bool isFat = GetStatValue(21) == 1000.0f;
        bool isSlim = !(isMuscle && isFat);

        Util::HideAllChilds(frame);
        if (isMuscle) { // muscle
            Util::ShowAllAtomics(data.pMuscle);
        } else if (isMuscle) { // fat
            Util::ShowAllAtomics(data.pFat);
        } else { // slim
            Util::ShowAllAtomics(data.pSlim);
        }
    }
}

void BodyStateFeature::ProcessZen(RwFrame* frame, CWeapon *pWeapon) {
    std::string name = GetFrameNodeName(frame);
    if (name.find("x_body_state") != std::string::npos) {
        WepData &data = wepData.Get(pWeapon);
        if (!data.m_bInitialized) {
            Initialize(frame, pWeapon);
            data.m_bInitialized = true;
        }

        bool isMuscle = GetStatValue(23) == 1000;
        bool isFat = GetStatValue(21) == 1000;
        bool isSlim = !(isMuscle && isFat);

        CPlayerPed *pPlayer = FindPlayerPed();
        if (!pPlayer) {
            return;
        }

        bool isLarge = pPlayer->m_pPlayerData->m_pPedClothesDesc->m_anModelKeys[0] != 3139216588; //hoodyA model

        Util::HideAllChilds(frame);
        if (isFat) { // fat
            Util::ShowAllAtomics(isLarge? data.pFatp : data.pFat);
        } else if (isMuscle) { // muscle
            Util::ShowAllAtomics(isLarge? data.pMusclep : data.pMuscle);
        } else { // slim
            Util::ShowAllAtomics(isLarge? data.pSlimp : data.pSlim);
        }
    }
}