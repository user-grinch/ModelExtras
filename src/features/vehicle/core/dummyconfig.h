#pragma once
#include <string>
#include "RenderWare.h"
#include "CRGBA.h"
#include "CVector.h"
#include "CVector2D.h"
#include "enums/dummypos.h"
#include "enums/lightingmode.h"
#include "enums/lighttype.h"

class CVehicle;
class RwFrame;

struct VehicleDummyConfig {
    CVehicle *pVeh = nullptr;
    RwFrame* frame = nullptr;
    CVector position;
    size_t dummyIdx = 0;
    eDummyPos dummyType = eDummyPos::None;
    eLightType lightType = eLightType::UnknownLight;
    bool mirroredX = false;
    float angleOffset = 0.0f;

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
        struct {
            CRGBA color = {255, 255, 255, 100};
        } on;
        struct {
            CRGBA color = {255, 255, 255, 100};
        } off;
    } material;

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
};