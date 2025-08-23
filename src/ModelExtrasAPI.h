/*
* API provided for ModelExtras v2.0 Release
*/

#pragma once
#define ME_API_VERSION 10000

#ifdef MODELEXTRAS_DEV
#define ME_WRAPPER __declspec(dllexport)
#else
#define ME_WRAPPER __declspec(dllimport)
#endif

enum ME_FeatureID {
    TextureRemapper,
    ModelRandomizer,
    AnimatedBrakes,
    AnimatedClutch,
    AnimatedGearLever,
    RotatingHandleBar,
    AnimatedChain,
    AnimatedDoors,
    AnimatedGasMeter,
    AnimatedGearMeter,
    AnimatedOdoMeter,
    AnimatedRpmMeter,
    AnimatedSpeedMeter,
    AnimatedSpoiler,
    AnimatedTurboMeter,
    BackfireEffect,
    DirtFX,
    HDLicensePlate,
    IVFCarcols,
    RotatingSteeringWheel,
    RotatingWheelHubs,
    StandardLights,
    SirenLights,
    SoundEffects,
    SpotLights,
    BodyStateVariation,
    CustomSounds,
    FeatureCount
};

enum ME_LightID {
    HeadLightLeft,
    HeadLightRight,
    TailLightLeft,
    TailLightRight,
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
    LightCount,
};

#ifdef __cplusplus
extern "C"
{
#endif

    // Core
    ME_WRAPPER int ME_GetAPIVersion();
    ME_WRAPPER int ME_GetVersion();
    ME_WRAPPER bool ME_IsFeatureAvail(ME_FeatureID featureId);

    // Vehicle
    ME_WRAPPER bool ME_GetVehicleLightState(CVehicle *pVeh, ME_LightID lightId);
    ME_WRAPPER void ME_SetVehicleLightState(CVehicle *pVeh, ME_LightID lightId, bool state);

    // Ped
    ME_WRAPPER int ME_GetPedRemap(CPed * ped, int index);
    ME_WRAPPER void ME_SetPedRemap(CPed * ped, int index, int num);
    ME_WRAPPER void ME_SetAllPedRemaps(CPed * ped, int num);
    
#ifdef __cplusplus
}
#endif