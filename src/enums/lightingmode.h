#pragma once

enum class eLightingMode { Directional, Inversed, NonDirectional, Rotator };

inline eLightingMode GetLightingMode(std::string dir) {
  if (dir == "directional") {
    return eLightingMode::Directional;
  } else if (dir == "rotator") {
    return eLightingMode::Rotator;
  } else if (dir == "inversed-directional") {
    return eLightingMode::Inversed;
  }
  return eLightingMode::NonDirectional;
}