#pragma once
#include <rwcore.h>

// Taken from Junior's Vehfuncs
class MatrixUtil {
public:
    static bool CreateBackup(RwFrame* frame);
    static void RestoreBackup(RwMatrix* dest, RwMatrix* backup);

    static float GetRotationX(RwMatrix *matrix);
    static float GetRotationY(RwMatrix *matrix);
    static float GetRotationZ(RwMatrix *matrix);
    static void ResetRotation(RwMatrix *matrix);
    static void SetRotationX(RwMatrix *matrix, float angle);
    static void SetRotationY(RwMatrix *matrix, float angle);
    static void SetRotationZ(RwMatrix *matrix, float angle);
};