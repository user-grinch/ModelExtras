#pragma once
#include <rwcore.h>

// Taken from Junior's Vehfuncs
class MatrixUtil {
public:
    static double GetRotationX(RwMatrix *matrix);
    static double GetRotationY(RwMatrix *matrix);
    static double GetRotationZ(RwMatrix *matrix);
    static void ResetRotation(RwMatrix *matrix);

    static void SetRotationXAbsolute(RwMatrix *matrix, double angle);
    static void SetRotationZAbsolute(RwMatrix *matrix, double angle);
};