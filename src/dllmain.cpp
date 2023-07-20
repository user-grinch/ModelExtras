#include "pch.h"
#include "features/brakes.h"
#include "features/chain.h"
#include "features/meter.h"
#include "features/gear.h"
#include "features/plate.h"
#include "soundsystem.h"

static void ProcessNodesRecursive(RwFrame * frame, CVehicle* pVeh)
{
	if(frame)
	{
		const std::string name = GetFrameNodeName(frame);

		if (name[0] == 'v' && name[1] == 'x' && name[2] == '_')
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
		LicensePlate.Process(frame, pVeh);

		
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
			SoundSystem.Inject();
			SoundSystem.Init(RsGlobal.ps->window);
		};

		Events::vehicleRenderEvent += [](CVehicle* pVeh)
		{
			ProcessNodesRecursive((RwFrame *)pVeh->m_pRwClump->object.parent, pVeh);
		};
	}
    return TRUE;
}

