#pragma once

#ifndef PI
#define PI 3.14159265358979323846
#endif

class MathUtil
{
public:
    static inline float NormalizeAngle(float angle)
    {
        while (angle < 0.0f)
            angle += 360.0f;
        while (angle >= 360.0f)
            angle -= 360.0f;
        return angle;
    }

    static inline double RadToDeg(double rad)
    {
        return rad * (180.0 / PI);
    }

    static inline double DegToRad(double deg)
    {
        return deg * (PI / 180.0);
    }
};
