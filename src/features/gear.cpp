#include "pch.h"
#include "Gear.h"
#include "bass.h"

ClutchFeature Clutch;
void ClutchFeature::Initialize(RwFrame* pFrame, CVehicle* pVeh) {
	VehData data = vehData.Get(pVeh);
	std::string name = GetFrameNodeName(pFrame);
	data.m_nCurOffset = std::stoi(Util::GetRegexVal(name, ".*_az(-?[0-9]+).*", "0"));
	data.m_nWaitTime = static_cast<unsigned int>(abs(data.m_nCurOffset / 10));
	IFeature::Initialize();
}

void ClutchFeature::Process(RwFrame* frame, CVehicle* pVeh)
{
	VehData data = vehData.Get(pVeh);
	std::string name = GetFrameNodeName(frame);
	if (name.find("fc_cl") != std::string::npos)
	{
		if (m_State == eFeatureState::NotInitialized)
		{
			Initialize(frame, pVeh);
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
	VehData data = vehData.Get(pVeh);
	std::string name = GetFrameNodeName(pFrame);
	data.m_nCurOffset = std::stoi(Util::GetRegexVal(name, ".*_ax(-?[0-9]+).*", "0"));
	data.m_nWaitTime = static_cast<unsigned int>(abs(data.m_nCurOffset / 10));
	IFeature::Initialize();
}

void GearLeverFeature::Process(RwFrame* frame, CVehicle* pVeh)
{
	VehData data = vehData.Get(pVeh);
	std::string name = GetFrameNodeName(frame);
	if (name.find("fc_gl") != std::string::npos)
	{
		if (m_State == eFeatureState::NotInitialized)
		{
			Initialize(frame, pVeh);
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
	VehData data = vehData.Get(pVeh);
	std::string name = GetFrameNodeName(pFrame);
	std::string upSoundPath = "audio/" + Util::GetRegexVal(name, "fc_gs_u_(.*$)", "") + ".wav";
	data.m_hUpSound = BASS_StreamCreateFile(false, 
		MOD_DATA_PATH_S(upSoundPath), NULL, NULL, NULL);
	
	int code = BASS_ErrorGetCode();
	if (code != BASS_OK)
	{
		Log::Print<eLogLevel::Warn>("Failed to create BASS audio stream. Error Code: {}", code);
	}

	// BASS_ChannelSetAttribute(data.m_hUpSound, BASS_ATTRIB_VOL, 1.0);

	std::string downSoundPath = "audio/" + Util::GetRegexVal(name, "fc_gs_d_(.*$)", "") + ".wav";
	data.m_hDownSound = BASS_StreamCreateFile(false, 
		MOD_DATA_PATH_S(downSoundPath), NULL, NULL, NULL);
	
	code = BASS_ErrorGetCode();
	if (code != BASS_OK)
	{
		Log::Print<eLogLevel::Warn>("Failed to create BASS audio stream. Error Code: {}", code);
	}

	// BASS_ChannelSetAttribute(data.m_hDownSound, BASS_ATTRIB_VOL, 1.0);
	IFeature::Initialize();
}

void GearSoundFeature::Process(RwFrame* frame, CVehicle* pVeh)
{
	VehData data = vehData.Get(pVeh);
	std::string name = GetFrameNodeName(frame);
	if (name.find("fc_gs") != std::string::npos)
	{
		if (m_State == eFeatureState::NotInitialized)
		{
			Initialize(frame, pVeh);
		}
		if (data.m_nCurGear != pVeh->m_nCurrentGear)
		{	
			// Gear Up sound
			if (data.m_nCurGear < pVeh->m_nCurrentGear)
			{
				BASS_ChannelPlay(data.m_hUpSound, true);
				int code = BASS_ErrorGetCode();
				if (code != BASS_OK)
				{
					Log::Print<eLogLevel::Warn>("Failed to play BASS audio stream. Error Code: {}", code);
				}
			}
			else // Gear down sound
			{
				BASS_ChannelPlay(data.m_hDownSound, true);
				int code = BASS_ErrorGetCode();
				if (code != BASS_OK)
				{
					Log::Print<eLogLevel::Warn>("Failed to play BASS audio stream. Error Code: {}", code);
				}
			}
			data.m_nCurGear = pVeh->m_nCurrentGear;
		}
	}
}