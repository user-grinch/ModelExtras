#include "Brakes.h"

void ProcessFrontBrake(const std::string& name, RwFrame* frame, FCData& data, CVehicle* pVeh)
{
	if (name.find("fc_fbrake") != std::string::npos)
	{
		if (!data.fbrake.init)
		{
			data.fbrake.init = true;

			data.fbrake.max_rot = std::stoi(ExtractStringValue(name, ".*_az(-?[0-9]+).*", "0"));
			data.fbrake.wait_time = static_cast<unsigned int>(abs(data.fbrake.max_rot / 5));
		}
		else
		{
			uint delta_time = (data.timer - data.fbrake.last_frame_ms);

			if (delta_time > data.fbrake.wait_time)
			{
				float temp;
				if (pVeh->m_nVehicleFlags.bIsHandbrakeOn && data.fbrake.cur_rot != data.fbrake.max_rot)
				{
					if (data.fbrake.max_rot < data.fbrake.cur_rot)
					{
						data.fbrake.cur_rot -= 1;
						temp = -1;
					}
					else
					{
						data.fbrake.cur_rot += 1;
						temp = 1;
					}

					RotateFrameZ(frame, temp);
					data.fbrake.last_frame_ms = data.timer;
				}
				else
				{
					if (data.fbrake.cur_rot != 0)
					{
						if (data.fbrake.max_rot > 0)
						{
							data.fbrake.cur_rot -= 1;
							temp = -1;
						}
						else
						{
							data.fbrake.cur_rot += 1;
							temp = 1;
						}

						RotateFrameZ(frame, temp);
						data.fbrake.last_frame_ms = data.timer;
					}
				}
			}
		}
	}
}

void ProcessRearBrake(const std::string& name, RwFrame* frame, FCData& data, CVehicle* pVeh)
{
	if (name.find("fc_rbrake") != std::string::npos)
	{
		if (!data.rbrake.init)
		{
			data.rbrake.init = true;

			data.rbrake.max_rot = std::stoi(ExtractStringValue(name, ".*_ax(-?[0-9]+).*", "0"));;
			data.rbrake.wait_time = static_cast<unsigned int>(abs(data.rbrake.max_rot / 5));
		}

		uint delta_time = (data.timer - data.rbrake.last_frame_ms);

		if (delta_time > data.rbrake.wait_time)
		{
			float temp;
			if (pVeh->m_fBreakPedal && data.rbrake.cur_rot != data.rbrake.max_rot)
			{
				if (data.rbrake.max_rot < data.rbrake.cur_rot)
				{
					data.rbrake.cur_rot -= 1;
					temp = -1;
				}
				else
				{
					data.rbrake.cur_rot += 1;
					temp = 1;
				}

				RotateFrameX(frame, temp);
				data.rbrake.last_frame_ms = data.timer;
			}
			else
			{
				if (data.rbrake.cur_rot != 0)
				{
					if (data.rbrake.max_rot > 0)
					{
						data.rbrake.cur_rot -= 1;
						temp = -1;
					}
					else
					{
						data.rbrake.cur_rot += 1;
						temp = 1;
					}

					RotateFrameX(frame, temp);
					data.rbrake.last_frame_ms = data.timer;
				}
			}
		}
	}
}

/*
void ProcessThrottle(const std::string& name, RwFrame* frame, FCData& data, CVehicle* pVeh, uint timer)
{
	if (name.find("fc_th") != std::string::npos)
	{
		if (!data.throttle.init)
		{
			data.throttle.init = true;

			data.throttle.rot.x = 0;//std::stoi(ExtractStringValue(name, ".*_x([0-9]+).*", "0"));
			data.throttle.rot.y = 30;// std::stoi(ExtractStringValue(name, ".*_y([0-9]+).*", "0"));
			data.throttle.rot.z = 0;//std::stoi(ExtractStringValue(name, ".*_z([0-9]+).*", "0"));

			data.throttle.wait_time = static_cast<unsigned int>(abs(data.throttle.rot.y / 5));
		}
		else
		{
			uint delta_time = (timer - data.throttle.last_frame_ms);

			if (delta_time > data.throttle.wait_time)
			{
				int temp;
				if (pVeh->m_fGasPedal && data.throttle.cur_rot != data.throttle.rot.y)
				{
					if (data.throttle.rot.y < data.throttle.cur_rot)
					{
						data.throttle.cur_rot -= 1;
						temp = -1;
					}
					else
					{
						data.throttle.cur_rot += 1;
						temp = 1;
					}
					RotateFrameX(frame, temp);
					RotateFrameY(frame, 0);
					RotateFrameZ(frame, 0);
					data.throttle.last_frame_ms = timer;
				}
				else
				{
					if (data.throttle.cur_rot != 0)
					{
						if (data.throttle.rot.y > 0)
						{
							data.throttle.cur_rot -= 1;
							temp = -1;
						}
						else
						{
							data.throttle.cur_rot += 1;
							temp = 1;
						}

						RotateFrameY(frame, temp);
						data.throttle.last_frame_ms = timer;
					}
				}
			}
		}
	}
}
*/