#pragma once
#include <string>

class CVehicle;

namespace SAMP
{
    // Returns true if a supported SA-MP version (0.3.7-R1, R3, R5, 0.3.DL) is loaded
    bool IsPresent();

    // Returns true if the SA-MP chat box, input box, or dialog is currently open/focused
    bool IsInputActive();

    // Retrieves and formats the custom license plate text for the given vehicle.
    // Returns empty string if not in SA-MP, vehicle not found in pool, or default plate ("XYZSR998").
    std::string GetVehiclePlateText(CVehicle *pGameVeh);

    // Applies SA-MP vehicle light patches to GTA SA if SA-MP is loaded.
    // On SA-MP servers that do not call ManualVehicleEngineAndLights(),
    // SA-MP skips NOPing GTA SA's CVehicle::DoVehicleLights daytime reset logic.
    // This causes GTA SA to continuously clear bLightsOn every frame in daytime,
    // conflicting with SA-MP sync packets and producing rapid headlight flickering
    // as well as preventing the local player's /lights command from working.
    void PatchVehicleLights();
}
