#include "pch.h"
#include "features/vehicle/brakes.h"
#include "features/vehicle/chain.h"
#include "features/vehicle/meter.h"
#include "features/vehicle/gear.h"
#include "features/vehicle/plate.h"
#include "features/weapon/bodystate.h"
#include "soundsystem.h"

static ThiscallEvent <AddressList<0x5E7859, H_CALL>, PRIORITY_BEFORE, ArgPickN<CPed*, 0>, void(CPed*)> weaponRenderEvent;

enum class eNodeEntityType {
	Ped,
	Object,
	Vehicle,
	Weapon,
};

static void ProcessNodesRecursive(RwFrame * frame, void* pEntity, eNodeEntityType type)
{
	if(frame)
	{
		const std::string name = GetFrameNodeName(frame);

		if (name[0] == 'x' && name[1] == '_')
		{
			if (type == eNodeEntityType::Vehicle) {
				CVehicle *pVeh = static_cast<CVehicle*>(pEntity);
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
			} else if (type == eNodeEntityType::Weapon) {
				CWeapon *pWep = static_cast<CWeapon*>(pEntity);
				BodyState.Process(frame, pWep);
			}
		}
		// LicensePlate.Process(frame, pVeh);

		if (RwFrame * newFrame = frame->child)
		{
			ProcessNodesRecursive(newFrame, pEntity, type);
		}
		if (RwFrame * newFrame = frame->next)
		{
			ProcessNodesRecursive(newFrame, pEntity, type);
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
			ProcessNodesRecursive((RwFrame *)pVeh->m_pRwClump->object.parent, pVeh, eNodeEntityType::Vehicle);
		};

		Events::pedRenderEvent += [](CPed* pPed)
		{
			// peds
			ProcessNodesRecursive((RwFrame *)pPed->m_pRwClump->object.parent, pPed, eNodeEntityType::Ped);

			// jetpack
			CTaskSimpleJetPack *pTask = pPed->m_pIntelligence->GetTaskJetPack();
			if (pTask && pTask->m_pJetPackClump) {
				ProcessNodesRecursive((RwFrame *)pTask->m_pJetPackClump->object.parent, pPed, eNodeEntityType::Weapon);
			}

			// weapons
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
						ProcessNodesRecursive((RwFrame *)pPed->m_pRwClump->object.parent, pPed, eNodeEntityType::Weapon);
					}
				}
			}
			
		};
	}
    return TRUE;
}

