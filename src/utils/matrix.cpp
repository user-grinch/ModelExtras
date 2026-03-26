#include "pch.h"
#include "matrix.h"
#include "utils/frameextention.h"

bool MatrixUtil::CreateBackup(RwFrame *frame)
{
    auto *backup = new RwMatrix();
    if (!backup)
    {
        LOG(ERROR) << "Failed to create matrix backup";
        RwFrameExtension::Get(frame)->pOrigMatrix = nullptr;
        return false;
    }

    RwMatrix *origin = &frame->modelling;
    backup->at = origin->at;
    backup->pos = origin->pos;
    backup->right = origin->right;
    backup->up = origin->up;

    RwFrameExtension::Get(frame)->pOrigMatrix = backup;
    return true;
}

void MatrixUtil::RestoreBackup(RwMatrix *dest, RwMatrix *backup)
{
    if (!backup)
    {
        LOG(ERROR) << "Failed to restore matrix backup";
        return;
    }

    dest->at = backup->at;
    dest->pos = backup->pos;
    dest->right = backup->right;
    dest->up = backup->up;

    RwMatrixUpdate(dest);
}

double MatrixUtil::GetRotationX(RwMatrix *matrix)
{
    double y = matrix->up.y;
    double z = matrix->up.z;
    double angle = Util::RadToDeg(atan2(z, y));
    angle = Util::NormalizeAngle(angle);
    return angle;
}

double MatrixUtil::GetRotationY(RwMatrix *matrix)
{
    double x = matrix->at.x;
    double z = matrix->at.z;
    double angle = Util::RadToDeg(atan2(x, z));
    angle = Util::NormalizeAngle(angle);
    return angle;
}

double MatrixUtil::GetRotationZ(RwMatrix *matrix)
{
    double x = matrix->right.x;
    double y = matrix->right.y;
    double angle = Util::RadToDeg(atan2(y, x));
    return Util::NormalizeAngle(angle);
}

void MatrixUtil::ResetRotation(RwMatrix *matrix)
{
    matrix->right = {1.0f, 0.0f, 0.0f};
    matrix->up = {0.0f, 1.0f, 0.0f};
    matrix->at = {0.0f, 0.0f, 1.0f};
}

void MatrixUtil::SetRotationXAbsolute(RwMatrix *matrix, double angle)
{
    double angleRad = Util::DegToRad(angle);

    // Calculate the sine and cosine of the angle
    double sinAngle = sin(angleRad);
    double cosAngle = cos(angleRad);

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
    RwMatrixUpdate(matrix);
}

void MatrixUtil::SetRotationYAbsolute(RwMatrix *matrix, double angle)
{
    double angleRad = Util::DegToRad(angle);

    // Calculate the sine and cosine of the angle
    double sinAngle = sin(angleRad);
    double cosAngle = cos(angleRad);

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
    RwMatrixUpdate(matrix);
}

void MatrixUtil::SetRotationZAbsolute(RwMatrix *matrix, double angle)
{
    double angleRad = Util::DegToRad(angle);

    // Calculate the sine and cosine of the angle
    double sinAngle = sin(angleRad);
    double cosAngle = cos(angleRad);

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
    RwMatrixUpdate(matrix);
}