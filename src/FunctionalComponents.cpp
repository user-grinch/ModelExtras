#include <math.h>
#include "plugin.h"
#include "Common.h"
#include "CTimer.h"
#include "NodeName.h"

using namespace plugin;

#include "Brakes.h"
#include "Chain.h"
#include "Gear.h"
#include "Meter.h"
#include "Sparks.h"
#include "CHud.h"

static void ProcessNodesRecursive(RwFrame * frame, CVehicle* pObj, FVCData &data);

std::fstream lg;
VehicleExtendedData<FVCData> vehdata;

class FVC {
public:
	FVC() {

		lg.open("FunctionalComponents.log", std::fstream::out | std::fstream::trunc);
		lg << "Log started" << std::endl;
		Events::vehicleRenderEvent += [](CVehicle* pVeh)
		{
			FVCData &data = vehdata.Get(pVeh);
			data.realistic_speed = static_cast<int>(GetVehicleSpeedRealistic(pVeh));

			ProcessNodesRecursive((RwFrame *)pVeh->m_pRwClump->object.parent, pVeh, data);
		};
	}
} fvc;

static void ProcessNodesRecursive(RwFrame * frame, CVehicle* pVeh, FVCData &data)
{
	if(frame)
	{
		const std::string name = GetFrameNodeName(frame);

		if (name[0] == 'f' && name[1] == 'c' && name[2] == '_')
		{

			data.timer = CTimer::m_snTimeInMilliseconds;

			ProcessChain(name, frame, data, pVeh);

			ProcessFrontBrake(name, frame, data, pVeh);
			ProcessRearBrake(name, frame, data, pVeh);

			ProcessClutch(name, frame, data, pVeh);
			ProcessGearLever(name, frame, data, pVeh);
			ProcessGearMeter(name, frame, data, pVeh);
			ProcessOdoMeter(name, frame, data, pVeh);
			ProcessSparks(name, frame, data, pVeh);
			ProcessSpeedoMeter(name, frame, data, pVeh);
		}
		if (RwFrame * newFrame = frame->child)
			ProcessNodesRecursive(newFrame, pVeh, data);
		if (RwFrame * newFrame = frame->next)
			ProcessNodesRecursive(newFrame, pVeh, data);
	}
	return;
}

