#include "Meter.h"
#include "CBike.h"
#include "CAutomobile.h"

void ProcessGearMeter(const std::string& name, RwFrame* frame, FVCData& data, CVehicle* pVeh)
{
	if (name.find("fc_gm") != std::string::npos)
	{
		if (data.gearmeter.gear_shown == -2)
		{
			StoreChilds(frame, data.gearmeter.frames);
			data.gearmeter.gear_shown = 0;
		}
		else
		{

			if (pVeh->m_nCurrentGear != data.gearmeter.gear_shown)
			{
				data.gearmeter.gear_shown = pVeh->m_nCurrentGear;

				HideAllChilds(frame);
				if (data.gearmeter.frames.size() > static_cast<size_t>(data.gearmeter.gear_shown))
					ShowAllAtomics(data.gearmeter.frames[data.gearmeter.gear_shown]);
			}
		}
	}
}

void ProcessOdoMeter(const std::string& name, RwFrame* frame, FVCData& data, CVehicle* pVeh)
{
	if (name.find("fc_om") != std::string::npos)
	{
		if (!data.odometer.init)
		{
			StoreChilds(frame, data.odometer.frames);

			data.odometer.init = true;
			data.odometer.bac_val = 1234 + rand() % (57842 - 1234);

			if (name.find("_kph") != std::string::npos)
			{
				data.odometer.mul = 100;
			}
		}
		else
		{

			// Calculate new value
			int rot = 0;
			if (pVeh->m_nVehicleSubClass == VEHICLE_BIKE)
			{
				CBike *bike = (CBike*)pVeh;
				rot = static_cast<int>(bike->m_afWheelRotationX[1]);
			}
			else
			{
				CAutomobile *am = (CAutomobile*)pVeh;
				rot = static_cast<int>(am->m_fWheelRotation[3]);
			}
			
			int rot_val = static_cast<int>((rot / (2.86* data.odometer.mul)));
			int val = std::stoi(data.odometer.val_shown) + abs(data.odometer.bac_val - rot_val);
			data.odometer.bac_val = rot_val;

			if (val < 999999)
			{
				std::string show_str = std::to_string(val);

				// 1 -> 000001
				while (show_str.size() < 6)
				{
					show_str = "0" + show_str;
				}

				if (data.odometer.val_shown != show_str)
				{
					// Update odometer value
					for (unsigned int i = 0; i < 6; i++)
					{
						if (show_str[i] != data.odometer.val_shown[i])
						{
							int angle = (std::stoi(std::to_string(show_str[i])) - std::stoi(std::to_string(data.odometer.val_shown[i]))) * 36;
							RotateFrameX(data.odometer.frames[i], angle);
						}
					}
					data.odometer.val_shown = show_str;
				}
			}
		}
	}
}

void ProcessRPMMeter(const std::string& name, RwFrame* frame, FVCData& data, CVehicle* pVeh)
{
}

void ProcessSpeedoMeter(const std::string& name, RwFrame* frame, FVCData& data, CVehicle* pVeh)
{
	if (name.find("fc_sm") != std::string::npos)
	{
		lg << name << std::endl;
		RotateFrameY(frame, 1);
	}
}