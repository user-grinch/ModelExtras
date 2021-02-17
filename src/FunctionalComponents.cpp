#include "pch.h"

#include "Brakes.h"
#include "Chain.h"
#include "Gear.h"
#include "Meter.h"

static void ProcessNodesRecursive(RwFrame * frame, CVehicle* pObj, FCData &data);

class FunctionalComponents {
public:
	FunctionalComponents() {

		lg.open("FunctionalComponents.log", std::fstream::out | std::fstream::trunc);
		lg << "Log started" << std::endl;
		Events::vehicleRenderEvent += [](CVehicle* pVeh)
		{
			FCData &data = vehdata.Get(pVeh);
			data.delta = CTimer::ms_fTimeScale;
			data.realistic_speed = static_cast<int>(GetVehicleSpeedRealistic(pVeh));
			data.timer = CTimer::m_snTimeInMilliseconds * CTimer::ms_fTimeScale;

			ProcessNodesRecursive((RwFrame *)pVeh->m_pRwClump->object.parent, pVeh, data);
		};
	}
} fc;

static void ProcessNodesRecursive(RwFrame * frame, CVehicle* pVeh, FCData &data)
{
	if(frame)
	{
		const std::string name = GetFrameNodeName(frame);

		if (name[0] == 'f' && name[1] == 'c' && name[2] == '_')
		{
			ProcessChain(name, frame, data, pVeh);

			ProcessFrontBrake(name, frame, data, pVeh);
			ProcessRearBrake(name, frame, data, pVeh);

			ProcessClutch(name, frame, data, pVeh);
			ProcessGearLever(name, frame, data, pVeh);
			ProcessGearMeter(name, frame, data, pVeh);
			ProcessOdoMeter(name, frame, data, pVeh);
			ProcessSpeedoMeter(name, frame, data, pVeh);
			ProcessRPMMeter(name, frame, data, pVeh);
		}
		
		if (RwFrame * newFrame = frame->child)
			ProcessNodesRecursive(newFrame, pVeh, data);
		if (RwFrame * newFrame = frame->next)
			ProcessNodesRecursive(newFrame, pVeh, data);
	}
	return;
}

