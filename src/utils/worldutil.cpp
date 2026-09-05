#include "pch.h"
#include "worldutil.h"
#include <CClock.h>
#include <CWeather.h>

bool WorldUtil::IsNightTime()
{
    return CClock::GetIsTimeInRange(20, 6);
}

bool WorldUtil::IsFoggy()
{
    return (CWeather::Foggyness > 0.1f) || (CWeather::Rain > 0.3f) || (CWeather::NewWeatherType == WEATHER_FOGGY_SF || CWeather::NewWeatherType == WEATHER_SANDSTORM_DESERT || CWeather::OldWeatherType == WEATHER_FOGGY_SF || CWeather::OldWeatherType == WEATHER_SANDSTORM_DESERT);
}
