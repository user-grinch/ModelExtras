#pragma once
#include <plugin.h>
#include <extender/PedExtender.h>
#include <CRGBA.h>

struct RpAtomic;
struct RpClump;
struct RpMaterial;

using namespace plugin;

struct PedData {
	bool m_bUsingPedCols = false;
	bool m_bInitialized = false;
	CRGBA m_Colors[4];

	PedData(CPed *ptr){};
	void Init(CPed *pPed);
	~PedData() {}
};

struct PedColors {
private:
	static inline PedExtendedData<PedData> m_PedData;
	static inline CPed *m_pCurrentPed;

	static void SetEditableMaterials(RpClump *pClump);

public:
	static void Initialize();
};