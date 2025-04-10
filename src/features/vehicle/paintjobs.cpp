#include "pch.h"
#include "paintjobs.h"
#include <plugin.h>
#include <CPools.h>

void PaintJobs::Load()
{
	for (const auto &e : std::filesystem::directory_iterator(MOD_DATA_PATH("paintjobs/")))
	{
		if (e.is_regular_file() && e.path().extension() == ".jsonc")
		{
			std::string filename = e.path().filename().string();
			int model = std::stoi(filename.substr(0, filename.find(".")));

			gLogger->info("Reading paintjob from {}", e.path().string());
			std::ifstream stream(e.path());

			nlohmann::json json;
			stream >> json;
			stream.close();

			for (nlohmann::json::iterator paintjob = json.begin(); paintjob != json.end(); ++paintjob)
			{
				int _paintjob = std::stoi(paintjob.key());
				nlohmann::json identifiers = paintjob.value().find("identifiers").value();
				for (nlohmann::json::iterator variable = identifiers.begin(); variable != identifiers.end(); ++variable)
				{
					m_PaintjobStore[model][variable.value()] = _paintjob;
				}
			}
		}
	}
}

void PaintJobs::Initialize()
{
	plugin::Events::initGameEvent += []()
	{
		Load();
	};

	plugin::Events::vehicleSetModelEvent += [](CVehicle *pVeh, int model)
	{
		m_PaintedFlag[CPools::ms_pVehiclePool->GetIndex(pVeh)] = false;
	};

	plugin::Events::vehicleRenderEvent += [](CVehicle *pVeh)
	{
		int index = CPools::ms_pVehiclePool->GetIndex(pVeh);
		if (!m_PaintedFlag[index] && m_PaintjobStore.count(pVeh->m_nModelIndex) && m_PaintjobStore[pVeh->m_nModelIndex].count((index)))
		{
			pVeh->SetRemap(m_PaintjobStore[pVeh->m_nModelIndex][index]);
			m_PaintedFlag[index] = true;
		}
	};
}