#pragma once

enum class eFeatureMatrix
{
    // Common Features
    TextureRemapper,
    REMOVED_NULL,

    // Bike Features
    AnimatedBrakes,
    AnimatedClutch,
    AnimatedGearLever,
    RotatingHandleBar,

    // Vehicle Features
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

    // Weapon Features
    BodyStateVariation,
    CustomSounds,

    // Ped Features
    GangHands,
    PedCols,

    ExhaustFx,
    ConvertibleRoof,
    DashboardLED,
    RollbackBed,
    Clock,
    ExtraWheels,

    FeatureCount
};