#pragma once
#include <string>
#include "RenderWare.h"
#include "CRGBA.h"
#include "CVector.h"
#include "CVector2D.h"
#include "enums/dummypos.h"
#include "enums/lightingmode.h"
#include "enums/materialtype.h"

class CVehicle;
class RwFrame;

struct VehicleDummyConfig {
    CVehicle *pVeh = nullptr;
    RwFrame* frame = nullptr;
    CVector position;
    size_t dummyIdx = 0;
    eDummyPos dummyPos = eDummyPos::None;
    eMaterialType lightType = eMaterialType::UnknownMaterial;
    bool mirroredX = false;
    bool isParentDummy = false;
    
    struct {
        float angle = 0.0f;
        float currentAngle = 0.0f;
    } rotation;

    struct {
        CRGBA color = {255, 255, 255, 100};
        float size = 0.35f;
        eLightingMode lightingType = eLightingMode::NonDirectional;
    } corona;

    struct {
        bool render = true;
        bool rotationChecks = true;
        std::string texture;
        CRGBA color = {255, 255, 255, 100};
        CVector2D offset = {0.0f, 0.0f};
        float size = 1.0f;
        CVector position;
    } shadow;

    struct {
        bool enabled = false;
        size_t timer = 0;
        size_t delay = 1000;
    } strobe;


    static inline int gId = 0;
    int id = 0;

    VehicleDummyConfig () : id(gId++) {
        
    } 
};