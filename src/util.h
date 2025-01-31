#pragma once

typedef enum class eModelEntityType eModelEntityType;

class Util {
public:
  RwTexture* FindTextureInDict(RpMaterial* pMat, RwTexDictionary* pDict);

    // Returns value of regex from source string
  static std::string GetRegexVal(const std::string& src, const std::string&& ptrn, const std::string&& def);

  // Returns the number of childs a parent contains
  static uint32_t GetChildCount(RwFrame* parent);

  // Returns the speed of the vehicle handler
  static float GetVehicleSpeedRealistic(CVehicle* vehicle);
  static unsigned int GetEntityModel(void* ptr, eModelEntityType type);

  // Rotate model frame
  static void SetFrameRotationX(RwFrame* frame, float angle);
  static void SetFrameRotationY(RwFrame* frame, float angle);
  static void SetFrameRotationZ(RwFrame* frame, float angle);

  // rotate matrix
  static float GetMatrixRotationX(RwMatrix* matrix);
  static float GetMatrixRotationY(RwMatrix* matrix);
  static float GetMatrixRotationZ(RwMatrix* matrix);

  static void SetMatrixRotationX(RwMatrix* matrix, float angle);
  static void SetMatrixRotationY(RwMatrix* matrix, float angle);
  static void SetMatrixRotationZ(RwMatrix* matrix, float angle);

  // Stores all the childs in a vector
  static void StoreChilds(RwFrame* parent_frame, std::vector<RwFrame*>& frame);

  static void HideAllAtomics(RwFrame* frame);
  static void ShowAllAtomics(RwFrame* frame);

  static void HideChildWithName(RwFrame* parent_frame, const char* name);
  static void ShowChildWithName(RwFrame* parent_frame, const char* name);
  static void HideAllChilds(RwFrame* parent_frame);
  static void ShowAllChilds(RwFrame* parent_frame);
  static RwTexture* LoadTextureFromFile(const char* filename, RwUInt8 alphaValue = 255);
};

