#pragma once

enum class LightType
{
    Directional,
    NonDirectional,
    Inversed,
    Rotator
};

inline LightType GetLightType(std::string dir)
{
    if (dir == "directional")
    {
        return LightType::Directional;
    }
    else if (dir == "rotator")
    {
        return LightType::Rotator;
    }
    else if (dir == "inversed-directional")
    {
        return LightType::Inversed;
    }
    return LightType::NonDirectional;
}