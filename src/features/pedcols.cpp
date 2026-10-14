#include "pch.h"
#include "pedcols.h"
#include <vector>
#include <RenderWare.h>
#include <rw/rpworld.h>
#include <rw/rwplcore.h>
#include "utils/datamgr.h"

using namespace plugin;

#define RwRGBAGetRGB(a) (*(DWORD *)&(a) & 0xFFFFFF)

void PedColors::SetEditableMaterials(RpClump *pClump) {
	if (!pClump || !m_pCurrentPed) return;
	RpClumpForAllAtomics(pClump, [](RpAtomic * pAtomic, void *data) {
        if (!pAtomic || !pAtomic->geometry) return pAtomic;
        auto &state = PedColors::m_PedData.Get(PedColors::m_pCurrentPed);
        if (state.m_Colors.size() < 4) return pAtomic;
        if (rwObjectGetFlags(pAtomic) & rpATOMICRENDER) {
            auto end = state.m_OriginalFlags.end();
            if (std::find_if(state.m_OriginalFlags.begin(), end, [=](const auto &e) { return e.first == pAtomic->geometry; }) == end) {
                if (state.m_OriginalFlags.size() == state.m_OriginalFlags.capacity()) return pAtomic;
                state.m_OriginalFlags.emplace_back(pAtomic->geometry, (pAtomic->geometry->flags & rpGEOMETRYMODULATEMATERIALCOLOR) != 0);
            }
			RpGeometryForAllMaterials(pAtomic->geometry, [](RpMaterial *pMaterial, void* data) {
				if (pMaterial && PedColors::m_pCurrentPed) {
					int idx = 0;
					auto &data = PedColors::m_PedData.Get(PedColors::m_pCurrentPed);
                    if (std::any_of(data.m_OriginalColors.begin(), data.m_OriginalColors.end(), [=](const auto &e) { return e.first == &pMaterial->color; })) return pMaterial;
					switch (RwRGBAGetRGB(pMaterial->color))
					{
					case 0x00FF3C:
						idx = 0;
						break;
					case 0xAF00FF:
						idx = 1;
						break;
					case 0xFFFF00:
						idx = 2;
						break;
					case 0xFF00FF:
						idx = 3;
						break;
					default:
						return pMaterial;
					}
					if (data.m_OriginalColors.size() == data.m_OriginalColors.capacity()) return pMaterial;
					data.m_OriginalColors.emplace_back(&pMaterial->color, pMaterial->color);
					pMaterial->color.red = data.m_Colors[idx].r;
					pMaterial->color.green = data.m_Colors[idx].g;
					pMaterial->color.blue = data.m_Colors[idx].b;
				}

				return pMaterial;
			}, nullptr);

			pAtomic->geometry->flags |= rpGEOMETRYMODULATEMATERIALCOLOR;
		}
		return pAtomic;
	}, nullptr);
}

void PedData::Init(CPed *pPed) {
    m_bUsingPedCols = false;
    m_Colors.clear();
    if (!pPed || !pPed->m_pRwClump) return;
    struct Counts { size_t materials = 0, atomics = 0; } counts;
    RpClumpForAllAtomics(pPed->m_pRwClump, [](RpAtomic *atomic, void *context) {
        if (atomic && atomic->geometry) {
            auto &counts = *static_cast<Counts *>(context);
            ++counts.atomics; counts.materials += RpGeometryGetNumMaterials(atomic->geometry);
        }
        return atomic;
    }, &counts);
    m_OriginalColors.reserve(counts.materials);
    m_OriginalFlags.reserve(counts.atomics);
	uint32_t model = pPed->m_nModelIndex;
	const auto &jsonData = DataMgr::Get(model);

	if (jsonData.contains("pedcols")) {
		const auto& pedCols = jsonData["pedcols"];

		if (pedCols.contains("colors") && pedCols.contains("variations")) {
			const auto& colorBank = pedCols["colors"];
			const auto& variations = pedCols["variations"];

			if (!variations.empty()) {
				size_t varIdx = RandomNumberInRange<size_t>(0, variations.size() - 1);
				const auto& selectedVar = variations[varIdx];

				const std::vector<std::string> keys = { "primary", "secondary", "tertiary", "quaternary" };
				m_Colors.assign(4, CRGBA(255, 255, 255, 255));

				for (size_t i = 0; i < keys.size(); ++i) {
					if (selectedVar.contains(keys[i])) {
						int colorIndex = selectedVar[keys[i]];

						if (colorIndex >= 0 && colorIndex < (int)colorBank.size()) {
							const auto& color = colorBank[colorIndex];

							m_Colors[i] = CRGBA(
								color.value("red", 255),
								color.value("green", 255),
								color.value("blue", 255)
							);
						}
					}
				}
				m_bUsingPedCols = true;
			}
		}
	}
}

void PedColors::Init() {
	Events::pedSetModelEvent.after += [](CPed *pPed, int model) {
		if (!pPed) return;
		auto &data = PedColors::m_PedData.Get(pPed);
        data.Init(pPed);
        data.m_bInitialized = true;
	};

	Events::pedRenderEvent.before += [](CPed *pPed) {
		if (!pPed || !pPed->m_pRwClump || !CBaseFeature::IsEnabled(eFeatureMatrix::PedCols)) return;
		auto &data = PedColors::m_PedData.Get(pPed);
		if (data.m_bUsingPedCols) {
			PedColors::m_pCurrentPed = pPed;
			SetEditableMaterials(pPed->m_pRwClump);
		}
	};

	Events::pedRenderEvent.after += [](CPed *pPed) {
        if (!pPed) return;
		auto &data = PedColors::m_PedData.Get(pPed);
        for (auto &e : data.m_OriginalColors) if (e.first) *e.first = e.second;
        for (auto &[geometry, enabled] : data.m_OriginalFlags)
            if (geometry) geometry->flags = enabled ? geometry->flags | rpGEOMETRYMODULATEMATERIALCOLOR : geometry->flags & ~rpGEOMETRYMODULATEMATERIALCOLOR;
        data.m_OriginalColors.clear(); data.m_OriginalFlags.clear();
        PedColors::m_pCurrentPed = nullptr;
	};
}