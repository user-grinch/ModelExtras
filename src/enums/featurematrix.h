#pragma once

enum class eFeatureMatrix {
  // Common Features
  TextureRemapper,
  ModelRandomizer,

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
  FeatureCount
};