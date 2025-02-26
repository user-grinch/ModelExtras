#include "pch.h"
#include <plugin.h>
#include "util.h"
#include <regex>
#include <CWeaponInfo.h>

std::string Util::GetRegexVal(const std::string& src, const std::string&& ptrn, const std::string&& def) {
    std::smatch match;
    std::regex_search(src.begin(), src.end(), match, std::regex(ptrn));

    if (match.empty())
        return def;
    else
        return match[1];

}

void Util::SetFrameRotationX(RwFrame* frame, float angle) {
    RwFrameRotate(frame, (RwV3d*)0x008D2E00, (RwReal)angle, rwCOMBINEPRECONCAT);
    RwFrameUpdateObjects(frame);
}

void Util::SetFrameRotationY(RwFrame* frame, float angle) {
    RwFrameRotate(frame, (RwV3d*)0x008D2E0C, (RwReal)angle, rwCOMBINEPRECONCAT);
    RwFrameUpdateObjects(frame);
}

void Util::SetFrameRotationZ(RwFrame* frame, float angle) {
    RwFrameRotate(frame, (RwV3d*)0x008D2E18, (RwReal)angle, rwCOMBINEPRECONCAT);
    RwFrameUpdateObjects(frame);
}

float GetATanOfXY(float x, float y) {
    if (x > 0.0f) {
        return atan2(y, x);
    }
    else if (x < 0.0f) {
        if (y >= 0.0f) {
            return atan2(y, x) + 3.1416f;
        }
        else {
            return atan2(y, x) - 3.1416f;
        }
    }
    else { // x is 0.0f
        if (y > 0.0f) {
            return 0.5f * 3.1416f;
        }
        else if (y < 0.0f) {
            return -0.5f * 3.1416f;
        }
        else {
         // x and y are both 0, undefined result
            return 0.0f;
        }
    }
}

float Util::GetMatrixRotationX(RwMatrix* matrix)
{
    float x = matrix->right.x;
    float y = matrix->right.y;
    float z = matrix->right.z;
    float angle = GetATanOfXY(z, sqrt(x * x + y * y)) * 57.295776f - 90.0f;
    while (angle < 0.0)
        angle += 360.0;
    return angle;
}

float Util::GetMatrixRotationY(RwMatrix* matrix)
{
    float x = matrix->up.x;
    float y = matrix->up.y;
    float z = matrix->up.z;
    float angle = GetATanOfXY(z, sqrt(x * x + y * y)) * 57.295776f - 90.0f;
    while (angle < 0.0)
        angle += 360.0;

    return angle;
}

float Util::GetMatrixRotationZ(RwMatrix* matrix)
{
    float angle = GetATanOfXY(matrix->right.x, matrix->right.y) * 57.295776f - 90.0f;
    while (angle < 0.0)
        angle += 360.0;
    return angle;
}

