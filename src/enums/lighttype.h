#pragma once

enum class eLightType
{
    Directional,
    Inversed,
    NonDirectional,
    Rotator
};

inline eLightType GetLightType(std::string dir)
{
    if (dir == "directional")
    {
        return eLightType::Directional;
    }
    else if (dir == "rotator")
    {
        return eLightType::Rotator;
    }
    else if (dir == "inversed-directional")
    {
        return eLightType::Inversed;
    }
    return eLightType::NonDirectional;
}