#include "pch.h"
#include "Gear.h"
#include "bass.h"
#include "../../soundsystem.h"

ClutchFeature Clutch;
void ClutchFeature::Initialize(RwFrame* pFrame, CVehicle* pVeh) {
	VehData &data = vehData.Get(pVeh);
	std::string name = GetFrameNodeName(pFrame);
	data.m_nCurOffset = std::stoi(Util::GetRegexVal(name, ".*_(-?[0-9]+).*", "0"));
	data.m_nWaitTime = static_cast<unsigned int>(abs(data.m_nCurOffset / 10));
	
}

void ClutchFeature::Process(RwFrame* frame, CVehicle* pVeh)
{
	VehData &data = vehData.Get(pVeh);
	std::string name = GetFrameNodeName(frame);
	if (name.find("x_clutch") != std::string::npos)
	{
		if (!data.m_bInitialized)
		{
			Initialize(frame, pVeh);
			data.m_bInitialized = true;
		}
		uint timer = CTimer::m_snTimeInMilliseconds;
		uint deltaTime = (timer - data.m_nLastFrameMS);

		if (deltaTime > data.m_nWaitTime)
		{
			if (data.m_eState == eFrameState::AtOrigin)
			{
				if (pVeh->m_nCurrentGear != data.m_nLastGear)
				{
					data.m_nLastGear = pVeh->m_nCurrentGear;
					data.m_eState = eFrameState::IsMoving;
				}
			}
			else
			{
				if (data.m_eState == eFrameState::AtOffset)
				{
					if (data.m_fCurRotation == 0)
					{
						data.m_eState = eFrameState::AtOrigin;
					}
					else
					{
						if (data.m_nCurOffset > 0)
						{
							data.m_fCurRotation -= 1;
							data.m_fCalVal = -1;
						}
						else
						{
							data.m_fCurRotation += 1;
							data.m_fCalVal = 1;
						}

						Util::RotateFrameZ(frame, data.m_fCalVal);
						data.m_nLastFrameMS = timer;
					}
				}
				else
				{
					if (data.m_fCurRotation == data.m_nCurOffset)
					{
						data.m_eState = eFrameState::AtOffset;
					}
					else
					{
						if (data.m_nCurOffset < data.m_fCurRotation)
						{
							data.m_fCurRotation -= 1;
							data.m_fCalVal = -1;
						}
						else
						{
							data.m_fCurRotation += 1;
							data.m_fCalVal = 1;
						}

						Util::RotateFrameZ(frame, data.m_fCalVal);
						data.m_nLastFrameMS = timer;
					}
				}
			}
		}
	}
}

GearLeverFeature GearLever;
void GearLeverFeature::Initialize(RwFrame* pFrame, CVehicle* pVeh) {
	VehData &data = vehData.Get(pVeh);
	std::string name = GetFrameNodeName(pFrame);
	data.m_nCurOffset = std::stoi(Util::GetRegexVal(name, ".*_(-?[0-9]+).*", "0"));
	data.m_nWaitTime = static_cast<unsigned int>(abs(data.m_nCurOffset / 10));
	
}

void GearLeverFeature::Process(RwFrame* frame, CVehicle* pVeh)
{
	VehData &data = vehData.Get(pVeh);
	std::string name = GetFrameNodeName(frame);
	if (name.find("x_gearlever") != std::string::npos)
	{
		if (!data.m_bInitialized)
		{
			Initialize(frame, pVeh);
			data.m_bInitialized = true;
		}
		uint timer = CTimer::m_snTimeInMilliseconds;	
		uint deltaTime = (timer - data.m_nLastFrameMS);

		if (deltaTime > data.m_nWaitTime)
		{
			if (data.m_eState == eFrameState::AtOrigin)
			{
				if (pVeh->m_nCurrentGear != data.m_nLastGear)
				{
					data.m_fCalVal = (pVeh->m_nCurrentGear > data.m_nLastGear) ? -1 : 1;
					data.m_nLastGear = pVeh->m_nCurrentGear;
					data.m_eState = eFrameState::IsMoving;
				}
			}
			else
			{
				if (data.m_eState == eFrameState::AtOffset)
				{
					if (data.m_fCurRotation != 0)
					{
						if (data.m_fCurRotation > 0)
						{
							data.m_fCurRotation -= 1;
							data.m_fCalVal = -1;
						}
						else
						{
							data.m_fCurRotation += 1;
							data.m_fCalVal = 1;
						}
						Util::RotateFrameX(frame, data.m_fCalVal);
					}
					else
					{
						data.m_eState = eFrameState::AtOrigin;
					}
				}
				else
				{
					if (data.m_nCurOffset != abs(data.m_fCurRotation))
					{
						data.m_fCurRotation += data.m_fCalVal;
						Util::RotateFrameX(frame, data.m_fCalVal);
					}
					else
					{
						data.m_eState = eFrameState::AtOffset;
					}
				}

				data.m_nLastFrameMS = timer;
			}
		}
	}
}

GearSoundFeature GearSound;

void GearSoundFeature::Initialize(RwFrame* pFrame, CVehicle* pVeh) {
	VehData &data = vehData.Get(pVeh);
	std::string name = GetFrameNodeName(pFrame);
	std::string regex = Util::GetRegexVal(name, "x_gs_(.*$)", "");
	std::string upPath = MOD_DATA_PATH_S(std::format("audio/{}.wav", regex));

	data.m_pUpAudio = SoundSystem.LoadStream(upPath.c_str(), false);
	data.m_pUpAudio->SetVolume(0.5f);
}

void GearSoundFeature::Process(RwFrame* frame, CVehicle* pVeh)
{
	std::string name = GetFrameNodeName(frame);
	if (name.find("x_gs") != std::string::npos)
	{
		VehData &data = vehData.Get(pVeh);
		if (!data.m_bInitialized)
		{
			Initialize(frame, pVeh);
			data.m_bInitialized = true;
		}
		if (data.m_nCurGear != pVeh->m_nCurrentGear)
		{	
			data.m_pUpAudio->Play();
			data.m_nCurGear = pVeh->m_nCurrentGear;
		}
	}
}