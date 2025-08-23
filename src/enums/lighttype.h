#pragma once

enum eLightType
{
    // Order can't be changed
    // Game internally uses them
    UnknownLight = -1,
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
    TotalLight,
};

#define INDICATOR_LIGHTS_TYPE { \
    IndicatorLightLeftFront, \
    IndicatorLightLeftMiddle, \
    IndicatorLightLeftRear, \
    IndicatorLightRightFront, \
    IndicatorLightRightMiddle, \
    IndicatorLightRightRear \
}