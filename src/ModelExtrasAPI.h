/*
 * API provided for ModelExtras v3.0 Release
 */

#pragma once
#define ME_API_VERSION 30001

#ifdef MODELEXTRAS_DEV
#define ME_WRAPPER __declspec(dllexport)
#else
#define ME_WRAPPER __declspec(dllimport)
#endif

struct ME_Color
{
    unsigned char r, g, b, a;
};

struct ME_ExhaustInfo
{
    RwFrame *pFrame;
    ME_Color Color;
    float fSpeedMul;
    float fLifeTime;
    float fSizeMul;
    bool bNitroEffect;
};

struct ME_LightDummyInfo
{
    RwFrame *pFrame;
    CVector pos;
    ME_Color color;
    float fSize;
    float fAngle;
    int lightingType;
};

enum ME_FeatureID
{
    ME_TextureRemapper,
    ME_REMOVED_NULL,
    ME_AnimatedBrakes,
    ME_AnimatedClutch,
    ME_AnimatedGearLever,
    ME_RotatingHandleBar,
    ME_AnimatedChain,
    ME_AnimatedDoors,
    ME_AnimatedGasMeter,
    ME_AnimatedGearMeter,
    ME_AnimatedOdoMeter,
    ME_AnimatedRpmMeter,
    ME_AnimatedSpeedMeter,
    ME_AnimatedSpoiler,
    ME_AnimatedTurboMeter,
    ME_BackfireEffect,
    ME_DirtFX,
    ME_HDLicensePlate,
    ME_IVFCarcols,
    ME_RotatingSteeringWheel,
    ME_RotatingWheelHubs,
    ME_StandardLights,
    ME_SirenLights,
    ME_SoundEffects,
    ME_SpotLights,
    ME_BodyStateVariation,
    ME_CustomSounds,
    ME_GangHands,
    ME_PedCols,
    ME_ExhaustFx,
    ME_ConvertibleRoof,
    ME_ConvertableRoof = ME_ConvertibleRoof,
    ME_DashboardLED,
    ME_DashboardLEDs = ME_DashboardLED,
    ME_RollbackBed,
    ME_Clock,
    ME_ExtraWheels,
    ME_FeatureCount
};

enum ME_LightID
{
    ME_HeadLightLeft,
    ME_HeadLightRight,
    ME_TailLightLeft,
    ME_TailLightRight,
    ME_ReverseLightLeft,
    ME_ReverseLightRight,
    ME_BrakeLightLeft,
    ME_BrakeLightRight,
    ME_AllDayLight,
    ME_DayLight,
    ME_NightLight,
    ME_FogLightLeft,
    ME_FogLightRight,
    ME_SideLightLeft,
    ME_SideLightRight,
    ME_STTLightLeft,
    ME_STTLightRight,
    ME_NABrakeLightLeft,
    ME_NABrakeLightRight,
    ME_SpotLight,
    ME_StrobeLight,
    ME_SirenLight,
    ME_IndicatorLightLeftFront,
    ME_IndicatorLightLeftMiddle,
    ME_IndicatorLightLeftRear,
    ME_IndicatorLightRightFront,
    ME_IndicatorLightRightMiddle,
    ME_IndicatorLightRightRear,
    ME_EngineOnLed,
    ME_EngineBrokenLed,
    ME_FogLightLed,
    ME_HighBeamLed,
    ME_LowBeamLed,
    ME_IndicatorLeftLed,
    ME_IndicatorRightLed,
    ME_SirenLed,
    ME_BootOpenLed,
    ME_BonnetOpenLed,
    ME_DoorOpenLed,
    ME_RoofOpenLed,
    ME_LightCount
};

enum ME_IndicatorState
{
    ME_IndicatorOff = 0,
    ME_IndicatorLeft = 1,
    ME_IndicatorRight = 2,
    ME_IndicatorBoth = 3
};

enum ME_LightOverride
{
    ME_LightOverrideNone = 0,
    ME_LightOverrideForceOff = 1,
    ME_LightOverrideForceOn = 2
};

