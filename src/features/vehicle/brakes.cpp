#include "pch.h"
#include "Brakes.h"

FrontBrakeFeature FrontBrake;
RearBrakeFeature RearBrake;

void FrontBrakeFeature::Initialize(RwFrame* pFrame, CVehicle* pVeh) {
	VehData &data = vehData.Get(pVeh);
	std::string name = GetFrameNodeName(pFrame);
	data.m_nMaxRotation = std::stoi(Util::GetRegexVal(name, ".*_az(-?[0-9]+).*", "0"));
	data.m_nWaitTime = static_cast<unsigned int>(abs(data.m_nMaxRotation / 5));
	
}

void RearBrakeFeature::Initialize(RwFrame* pFrame, CVehicle* pVeh) {
	VehData &data = vehData.Get(pVeh);
	std::string name = GetFrameNodeName(pFrame);
	data.m_nMaxRotation = std::stoi(Util::GetRegexVal(name, ".*_ax(-?[0-9]+).*", "0"));
	data.m_nWaitTime = static_cast<unsigned int>(abs(data.m_nMaxRotation / 5));
	
}

void FrontBrakeFeature::Process(RwFrame* frame, CVehicle* pVeh) {
	std::string name = GetFrameNodeName(frame);
	if (name.find("x_fbrake") != std::string::npos)
	{
		VehData &data = vehData.Get(pVeh);

		if (!data.m_bInitialized)
		{
			Initialize(frame, pVeh);
			data.m_bInitialized = true;
		}

		uint timer = CTimer::m_snTimeInMilliseconds * CTimer::ms_fTimeScale;;
		uint deltaTime = (timer - data.m_nLastFrameMS);

		if (deltaTime > data.m_nWaitTime)
		{
			float temp;
			if (pVeh->m_nVehicleFlags.bIsHandbrakeOn && data.m_nCurRotation != data.m_nMaxRotation)
			{
				if (data.m_nMaxRotation < data.m_nCurRotation)
				{
					data.m_nCurRotation -= 1;
					temp = -1;
				}
				else
				{
					data.m_nCurRotation += 1;
					temp = 1;
				}

				Util::RotateFrameZ(frame, temp);
				data.m_nLastFrameMS = timer;
			}
			else
			{
				if (data.m_nCurRotation != 0)
				{
					if (data.m_nMaxRotation > 0)
					{
						data.m_nCurRotation -= 1;
						temp = -1;
					}
					else
					{
						data.m_nCurRotation += 1;
						temp = 1;
					}

					Util::RotateFrameZ(frame, temp);
					data.m_nLastFrameMS = timer;
				}
			}
		}
	}

	IFeature::Process(frame, pVeh);
}

void RearBrakeFeature::Process(RwFrame* frame, CVehicle* pVeh) {
	std::string name = GetFrameNodeName(frame);
	if (name.find("x_rbrake") != std::string::npos)
	{
		VehData &data = vehData.Get(pVeh);

		if (!data.m_bInitialized)
		{
			Initialize(frame, pVeh);
			data.m_bInitialized = true;
		}

		uint timer = CTimer::m_snTimeInMilliseconds * CTimer::ms_fTimeScale;;
		uint deltaTime = (timer - data.m_nLastFrameMS);

		if (deltaTime > data.m_nWaitTime)
		{
			float temp;
			if (pVeh->m_fBreakPedal && data.m_nCurRotation != data.m_nMaxRotation)
			{
				if (data.m_nMaxRotation < data.m_nCurRotation)
				{
					data.m_nCurRotation -= 1;
					temp = -1;
				}
				else
				{
					data.m_nCurRotation += 1;
					temp = 1;
				}

				Util::RotateFrameX(frame, temp);
				data.m_nLastFrameMS = timer;
			}
			else
			{
				if (data.m_nCurRotation != 0)
				{
					if (data.m_nMaxRotation > 0)
					{
						data.m_nCurRotation -= 1;
						temp = -1;
					}
					else
					{
						data.m_nCurRotation += 1;
						temp = 1;
					}

					Util::RotateFrameX(frame, temp);
					data.m_nLastFrameMS = timer;
				}
			}
		}
	}
	IFeature::Process(frame, pVeh);
}