#pragma once

enum eMaterialType
{
    // Order can't be changed
    // Game internally uses them
    UnknownMaterial = -1,
    HeadLightLeft,
    HeadLightRight,
    TailLightLeft,
    TailLightRight,

    // ModelExtras
    ReverseLightLeft,
    ReverseLightRight,
    BrakeLightLeft,
    BrakeLightRight,
    AllDayLight,
    DayLight,
    NightLight,
    FogLightLeft,
    FogLightRight,
    SideLightLeft,
    SideLightRight,
    STTLightLeft,
    STTLightRight,
    NABrakeLightLeft,
    NABrakeLightRight,
    SpotLight,
    StrobeLight,
    SirenLight,

    IndicatorLightLeftFront,
    IndicatorLightLeftMiddle,
    IndicatorLightLeftRear,
    IndicatorLightRightFront,
    IndicatorLightRightMiddle,
    IndicatorLightRightRear,

    EngineOnLed,
    EngineBrokenLed,
    FogLightLed,
    HighBeamLed,
    LowBeamLed,
    IndicatorLeftLed,
    IndicatorRightLed,
    SirenLed,
    BootOpenLed,
    BonnetOpenLed,
    DoorOpenLed,
    RoofOpenLed,

    TotalMaterial,
};

#define INDICATOR_LIGHTS_TYPE { \
    IndicatorLightLeftFront, \
    IndicatorLightLeftMiddle, \
    IndicatorLightLeftRear, \
    IndicatorLightRightFront, \
    IndicatorLightRightMiddle, \
    IndicatorLightRightRear \
}

#define INDICATOR_LIGHTS_TYPE_REAR { \
    IndicatorLightLeftRear, \
    IndicatorLightRightRear \
}