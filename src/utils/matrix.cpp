#include "pch.h"
#include "matrix.h"

bool MatrixUtil::CreateBackup(RwFrame* frame) {
    auto* backup = new RwMatrix();
    if (!backup) {
        gLogger->error("Failed to create matrix backup");
        FRAME_EXTENSION(frame)->pOrigMatrix = nullptr;
        return false;
    }

    RwMatrix* origin = &frame->modelling;
    backup->at = origin->at;
    backup->pos = origin->pos;
    backup->right = origin->right;
    backup->up = origin->up;

    FRAME_EXTENSION(frame)->pOrigMatrix = backup;
    return true;
}

void MatrixUtil::RestoreBackup(RwMatrix* dest, RwMatrix* backup) {
    if (!backup) {
        gLogger->error("Failed to restore matrix backup");
        return;
    }

    dest->at = backup->at;
    dest->pos = backup->pos;
    dest->right = backup->right;
    dest->up = backup->up;

    RwMatrixUpdate(dest);
}

float GetATanOfXY(float x, float y)
{
    if (x > 0.0f)
    {
        return atan2(y, x);
    }
    else if (x < 0.0f)
    {
        if (y >= 0.0f)
        {
            return atan2(y, x) + 3.1416f;
        }
        else
        {
            return atan2(y, x) - 3.1416f;
        }
    }
    else
    { // x is 0.0f
        if (y > 0.0f)
        {
            return 0.5f * 3.1416f;
        }
        else if (y < 0.0f)
        {
            return -0.5f * 3.1416f;
        }
        else
        {
            // x and y are both 0, undefined result
            return 0.0f;
        }
    }
}

float MatrixUtil::GetRotationX(RwMatrix *matrix)
{
    float x = matrix->right.x;
    float y = matrix->right.y;
    float z = matrix->right.z;
    float angle = GetATanOfXY(z, sqrt(x * x + y * y)) * 57.295776f - 90.0f;
    while (angle < 0.0)
        angle += 360.0;
    return angle;
}

float MatrixUtil::GetRotationY(RwMatrix *matrix)
{
    float x = matrix->up.x;
    float y = matrix->up.y;
    float z = matrix->up.z;
    float angle = GetATanOfXY(z, sqrt(x * x + y * y)) * 57.295776f - 90.0f;
    while (angle < 0.0)
        angle += 360.0;

    return angle;
}

float MatrixUtil::GetRotationZ(RwMatrix *matrix)
{
    if (!matrix)
        return 0.0f;

    float angle = GetATanOfXY(matrix->right.x, matrix->right.y) * 57.295776f - 90.0f;
    while (angle < 0.0)
        angle += 360.0;
    return angle;
}

void MatrixUtil::ResetRotation(RwMatrix *matrix)
{
    matrix->right = {1.0f, 0.0f, 0.0f};
    matrix->up = {0.0f, 1.0f, 0.0f};
    matrix->at = {0.0f, 0.0f, 1.0f};
}

void MatrixUtil::SetRotationX(RwMatrix *matrix, float angle)
{
    angle -= GetRotationX(matrix);

    // Ensure the angle is within [0, 360) range
    while (angle >= 360.0f)
        angle -= 360.0f;

    while (angle < 0.0f)
        angle += 360.0f;

    // Convert angle to radians
    float angleRad = angle / 57.295776f;

    // Calculate the sine and cosine of the angle
    float sinAngle = sin(angleRad);
    float cosAngle = cos(angleRad);

    // Store the existing up and at vectors
    RwV3d up = matrix->up;
    RwV3d at = matrix->at;

    // Update the up and at vectors for the X-axis rotation
    matrix->up.x = cosAngle * up.x + sinAngle * at.x;
    matrix->up.y = cosAngle * up.y + sinAngle * at.y;
    matrix->up.z = cosAngle * up.z + sinAngle * at.z;

    matrix->at.x = -sinAngle * up.x + cosAngle * at.x;
    matrix->at.y = -sinAngle * up.y + cosAngle * at.y;
    matrix->at.z = -sinAngle * up.z + cosAngle * at.z;

    // Normalize the vectors to ensure they remain orthogonal
    RwV3dNormalize(&matrix->up, &matrix->up);
    RwV3dNormalize(&matrix->at, &matrix->at);
}

void MatrixUtil::SetRotationY(RwMatrix *matrix, float angle)
{
    angle -= GetRotationY(matrix);

    // Ensure the angle is within [0, 360) range
    while (angle >= 360.0f)
        angle -= 360.0f;

    while (angle < 0.0f)
        angle += 360.0f;

    // Convert angle to radians
    float angleRad = angle / 57.295776f;

    // Calculate the sine and cosine of the angle
    float sinAngle = sin(angleRad);
    float cosAngle = cos(angleRad);

    // Store the existing right and at vectors
    RwV3d right = matrix->right;
    RwV3d at = matrix->at;

    // Update the right and at vectors for the Y-axis rotation
    matrix->right.x = cosAngle * right.x + sinAngle * at.x;
    matrix->right.y = cosAngle * right.y + sinAngle * at.y;
    matrix->right.z = cosAngle * right.z + sinAngle * at.z;

    matrix->at.x = -sinAngle * right.x + cosAngle * at.x;
    matrix->at.y = -sinAngle * right.y + cosAngle * at.y;
    matrix->at.z = -sinAngle * right.z + cosAngle * at.z;

    // Normalize the vectors to ensure they remain orthogonal
    RwV3dNormalize(&matrix->right, &matrix->right);
    RwV3dNormalize(&matrix->at, &matrix->at);
}

void MatrixUtil::SetRotationZ(RwMatrix *matrix, float angle)
{
    angle -= GetRotationZ(matrix);

    // Ensure the angle is within [0, 360) range
    while (angle >= 360.0f)
        angle -= 360.0f;

    while (angle < 0.0f)
        angle += 360.0f;

    // Convert angle to radians
    float angleRad = angle / 57.295776f;

    // Calculate the sine and cosine of the angle
    float sinAngle = sin(angleRad);
    float cosAngle = cos(angleRad);

    // Store the existing right and up vectors
    RwV3d right = matrix->right;
    RwV3d up = matrix->up;

    // Set the right vector for the Z-axis rotation
    matrix->right.x = cosAngle * right.x - sinAngle * right.y;
    matrix->right.y = sinAngle * right.x + cosAngle * right.y;

    // Set the up vector for the Z-axis rotation
    matrix->up.x = cosAngle * up.x - sinAngle * up.y;
    matrix->up.y = sinAngle * up.x + cosAngle * up.y;

    // Normalize the vectors to ensure they remain orthogonal
    RwV3dNormalize(&matrix->right, &matrix->right);
    RwV3dNormalize(&matrix->up, &matrix->up);
}