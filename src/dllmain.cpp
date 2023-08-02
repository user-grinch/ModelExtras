#include "pch.h"
#include "features/vehicle/brakes.h"
#include "features/vehicle/chain.h"
#include "features/vehicle/meter.h"
#include "features/vehicle/gear.h"
#include "features/vehicle/plate.h"
#include "features/weapon/bodystate.h"
#include "soundsystem.h"

static void ProcessNodesRecursive(RwFrame * frame, void* pEntity)
{
	if(frame)
	{
		const std::string name = GetFrameNodeName(frame);

		if (name[0] == 'x' && name[1] == '_')
		{
			// vehicle
			Chain.Process(frame, static_cast<CVehicle*>(pEntity));

			FrontBrake.Process(frame, static_cast<CVehicle*>(pEntity));
			RearBrake.Process(frame, static_cast<CVehicle*>(pEntity));

			GearMeter.Process(frame, static_cast<CVehicle*>(pEntity));
			OdoMeter.Process(frame, static_cast<CVehicle*>(pEntity));
			RpmMeter.Process(frame, static_cast<CVehicle*>(pEntity));
			SpeedMeter.Process(frame, static_cast<CVehicle*>(pEntity));

			Clutch.Process(frame, static_cast<CVehicle*>(pEntity));
			GearLever.Process(frame, static_cast<CVehicle*>(pEntity));
			GearSound.Process(frame, static_cast<CVehicle*>(pEntity));

			// weapon
			BodyState.Process(frame, static_cast<CWeapon*>(pEntity));
		}
		// LicensePlate.Process(frame, static_cast<CVehicle*>(pEntity));

		
		if (RwFrame * newFrame = frame->child)
		{
			ProcessNodesRecursive(newFrame, pEntity);
		}
		if (RwFrame * newFrame = frame->next)
		{
			ProcessNodesRecursive(newFrame, pEntity);
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

		Events::pedRenderEvent += [](CPed* pPed)
		{
			CWeapon *pWeapon = &pPed->m_aWeapons[pPed->m_nActiveWeaponSlot];
			if (pWeapon)
			{
				eWeaponType weaponType = pWeapon->m_eWeaponType;
				CWeaponInfo* pWeaponInfo = CWeaponInfo::GetWeaponInfo(weaponType, pPed->GetWeaponSkill(weaponType));
				if (pWeaponInfo)
				{
					CWeaponModelInfo* pWeaponModelInfo = static_cast<CWeaponModelInfo*>(CModelInfo::GetModelInfo(pWeaponInfo->m_nModelId1));
					if (pWeaponModelInfo)
					{
						ProcessNodesRecursive((RwFrame *)pWeaponModelInfo->m_pRwClump->object.parent, pWeapon);
					}
				}
			}
			
		};
	}
    return TRUE;
}

