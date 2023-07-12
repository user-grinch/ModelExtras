#include "pch.h"
#include "features/brakes.h"
#include "features/chain.h"
#include "features/meter.h"
#include "features/gear.h"

static void ProcessNodesRecursive(RwFrame * frame, CVehicle* pVeh)
{
	if(frame)
	{
		const std::string name = GetFrameNodeName(frame);

		if (name[0] == 'f' && name[1] == 'c' && name[2] == '_')
		{
			Chain.Process(frame, pVeh);

			FrontBrake.Process(frame, pVeh);
			RearBrake.Process(frame, pVeh);

			GearMeter.Process(frame, pVeh);
			OdoMeter.Process(frame, pVeh);
			RpmMeter.Process(frame, pVeh);
			SpeedMeter.Process(frame, pVeh);

			Clutch.Process(frame, pVeh);
			GearLever.Process(frame, pVeh);
			GearSound.Process(frame, pVeh);
		}
		
		if (RwFrame * newFrame = frame->child)
		{
			ProcessNodesRecursive(newFrame, pVeh);
		}
		if (RwFrame * newFrame = frame->next)
		{
			ProcessNodesRecursive(newFrame, pVeh);
		}
	}
	return;
}

BOOL WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID Reserved)
{
    if (nReason == DLL_PROCESS_ATTACH)
    {
		Events::initGameEvent += []()
		{
			Log::Print<eLogLevel::None>("Starting " MOD_TITLE " (" __DATE__ ")\nAuthor: Grinch_\nDiscord: "
                                    DISCORD_INVITE "\nPatreon: " PATREON_LINK "\nMore Info: " GITHUB_LINK "\n");
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
		};

		Events::vehicleRenderEvent += [](CVehicle* pVeh)
		{
			ProcessNodesRecursive((RwFrame *)pVeh->m_pRwClump->object.parent, pVeh);
		};
	}
    return TRUE;
}

