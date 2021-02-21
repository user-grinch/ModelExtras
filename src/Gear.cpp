#include "Gear.h"

void ProcessClutch(const std::string& name, RwFrame* frame, FCData& data, CVehicle* pVeh)
{
	if (name.find("fc_cl") != std::string::npos)
	{
		if (!data.clutch.init)
		{
			data.clutch.init = true;

			data.clutch.rot_offset = std::stoi(ExtractStringValue(name, ".*_az(-?[0-9]+).*", "0"));
			data.clutch.wait_time = static_cast<unsigned int>(abs(data.clutch.rot_offset / 10));
		}
		else
		{
			uint delta_time = (data.timer - data.clutch.last_frame_ms);

			if (delta_time > data.clutch.wait_time)
			{
				if (data.clutch.state == FRAME_AT_ORIGIN)
				{
					if (pVeh->m_nCurrentGear != data.clutch.last_gear)
					{
						data.clutch.last_gear = pVeh->m_nCurrentGear;
						data.clutch.state = FRAME_MOVING;
					}
				}
				else
				{
					if (data.clutch.state == FRAME_AT_OFFSET)
					{
						if (data.clutch.cur_rot == 0)
						{
							data.clutch.state = FRAME_AT_ORIGIN;
						}
						else
						{
							if (data.clutch.rot_offset > 0)
							{
								data.clutch.cur_rot -= 1;
								data.clutch.cal_value = -1;
							}
							else
							{
								data.clutch.cur_rot += 1;
								data.clutch.cal_value = 1;
							}

							RotateFrameZ(frame, data.clutch.cal_value);
							data.clutch.last_frame_ms = data.timer;
						}
					}
					else
					{
						if (data.clutch.cur_rot == data.clutch.rot_offset)
						{
							data.clutch.state = FRAME_AT_OFFSET;
						}
						else
						{
							if (data.clutch.rot_offset < data.clutch.cur_rot)
							{
								data.clutch.cur_rot -= 1;
								data.clutch.cal_value = -1;
							}
							else
							{
								data.clutch.cur_rot += 1;
								data.clutch.cal_value = 1;
							}

							RotateFrameZ(frame, data.clutch.cal_value);
							data.clutch.last_frame_ms = data.timer;
						}
					}
				}
			}
		}
	}
}

void ProcessGearLever(const std::string& name, RwFrame* frame, FCData& data, CVehicle* pVeh)
{
	if (name.find("fc_gl") != std::string::npos)
	{
		if (!data.gearlever.init)
		{
			data.gearlever.init = true;
			data.gearlever.rot_offset = std::stoi(ExtractStringValue(name, ".*_ax(-?[0-9]+).*", "0"));

			data.gearlever.wait_time = static_cast<unsigned int>(abs(data.gearlever.rot_offset / 10));
		}
		else
		{
			uint delta_time = (data.timer - data.gearlever.last_frame_ms);

			if (delta_time > data.gearlever.wait_time)
			{
				if (data.gearlever.state == FRAME_AT_ORIGIN)
				{
					if (pVeh->m_nCurrentGear != data.gearlever.last_gear)
					{
						if (pVeh->m_nCurrentGear > data.gearlever.last_gear)
							data.gearlever.cal_value = -1;
						else
							data.gearlever.cal_value = 1;

						data.gearlever.last_gear = pVeh->m_nCurrentGear;
						data.gearlever.state = FRAME_MOVING;
					}
				}
				else
				{

					if (data.gearlever.state == FRAME_AT_OFFSET)
					{
						if (data.gearlever.cur_rot != 0)
						{
							if (data.gearlever.cur_rot > 0)
							{
								data.gearlever.cur_rot -= 1;
								data.gearlever.cal_value = -1;
							}
							else
							{
								data.gearlever.cur_rot += 1;
								data.gearlever.cal_value = 1;
							}
							RotateFrameX(frame, data.gearlever.cal_value);
						}
						else
						{
							data.gearlever.state = FRAME_AT_ORIGIN;
						}
					}
					else
					{
						if (data.gearlever.rot_offset != abs(data.gearlever.cur_rot))
						{
							data.gearlever.cur_rot += data.gearlever.cal_value;
							RotateFrameX(frame, data.gearlever.cal_value);
						}
						else
						{
							data.gearlever.state = FRAME_AT_OFFSET;
						}
					}

					data.gearlever.last_frame_ms = data.timer;
				}
			}
		}
	}
}
