#include "pch.h"
#include "bodystate.h"

BodyStateFeature BodyState;

float GetStatValue(unsigned short stat) {
    return plugin::CallAndReturn<float, 0x558E40, unsigned short>(stat);
}

void BodyStateFeature::Initialize(RwFrame* pFrame, CWeapon *pWeapon) {
	WepData &data = wepData.Get(pWeapon);

	RwFrame* child = pFrame->child;
	while (child)
	{
		const std::string name = GetFrameNodeName(child);

		if (name == "slim") {
			data.pSlim = child;
		} else if (name == "fat") {
			data.pFat = child;
		} else if (name == "muscle") {
			data.pMuscle = child;
		}
		child = child->next;
	}
}

void BodyStateFeature::Process(RwFrame* frame, CWeapon *pWeapon) {
	std::string name = GetFrameNodeName(frame);
	if (name.find("x_body_state") != std::string::npos) {
		WepData &data = wepData.Get(pWeapon);
		if (!data.m_bInitialized) {
			Initialize(frame, pWeapon);
			data.m_bInitialized = true;
		}
		
		Util::HideAllChilds(frame);
		if (GetStatValue(23) == 1000.0f) { // muscle
			Util::ShowAllAtomics(data.pMuscle);
		} else if (GetStatValue(21) == 1000.0f) { // fat
			Util::ShowAllAtomics(data.pFat);
		} else { // slim
			Util::ShowAllAtomics(data.pSlim);
		}
	}
}