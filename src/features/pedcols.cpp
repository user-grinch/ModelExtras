#include "pch.h"
#include "pedcols.h"
#include "utils/datamgr.h"

using namespace plugin;

#define RwRGBAGetRGB(a) (*(DWORD *)&(a) & 0xFFFFFF)

void PedColors::SetEditableMaterials(RpClump *pClump) {
	RpClumpForAllAtomics(pClump, [](RpAtomic * pAtomic, void *data) {
		if (rwObjectGetFlags(pAtomic) & rpATOMICRENDER) {
			RpGeometryForAllMaterials(pAtomic->geometry, [](RpMaterial *pMaterial, void* data) {
				if (PedColors::m_pCurrentPed) {
					int idx = 0;
					auto &data = PedColors::m_PedData.Get(PedColors::m_pCurrentPed);
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
					data.m_OriginalColors.push_back(std::make_pair(&pMaterial->color, pMaterial->color));
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

PedData::PedData(CPed *pPed) {
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

void PedColors::Init() {
	Events::pedRenderEvent.before += [](CPed *pPed) {
		auto &data = PedColors::m_PedData.Get(pPed);
		if (data.m_bUsingPedCols) {
			PedColors::m_pCurrentPed = pPed;
			SetEditableMaterials(pPed->m_pRwClump);
		}
	};

	Events::pedRenderEvent.after += [](CPed *pPed) {
		auto &data = PedColors::m_PedData.Get(pPed);
		for (auto &e : data.m_OriginalColors) {
			*e.first = e.second;
		}
		data.m_OriginalColors.clear();
	};
}