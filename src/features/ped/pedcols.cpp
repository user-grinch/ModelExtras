#include "pch.h"
#include "pedcols.h"
#include <vector>
#include <RenderWare.h>
#include <rw/rpworld.h>
#include <rw/rwplcore.h>
#include "datamgr.h"

using namespace plugin;

#define RwRGBAGetRGB(a) (*(DWORD *)&(a) & 0xFFFFFF)
std::unordered_map<CPed*, std::vector<std::pair<void *, int>>> store;

void PedColors::SetEditableMaterials(RpClump *pClump) {
	RpClumpForAllAtomics(pClump, [](RpAtomic * pAtomic, void *data) {
		if (rwObjectGetFlags(pAtomic) & rpATOMICRENDER) {
			RpGeometryForAllMaterials(pAtomic->geometry, [](RpMaterial *pMaterial, void* data) {
				if (m_pCurrentPed) {
					int idx = 0;
					auto &data = m_PedData.Get(m_pCurrentPed);
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
					store[m_pCurrentPed].push_back(std::make_pair(&pMaterial->color, *reinterpret_cast<int *>(&pMaterial->color)));
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
	uint32_t model = pPed->m_nModelIndex;
	auto jsonData = DataMgr::Get(model);

	if (jsonData.contains("pedcols")) {
		const auto& pedCols = jsonData["pedcols"];

		if (pedCols.contains("colors") && pedCols.contains("variations")) {
			const auto& colorBank = pedCols["colors"];
			const auto& variations = pedCols["variations"];

			if (!variations.empty()) {
				size_t varIdx = RandomNumberInRange<size_t>(0, variations.size() - 1);
				const auto& selectedVar = variations[varIdx];

				const std::vector<std::string> keys = { "primary", "secondary", "tertiary", "quaternary" };

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

void PedColors::Initialize() {
	Events::pedSetModelEvent.after += [](CPed *pPed, int model) {
		auto &data = m_PedData.Get(pPed);
		if (!data.m_bInitialized) {
			data.Init(pPed);
			data.m_bInitialized = true;
		}
	};

	Events::pedRenderEvent.before += [](CPed *pPed) {
		auto &data = m_PedData.Get(pPed);
		if (data.m_bUsingPedCols) {
			m_pCurrentPed = pPed;
			SetEditableMaterials(pPed->m_pRwClump);
		}
	};

	Events::pedRenderEvent.after += [](CPed *pPed) {
		for (auto &e : store[pPed]) {
			*static_cast<int *>(e.first) = e.second;
		}
		store[pPed].clear();
	};
}