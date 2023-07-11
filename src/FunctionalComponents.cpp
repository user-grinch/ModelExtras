#include "pch.h"
#include "Brakes.h"
#include "Chain.h"
#include "Gear.h"
#include "Meter.h"

static void ProcessNodesRecursive(RwFrame * frame, CVehicle* pObj, FCData &data);

class FunctionalComponents {
public:
	FunctionalComponents() {
		if (HIWORD(BASS_GetVersion()) != BASSVERSION)
		{
			Log::Print<eLogLevel::Error>("Incorrect bass.dll version. Use the version that came with the mod.");
			return;
		}

		BASS_DEVICEINFO info;
		int defaultDevice = -1;
		for (int i = 0; BASS_GetDeviceInfo(i, &info); i++)
		{
			if (info.flags & BASS_DEVICE_DEFAULT) 
			{
				defaultDevice = i;
			}
		}
		if (!BASS_Init(defaultDevice, 44100, BASS_DEVICE_3D | BASS_DEVICE_DEFAULT, RsGlobal.ps->window, nullptr))
		{
			Log::Print<eLogLevel::Warn>("Failed to initialize BASS device. Audio glitches might occur");
		}

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
			ProcessGearSound(name, frame, data, pVeh);
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

