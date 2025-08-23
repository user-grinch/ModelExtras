#pragma once
const int TEXTURE_LIMIT = 8;

// Souce Code is based on PedFuncs https://github.com/JuniorDjjr/PedFuncs
struct PedExtended {
public:
	int curRemapNum[TEXTURE_LIMIT];
	int TotalRemapNum[TEXTURE_LIMIT];
	RwTexture * originalRemap[TEXTURE_LIMIT];
	std::list<RwTexture*> remaps[TEXTURE_LIMIT];

	PedExtended(CPed *ped) {
		for (int i = 0; i < TEXTURE_LIMIT; ++i) {
			originalRemap[i] = nullptr;
			curRemapNum[i] = -1;
			TotalRemapNum[i] = 0;
		}
	}
};

class DontRepeatIt {
public:
	int modelId;
	int lastNum[TEXTURE_LIMIT];

	DontRepeatIt() {
		modelId = -1;
		for (int i = 0; i < TEXTURE_LIMIT; ++i) {
			lastNum[i] = -1;
		}
	}
};

class PedRemap {
private: 
    static inline bool txdsNotLoadedYet = true;
    static inline bool alreadyLoaded = false;
    static inline unsigned int cutsceneRunLastTime = 0;
    static inline uintptr_t ORIGINAL_AssignRemapTxd = 0;
    static inline uintptr_t ORIGINAL_RwTexDictionaryFindNamedTexture = 0;

    static inline RwTexDictionary* pedstxdArray[4];
    static inline int pedstxdIndexArray[4];
    static inline bool anyAdditionalPedsTxd;

    static void FindRemaps(CPed* ped);
    static void CustomAssignRemapTxd(const char* txdName, uint16_t txdId);
    static RwTexture* __cdecl Custom_RwTexDictionaryFindNamedTexture(RwTexDictionary* dict, const char* name);
    static void LoadAdditionalTxds();

public:
    static inline PedExtendedData<PedExtended> extData;
	
	static void Initialize();
};