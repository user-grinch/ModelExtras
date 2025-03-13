#pragma once
#include <string>

// This enum order needs to be same as ePanels
enum class eParentType {
    FrontLeftWing,
    FrontRightWing,
    RearLeftWing,
    RearRightWing,
    WindScreen,
    FrontBumper,
    RearBumper,
    Unknown,
};

inline eParentType eParentTypeFromString(const std::string& str) {
    if (str == "front-left-wing") return eParentType::FrontLeftWing;
    else if (str == "front-right-wing") return eParentType::FrontRightWing;
    else if (str == "rear-left-wing") return eParentType::RearLeftWing;
    else if (str == "rear-right-wing") return eParentType::RearRightWing;
    else if (str == "wind-screen") return eParentType::WindScreen;
    else if (str == "front-bumper") return eParentType::FrontBumper;
    else if (str == "rear-bumper") return eParentType::RearBumper;
    return eParentType::Unknown;
}