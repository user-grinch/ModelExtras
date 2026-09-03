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
}