#ifdef __cplusplus
extern "C"
{
#endif

    // Core & Lifecycle
    ME_WRAPPER int  ME_GetAPIVersion();
    ME_WRAPPER int  ME_GetVersion();
    ME_WRAPPER bool ME_IsFeatureAvail(ME_FeatureID featureId);
    ME_WRAPPER bool ME_IsFeatureEnabled(ME_FeatureID featureId);
    ME_WRAPPER void ME_Reload();
    ME_WRAPPER void ME_ReloadVehicle(CVehicle *pVeh);

    // Vehicle Lights
    ME_WRAPPER bool  ME_GetVehicleLightState(CVehicle *pVeh, ME_LightID lightId);
    ME_WRAPPER void  ME_SetVehicleLightState(CVehicle *pVeh, ME_LightID lightId, bool state);
    ME_WRAPPER bool  ME_IsLightAvailable(CVehicle *pVeh, ME_LightID lightId);
    ME_WRAPPER int   ME_GetIndicatorState(CVehicle *pVeh);
    ME_WRAPPER void  ME_SetIndicatorState(CVehicle *pVeh, int state);
    ME_WRAPPER int   ME_GetLightOverride(CVehicle *pVeh);
    ME_WRAPPER void  ME_SetLightOverride(CVehicle *pVeh, int overrideMode);
    ME_WRAPPER bool  ME_GetFogLights(CVehicle *pVeh);
    ME_WRAPPER void  ME_SetFogLights(CVehicle *pVeh, bool on);
    ME_WRAPPER bool  ME_GetHighBeam(CVehicle *pVeh);
    ME_WRAPPER void  ME_SetHighBeam(CVehicle *pVeh, bool on);
    ME_WRAPPER float ME_GetLightInertia(CVehicle *pVeh, ME_LightID lightId);
    ME_WRAPPER bool  ME_GetLightColor(CVehicle *pVeh, ME_LightID lightId, ME_Color *onColor, ME_Color *offColor);
    ME_WRAPPER float ME_GetLightFactor(CVehicle *pVeh, ME_LightID lightId);
    ME_WRAPPER int   ME_GetLightDummyCount(CVehicle *pVeh, ME_LightID lightId);
    ME_WRAPPER bool  ME_GetLightDummyData(CVehicle *pVeh, ME_LightID lightId, int dummyIndex, ME_LightDummyInfo *outInfo);
    ME_WRAPPER bool  ME_IsLightDamaged(CVehicle *pVeh, ME_LightID lightId, int dummyIndex);
    ME_WRAPPER bool  ME_ArePopUpHeadlightsOpen(CVehicle *pVeh);
    ME_WRAPPER bool  ME_HasPopUpHeadlights(CVehicle *pVeh);
    ME_WRAPPER bool  ME_IsLightRendered(CVehicle *pVeh, ME_LightID lightId);
    ME_WRAPPER float ME_GetRealisticSpeed(CVehicle *pVeh);

    // Sirens
    ME_WRAPPER int  ME_GetSirenStateCount(CVehicle *pVeh);
    ME_WRAPPER int  ME_GetSirenStateCountByModel(int modelIndex);
    ME_WRAPPER int  ME_GetSirenState(CVehicle *pVeh);
    ME_WRAPPER bool ME_SetSirenState(CVehicle *pVeh, int state);
    ME_WRAPPER int  ME_GetSirenSoundMode(CVehicle *pVeh);
    ME_WRAPPER bool ME_SetSirenSoundMode(CVehicle *pVeh, int soundMode);
    ME_WRAPPER bool ME_GetSirenMute(CVehicle *pVeh);
    ME_WRAPPER void ME_SetSirenMute(CVehicle *pVeh, bool mute);
    ME_WRAPPER bool ME_IsSirenActive(CVehicle *pVeh);
    ME_WRAPPER bool ME_IsSirenVehicle(CVehicle *pVeh);
    ME_WRAPPER bool ME_IsCustomSirenPlaying(CVehicle *pVeh);
    ME_WRAPPER bool ME_HasSirens(CVehicle *pVeh);
    ME_WRAPPER int  ME_GetSirenSoundState(CVehicle *pVeh);

    // License Plate
    ME_WRAPPER bool ME_GetPlateText(CVehicle *pVeh, char *outBuffer, int maxLen);
    ME_WRAPPER bool ME_SetPlateText(CVehicle *pVeh, const char *text);
    ME_WRAPPER bool ME_HasCustomPlate(CVehicle *pVeh);
    ME_WRAPPER bool ME_GetPlateColors(CVehicle *pVeh, ME_Color *onColor, ME_Color *offColor);
    ME_WRAPPER int  ME_GetPlateCity(CVehicle *pVeh);
    ME_WRAPPER void ME_SetPlateCity(CVehicle *pVeh, int cityId);
    ME_WRAPPER bool ME_IsPlateNightIlluminated(CVehicle *pVeh);

    // Carcols & Colors
    ME_WRAPPER bool ME_GetVehicleColors(CVehicle *pVeh, ME_Color *col1, ME_Color *col2, ME_Color *col3, ME_Color *col4);
    ME_WRAPPER bool ME_SetVehicleColors(CVehicle *pVeh, ME_Color col1, ME_Color col2, ME_Color col3, ME_Color col4);
    ME_WRAPPER int  ME_GetCarcolVariationCount(int modelIndex);
    ME_WRAPPER int  ME_GetCarcolVariation(CVehicle *pVeh);
    ME_WRAPPER bool ME_SetCarcolVariation(CVehicle *pVeh, int variationIndex);

    // Exhausts & Nitro
    ME_WRAPPER unsigned int   ME_GetExhaustCount(CVehicle *pVeh);
    ME_WRAPPER ME_ExhaustInfo ME_GetExhaustData(CVehicle *pVeh, int index);
    ME_WRAPPER void           ME_SetExhaustData(CVehicle *pVeh, int index, ME_ExhaustInfo &data);
    ME_WRAPPER void           ME_TriggerNitro(CVehicle *pVeh, bool enable);
    ME_WRAPPER bool           ME_IsNitroFiring(CVehicle *pVeh);

    // Backfire
    ME_WRAPPER void ME_TriggerBackfire(CVehicle *pVeh, bool bPlaySound);
    ME_WRAPPER void ME_TriggerBackfireBurst(CVehicle *pVeh, bool bPlaySound);
    ME_WRAPPER bool ME_IsBackfireActive(CVehicle *pVeh);

    // Dashboard, Gauges & Clock
    ME_WRAPPER float ME_GetVehicleMileage(CVehicle *pVeh);
    ME_WRAPPER void  ME_SetVehicleMileage(CVehicle *pVeh, float mileageKm);
    ME_WRAPPER bool  ME_HasOdometer(CVehicle *pVeh);
    ME_WRAPPER float ME_GetSpeedometerValue(CVehicle *pVeh);
    ME_WRAPPER bool  ME_HasSpeedometer(CVehicle *pVeh);
    ME_WRAPPER float ME_GetRPMValue(CVehicle *pVeh);
    ME_WRAPPER bool  ME_HasRPMGauge(CVehicle *pVeh);
    ME_WRAPPER float ME_GetTurboValue(CVehicle *pVeh);
    ME_WRAPPER bool  ME_HasTurboGauge(CVehicle *pVeh);
    ME_WRAPPER bool  ME_HasGearIndicator(CVehicle *pVeh);
    ME_WRAPPER int   ME_GetGearIndicatorValue(CVehicle *pVeh);
    ME_WRAPPER void  ME_SetGearIndicatorValue(CVehicle *pVeh, int gear);
    ME_WRAPPER bool  ME_GetDashboardLEDState(CVehicle *pVeh, ME_LightID ledId);
    ME_WRAPPER void  ME_SetDashboardLEDState(CVehicle *pVeh, ME_LightID ledId, bool state);
    ME_WRAPPER bool  ME_IsClockAvailable(CVehicle *pVeh);
    ME_WRAPPER bool  ME_IsClock12HourFormat(CVehicle *pVeh);
    ME_WRAPPER void  ME_SetClock12HourFormat(CVehicle *pVeh, bool is12Hour);

    // Dirt FX
    ME_WRAPPER float ME_GetDirtLevel(CVehicle *pVeh);
    ME_WRAPPER void  ME_SetDirtLevel(CVehicle *pVeh, float dirtLevel);

    // Remap
    ME_WRAPPER int  ME_GetRemapIndex(CVehicle *pVeh);
    ME_WRAPPER void ME_SetRemapIndex(CVehicle *pVeh, int remapIndex);
    ME_WRAPPER bool ME_HasRemaps(int modelIndex);
    ME_WRAPPER int  ME_GetRemapCount(CVehicle *pVeh);
    ME_WRAPPER int  ME_GetRemapCountByModel(int modelIndex);

    // Spoilers & Convertible Roof
    ME_WRAPPER bool  ME_IsRoofOpen(CVehicle *pVeh);
    ME_WRAPPER void  ME_SetRoofOpen(CVehicle *pVeh, bool open);
    ME_WRAPPER bool  ME_HasConvertibleRoof(CVehicle *pVeh);
    ME_WRAPPER int   ME_GetRoofAnimPhase(CVehicle *pVeh);
    ME_WRAPPER int   ME_GetSpoilerCount(CVehicle *pVeh);
    ME_WRAPPER float ME_GetSpoilerAngle(CVehicle *pVeh, int spoilerIndex);
    ME_WRAPPER void  ME_SetSpoilerAngle(CVehicle *pVeh, int spoilerIndex, float angle);

    // Doors & Animation
    ME_WRAPPER bool ME_HasSlideDoors(CVehicle *pVeh);
    ME_WRAPPER bool ME_HasRotateDoors(CVehicle *pVeh);

    // Wheels, Hubs & Chain
    ME_WRAPPER bool ME_HasExtraWheels(CVehicle *pVeh);
    ME_WRAPPER int  ME_GetExtraWheelCount(CVehicle *pVeh, int wheelPos);
    ME_WRAPPER bool ME_HasWheelHubs(CVehicle *pVeh);
    ME_WRAPPER bool ME_HasAnimatedChain(CVehicle *pVeh);
    ME_WRAPPER int  ME_GetChainFrameIndex(CVehicle *pVeh);
    ME_WRAPPER int  ME_GetChainFrameCount(CVehicle *pVeh);

    // Rollback Bed
    ME_WRAPPER bool  ME_IsRollbackBedExpanded(CVehicle *pVeh);
    ME_WRAPPER void  ME_SetRollbackBedExpanded(CVehicle *pVeh, bool expanded);
    ME_WRAPPER bool  ME_HasRollbackBed(CVehicle *pVeh);
    ME_WRAPPER float ME_GetRollbackBedTilt(CVehicle *pVeh);
    ME_WRAPPER int   ME_GetRollbackBedPistonCount(CVehicle *pVeh);
    ME_WRAPPER float ME_GetRollbackBedPistonMove(CVehicle *pVeh, int pistonIndex);

    // Spotlights
    ME_WRAPPER bool ME_IsSpotlightActive(CVehicle *pVeh);
    ME_WRAPPER void ME_SetSpotlightActive(CVehicle *pVeh, bool active);
    ME_WRAPPER bool ME_HasSpotlight(CVehicle *pVeh);

    // Ped Colors
    ME_WRAPPER int  ME_GetPedVariationCount(int modelIndex);
    ME_WRAPPER bool ME_GetPedColors(CPed *pPed, ME_Color *col1, ME_Color *col2, ME_Color *col3, ME_Color *col4);
    ME_WRAPPER bool ME_SetPedColors(CPed *pPed, ME_Color col1, ME_Color col2, ME_Color col3, ME_Color col4);

    // Audio & Sounds
    ME_WRAPPER bool  ME_PlaySound(CVehicle *pVeh, const char *soundPath, float volume);
    ME_WRAPPER float ME_GetBrakePressure(CVehicle *pVeh);
    ME_WRAPPER bool  ME_PlayFileSound(const char *soundPath, float volume);
    ME_WRAPPER void  ME_PlaySwitchSound(CVehicle *pVeh);
    ME_WRAPPER bool  ME_GetSirenAudioPath(int modeIndex, char *outBuffer, int maxLen);

    // Visual & Render Configuration
    ME_WRAPPER float ME_GetMaterialAmbientMul();
    ME_WRAPPER void  ME_SetMaterialAmbientMul(float mul);
    ME_WRAPPER float ME_GetCoronaDistanceMul();
    ME_WRAPPER float ME_GetCoronaNearClip();
    ME_WRAPPER float ME_GetLightShadowDistance();
    ME_WRAPPER float ME_GetHighBeamPointLightMul();
    ME_WRAPPER bool  ME_IsAutoIndicatorsOnSteerEnabled();
    ME_WRAPPER bool  ME_IsFoglightTiedToHeadlight();
    ME_WRAPPER bool  ME_IsPlayerIdleBrakeLightsEnabled();

    // Vehicle Physics & Auxiliary
    ME_WRAPPER float ME_GetVehiclePitch(CVehicle *pVeh);
    ME_WRAPPER bool  ME_IsVehicleDoingWheelie(CVehicle *pVeh);
    ME_WRAPPER bool  ME_IsVehicleEngineOff(CVehicle *pVeh);

    // ==========================================
    // JSONC Configuration Export API
    // ==========================================

    // Model Data & Metadata
    ME_WRAPPER bool ME_HasModelData(int modelIndex);
    ME_WRAPPER bool ME_GetModelDataPath(int modelIndex, char *outPath, int maxLen);
    ME_WRAPPER bool ME_GetModelMetadata(int modelIndex, char *outAuthor, int maxAuthorLen, char *outDesc, int maxDescLen, char *outCreationTime, int maxCreationTimeLen, int *outMinVer);

    // Vehicle Lights JSON Config
    ME_WRAPPER bool ME_GetGlobalLightInertia(int modelIndex, float *outInertia);
    ME_WRAPPER bool ME_GetLightDummyConfig(int modelIndex, const char *dummyName, ME_Color *outCoronaColor, float *outCoronaSize, int *outLightingType, ME_Color *outShadowColor, float *outShadowSize, char *outShadowTexture, int maxTextureLen, bool *outRotationChecks, float *outInertia, int *outStrobeDelay);
    ME_WRAPPER bool ME_GetLightGroupConfig(int modelIndex, ME_LightID lightId, ME_Color *outCoronaColor, float *outCoronaSize, int *outLightingType, ME_Color *outShadowColor, float *outShadowSize, char *outShadowTexture, int maxTextureLen, float *outInertia);

    // Sirens JSON Config
    ME_WRAPPER bool ME_IsSirenImVehFt(int modelIndex);
    ME_WRAPPER bool ME_GetSirenStateName(int modelIndex, int stateIndex, char *outName, int maxNameLen);
    ME_WRAPPER int  ME_GetSirenItemCount(int modelIndex, int stateIndex);
    ME_WRAPPER bool ME_GetSirenItemConfig(int modelIndex, int stateIndex, const char *sirenKey, ME_Color *outColor, float *outSize, float *outInertia, int *outStartState, char *outType, int maxTypeLen, ME_Color *outShadowColor, float *outShadowSize, char *outShadowType, int maxShadowTypeLen, float *outAngleOffset);
    ME_WRAPPER bool ME_GetSirenItemPattern(int modelIndex, int stateIndex, const char *sirenKey, int *outPatternDelays, int maxDelays, int *outDelayCount);

    // Gauges & Meters JSON Config
    ME_WRAPPER bool ME_GetSpeedometerConfig(int modelIndex, const char *nodeName, float *outMaxSpeed, float *outMaxRotation, bool *outIsKph);
    ME_WRAPPER bool ME_GetRPMConfig(int modelIndex, const char *nodeName, float *outMaxRPM, float *outMaxRotation);
    ME_WRAPPER bool ME_GetTurboConfig(int modelIndex, const char *nodeName, float *outMaxTurbo, float *outMaxRotation);
    ME_WRAPPER bool ME_GetFixedGaugeConfig(int modelIndex, const char *nodeName, float *outMinAngle, float *outMaxAngle);
    ME_WRAPPER bool ME_GetOdometerConfig(int modelIndex, const char *nodeName, bool *outIsKph);

    // Clocks JSON Config
    ME_WRAPPER bool ME_GetClockConfig(int modelIndex, const char *nodeName, bool *out12HourFormat);

    // Animated Doors JSON Config
    ME_WRAPPER bool ME_GetSlideDoorConfig(int modelIndex, const char *nodeName, float *outMovMul, float *outPopOut);
    ME_WRAPPER bool ME_GetRotateDoorConfig(int modelIndex, const char *nodeName, float *outMul, float *outPopOut);

    // Spoilers JSON Config
    ME_WRAPPER bool ME_GetSpoilerConfig(int modelIndex, const char *nodeName, float *outRotation, float *outTimeMs, float *outTriggerSpeed);

    // Convertible Roofs JSON Config
    ME_WRAPPER bool ME_GetRoofConfig(int modelIndex, const char *nodeName, float *outRotation, float *outSpeed);

    // Rollback Bed JSON Config
    ME_WRAPPER bool ME_GetRollbackBedConfig(int modelIndex, float *outBedTargetRot, float *outBedRotSpeed, float *outHyTargetRot, float *outHyRotSpeed, float *outHyTargetMove, float *outHyMoveSpeed);

    // Exhausts JSON Config
    ME_WRAPPER bool ME_GetExhaustConfig(int modelIndex, const char *nodeName, float *outLifetime, float *outSpeed, float *outSize, bool *outNitroEffect, ME_Color *outColor);

    // License Plate JSON Config
    ME_WRAPPER bool ME_GetPlateConfig(int modelIndex, ME_Color *outOnColor, ME_Color *outOffColor);

    // Carcols JSON Config
    ME_WRAPPER int  ME_GetCarcolColorCount(int modelIndex);
    ME_WRAPPER bool ME_GetCarcolColor(int modelIndex, int colorIndex, ME_Color *outColor);
    ME_WRAPPER bool ME_GetCarcolVariationData(int modelIndex, int varIndex, int *outPrimary, int *outSecondary, int *outTertiary, int *outQuaternary);

    // PedCols JSON Config
    ME_WRAPPER int  ME_GetPedColColorCount(int modelIndex);
    ME_WRAPPER bool ME_GetPedColColor(int modelIndex, int colorIndex, ME_Color *outColor);
    ME_WRAPPER bool ME_GetPedColVariationData(int modelIndex, int varIndex, int *outPrimary, int *outSecondary, int *outTertiary, int *outQuaternary);

#ifdef __cplusplus
}
#endif