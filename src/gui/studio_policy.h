#pragma once
#include <string_view>
#include <initializer_list>

// Only audited reversible settings are live. Unknown future settings defer to restart.
namespace StudioPolicy {
template <typename Ini, typename Settings> void Restore(Ini &ini, const Settings &boot) {
    for (const auto &[key, value] : boot) ini.set(key.first, key.second, value);
}
inline const char *RestartReason(std::string_view section, std::string_view key) {
    if (key == "HDLicensePlate") return "Plate textures and permanent game hooks are installed at startup.";
    if (key == "StandardLights") return "Vanilla lighting patches and light components cannot be undone by reloading.";
    if (key == "DirtFX") return "The dirt renderer patches vanilla and SilentPatch code without an original-function fallback.";
    if (key == "DigitalClock") return "Clock setup clones digits and destroys the original node hierarchy.";
    if (key == "SirenLights") return "Siren hooks change vehicle audio capabilities without restoring them on disable.";
    if (section == "FEATURES") {
        for (auto live : {"AnimatedGasMeter", "Carcols", "DashboardLED", "ExhaustFx", "Neon", "TextureRemapper", "PedCols", "BackfireEffect", "BackfireEffect_OnlySelectedModels"})
            if (key == live) return nullptr;
        for (auto animated : {"AnimatedChain", "AnimatedDoors", "AnimatedGearMeter", "AnimatedOdoMeter", "AnimatedRpmMeter",
            "AnimatedSpeedMeter", "AnimatedSpoiler", "AnimatedTurboMeter", "ConvertibleRoof", "RollbackBed", "ExtraWheels", "RotatingWheelHubs"})
            if (key == animated) return nullptr;
    }
    if (section == "CONFIG") {
        if (key == "VerboseLogging" || key == "EnableLiveReload" || key == "ModelVersionCheck") return nullptr;
        if (key == "ModLoaderData") return "Changing config source priority requires reinitializing model and ped configuration ownership.";
        if (key == "EnableLogging") return "The log file and logging sinks are created during startup.";
    }
    if (section == "LIGHTS") {
        if (key == "SpotLights") return nullptr;
        for (auto live : {"LightCoronas", "LightShadows", "PointLights", "HeadLightBeams", "SirenPointLights", "AutoIndicatorsOnSteer",
            "StandardLights_GlobalIndicatorLights", "FoglightTiedToHeadlight", "PlayerIdleBrakeLights", "LightsRequireEngine", "SirensRequireEngine",
            "HeadLightCoronaDistanceMul", "TailLightCoronaDistanceMul", "CoronaDistanceMul", "CoronaNearClip", "LightShadowDistance",
            "HighBeamPointLightMul", "PointLightIntensity", "SirenPointLightMul", "LightHeightLimit", "MaterialAmbientMul", "LightCoronaSize",
            "LightCoronaIntensity", "LightShadowIntensity", "HeadLightCoronaSize", "HeadLightCoronaIntensity", "HeadLightShadowSize",
            "HeadLightShadowIntensity", "TailLightCoronaSize", "TailLightCoronaIntensity", "TailLightShadowSize", "TailLightShadowIntensity"})
            if (key == live) return nullptr;
    }
    if (section == "SOUND") {
        for (auto live : {"SoundEffects", "SoundMult", "GlobalAirbreakSound", "GlobalEngineSound", "GlobalIndicatorSound", "GlobalReverseSound",
            "BrakePadSound", "BrakePadOnlySelected", "DoorChimeSound", "DoorChimeOnlySelected", "NonPlayerVehicles"})
            if (key == live) return nullptr;
    }
    if (section == "KEYS") {
        for (auto live : {"FogLightKey", "IndicatorLightBothKey", "IndicatorLightLeftKey", "IndicatorLightRightKey", "IndicatorLightNoneKey",
            "LongLightKey", "RollbackBedToggleKey", "RoofToggleKey", "SirenLightKey", "SpotLightKey"})
            if (key == live) return nullptr;
    }
    if (section == "TABLE") {
        for (auto live : {"BigVehicleModels", "DoorChime_VehicleModels", "BrakePad_VehicleModels", "BackFireEffect_VehicleModels"})
            if (key == live) return nullptr;
    }
    return "This setting has no audited reversible live update path.";
}
}
