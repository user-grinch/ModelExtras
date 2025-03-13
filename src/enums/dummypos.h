#pragma once

enum class eDummyPos {
    None,
    Front,
    Rear,
    Left,
    Right,
    MiddleLeft,
    MiddleRight,
    FrontLeft,
    RearLeft,
    FrontRight,
    RearRight
};

inline eDummyPos eDummyPosFromString(const std::string& str) {
    if (str == "front") return eDummyPos::Front;
    else if (str == "rear") return eDummyPos::Rear;
    else if (str == "left") return eDummyPos::Left;
    else if (str == "right") return eDummyPos::Right;
    else if (str == "middle-left") return eDummyPos::MiddleLeft;
    else if (str == "middle-right") return eDummyPos::MiddleRight;
    else if (str == "front-left") return eDummyPos::FrontLeft;
    else if (str == "rear-left") return eDummyPos::RearLeft;
    else if (str == "front-right") return eDummyPos::FrontRight;
    else if (str == "rear-right") return eDummyPos::RearRight;
    return eDummyPos::None;
}
