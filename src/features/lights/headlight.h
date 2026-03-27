#pragma  once
#include "base.h"
#include "core/dummyconfig.h"
#include "lights/config.h"

class HeadLightData : public LightsCommonData {
public:
    bool bLongLights = false;

    HeadLightData(CVehicle *pVeh) : LightsCommonData(pVeh) {
        config.shadow.texture = "headlight";
    }
};

class HeadlightBehavior : public ILightBehavior<HeadLightData> {
public:
    HeadlightBehavior() {

    }

    void RenderHeadlights(CVehicle *pControlVeh, bool isLeftOn, bool isRightOn);

    bool IsValidDummy(RwFrame *pFrame) override;
    bool IsValidMaterial(RpMaterial *pMat) override;
    bool RegisterDummy(CVehicle *pVeh, RwFrame *pFrame) override;
    bool RegisterMat(CVehicle *pVeh, RpMaterial *pMat) override;

    std::vector<eMaterialType> GetSupportedMatTypes() override;
    eMaterialType GetMatType(RpMaterial *pMat) override;


    void Process(CVehicle *pVeh) override;
    void Render(CVehicle *pControlVeh, CVehicle *pTowedVeh) override;
};