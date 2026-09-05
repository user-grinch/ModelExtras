#include "pch.h"
#include "worldutil.h"
#include <CClock.h>
#include <CWeather.h>

bool WorldUtil::IsNightTime() {
    uint8_t hours = CClock::ms_nGameClockHours;
    return (hours >= 21 || hours < 6);
}

bool WorldUtil::IsFoggy() {
    return (CWeather::WeatherTypeInList == WEATHER_FOGGY_SF || CWeather::ForcedWeatherType == WEATHER_FOGGY_SF);
}