void Util::SetMatrixRotationX(RwMatrix* matrix, float angle)
{
    angle -= GetMatrixRotationX(matrix);

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

void Util::SetMatrixRotationY(RwMatrix* matrix, float angle)
{
    angle -= GetMatrixRotationY(matrix);

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

void Util::SetMatrixRotationZ(RwMatrix* matrix, float angle)
{
    angle -= GetMatrixRotationZ(matrix);

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

uint32_t Util::GetChildCount(RwFrame* parent) {
    RwFrame* child = parent->child;
    uint32_t count = 0U;
    if (child) {
        while (child) {
            ++count;
            child = child->next;
        }
        return count;
    }
    return 0U;
}

void Util::StoreChilds(RwFrame* parent, std::vector<RwFrame*>& store) {
    RwFrame* child = parent->child;

    while (child) {
        store.push_back(child);
        child = child->next;
    }
}


void Util::ShowAllAtomics(RwFrame* frame) {
    if (!rwLinkListEmpty(&frame->objectList)) {
        RwObjectHasFrame* atomic;

        RwLLLink* current = rwLinkListGetFirstLLLink(&frame->objectList);
        RwLLLink* end = rwLinkListGetTerminator(&frame->objectList);

        do {
            atomic = rwLLLinkGetData(current, RwObjectHasFrame, lFrame);
            atomic->object.flags |= rpATOMICRENDER; // clear

            current = rwLLLinkGetNext(current);
        } while (current != end);
    }
}

void Util::HideAllAtomics(RwFrame* frame) {
    if (!rwLinkListEmpty(&frame->objectList)) {
        RwObjectHasFrame* atomic;

        RwLLLink* current = rwLinkListGetFirstLLLink(&frame->objectList);
        RwLLLink* end = rwLinkListGetTerminator(&frame->objectList);

        while (current != end) {
            atomic = rwLLLinkGetData(current, RwObjectHasFrame, lFrame);
            atomic->object.flags &= ~rpATOMICRENDER;

            current = rwLLLinkGetNext(current);
        }
    }
}

void Util::HideChildWithName(RwFrame* parent_frame, const char* name) {
    RwFrame* child = parent_frame->child;
    while (child) {
        if (!strcmp(GetFrameNodeName(child), name)) {
            Util::HideAllAtomics(child);
            return;
        }
        child = child->next;
    }
}

void Util::ShowChildWithName(RwFrame* parent_frame, const char* name) {
    RwFrame* child = parent_frame->child;
    while (child) {
        if (!strcmp(GetFrameNodeName(child), name)) {
            Util::ShowAllAtomics(child);
            return;
        }
        child = child->next;
    }
}

void Util::HideAllChilds(RwFrame* parent_frame) {
    RwFrame* child = parent_frame->child;
    while (child) {
        Util::HideAllAtomics(child);
        child = child->next;
    }
    Util::HideAllAtomics(parent_frame);
}

void Util::ShowAllChilds(RwFrame* parent_frame) {
    RwFrame* child = parent_frame->child;
    while (child) {
        Util::ShowAllAtomics(child);
        child = child->next;
    }
    Util::ShowAllAtomics(parent_frame);
}

// Taken from vehfuncs
float Util::GetVehicleSpeedRealistic(CVehicle* vehicle) {
    float wheelSpeed = 0.0;
    CVehicleModelInfo* vehicleModelInfo = (CVehicleModelInfo*)CModelInfo::GetModelInfo(vehicle->m_nModelIndex);
    if (vehicle->m_nVehicleSubClass == VEHICLE_BIKE || vehicle->m_nVehicleSubClass == VEHICLE_BMX) {
        CBike* bike = (CBike*)vehicle;
        wheelSpeed = ((bike->m_fWheelSpeed[0] * vehicleModelInfo->m_fWheelSizeFront) +
                      (bike->m_fWheelSpeed[1] * vehicleModelInfo->m_fWheelSizeRear)) / 2.0f;
    }
    else if (vehicle->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || vehicle->m_nVehicleSubClass == VEHICLE_MTRUCK || vehicle->m_nVehicleSubClass == VEHICLE_QUAD) {
        CAutomobile* automobile = (CAutomobile*)vehicle;
        wheelSpeed = ((automobile->m_fWheelSpeed[0] + automobile->m_fWheelSpeed[1] * vehicleModelInfo->m_fWheelSizeFront) +
                      (automobile->m_fWheelSpeed[2] + automobile->m_fWheelSpeed[3] * vehicleModelInfo->m_fWheelSizeRear)) / 4.0f;
    }
    else {
        return (vehicle->m_vecMoveSpeed.Magnitude() * 50.0f) * 3.6f;
    }
    wheelSpeed /= 2.45f; // tweak based on distance (manually testing)
    wheelSpeed *= -186.0f; // tweak based on km/h

    return wheelSpeed;
}

unsigned int Util::GetEntityModel(void* ptr, eModelEntityType type) {
    int model = 0;
    if (type == eModelEntityType::Weapon) {
        CWeaponInfo* pWeaponInfo = CWeaponInfo::GetWeaponInfo(reinterpret_cast<CWeapon*>(ptr)->m_eWeaponType,
                                                                FindPlayerPed()->GetWeaponSkill(reinterpret_cast<CWeapon*>(ptr)->m_eWeaponType));
        if (pWeaponInfo) {
            model = pWeaponInfo->m_nModelId1;
        }
    }
    else {
        model = reinterpret_cast<CEntity*>(ptr)->m_nModelIndex;
    }
    return model;
}

RwTexture* Util::LoadTextureFromFile(const char* filename, RwUInt8 alphaValue) {

    RwImage* image = RtPNGImageRead(filename);
    if (!image) {
        return nullptr;
    }

    RwInt32 width, height, depth, flags;
    RwImageFindRasterFormat(image, 4, &width, &height, &depth, &flags);

    RwRaster* raster = RwRasterCreate(width, height, depth, flags);
    if (!raster) {
        RwImageDestroy(image);
        return nullptr;
    }

    // Set the alpha value for each pixel
    RwRGBA* pixels = (RwRGBA*)RwImageGetPixels(image);
    for (RwInt32 y = 0; y < height; y++) {
        for (RwInt32 x = 0; x < width; x++) {
            RwRGBA* pixel = pixels + (y * width + x);
            pixel->red = (pixel->red * alphaValue) / 255;
            pixel->green = (pixel->green * alphaValue) / 255;
            pixel->blue = (pixel->blue * alphaValue) / 255;
            pixel->alpha = alphaValue;
        }
    }

    RwRasterSetFromImage(raster, image);
    RwImageDestroy(image);
    return RwTextureCreate(raster);
}

#include <rwcore.h>
#include <rwplcore.h>
#include <rpworld.h>

RwTexture* Util::FindTextureInDict(RpMaterial* pMat, RwTexDictionary* pDict) {
    const std::string baseName = pMat->texture->name;

    // texture glitch fix
    const std::vector<std::string> texNames = {
        // baseName,
        baseName + "on",
        baseName + "_on",
        // "sirenlighton",
        // "sirenlight_on",
        // "vehiclelightson128"
    };

    RwTexture* pTex = nullptr;
    for (const auto& name : texNames) {
        pTex = RwTexDictionaryFindNamedTexture(pDict, name.c_str());
        if (pTex) {
            break;
        }
    }
    return pTex;
}

bool FileCheck(const char* name) {
    struct stat buffer;

    return (stat(name, &buffer) == 0);
}

// Taken from _AG (vHud)

RwTexture* Util::LoadDDSTextureCB(const char* path, const char* name) {
    char file[512];

    sprintf(file, "%s\\%s", path, name);

    return RwD3D9DDSTextureRead(file, NULL);
}

RwTexture* Util::LoadBMPTextureCB(const char* path, const char* name) {
    int w, h, d, f;
    char file[512];
    RwTexture* texture = NULL;

    sprintf(file, "%s\\%s.bmp", path, name);

    if (file && FileCheck(file)) {
        if (RwImage* img = RtBMPImageRead(file)) {
            RwImageFindRasterFormat(img, rwRASTERTYPETEXTURE, &w, &h, &d, &f);

            if (RwRaster* raster = RwRasterCreate(w * 0.25f, h * 0.25f, d, f)) {
                RwRasterSetFromImage(raster, img);

                if (texture = RwTextureCreate(raster)) {
                    RwTextureSetName(texture, name);

                    if ((texture->raster->cFormat & 0x80) == 0)
                        RwTextureSetFilterMode(texture, rwFILTERLINEAR);
                    else
                        RwTextureSetFilterMode(texture, rwFILTERLINEARMIPLINEAR);

                    RwTextureSetAddressing(texture, rwTEXTUREADDRESSWRAP);
                }
            }

            RwImageDestroy(img);
        }
    }

    return texture;
}

RwTexture* Util::LoadPNGTextureCB(const char* path, const char* name) {
    int w, h, d, f;
    char file[512];
    RwTexture* texture = NULL;

    sprintf(file, "%s\\%s.png", path, name);

    if (file && FileCheck(file)) {
        if (RwImage* img = RtPNGImageRead(file)) {
            RwImageFindRasterFormat(img, rwRASTERTYPETEXTURE, &w, &h, &d, &f);

            if (RwRaster* raster = RwRasterCreate(w, h, d, f)) {
                RwRasterSetFromImage(raster, img);

                if (texture = RwTextureCreate(raster)) {
                    RwTextureSetName(texture, name);

                    if ((texture->raster->cFormat & 0x80) == 0)
                        RwTextureSetFilterMode(texture, rwFILTERLINEAR);
                    else
                        RwTextureSetFilterMode(texture, rwFILTERLINEARMIPLINEAR);

                    RwTextureSetAddressing(texture, rwTEXTUREADDRESSWRAP);
                }
            }

            RwImageDestroy(img);
        }
    }

    return texture;
}