#include "Chain.h"

void ProcessChain(const std::string& name,RwFrame* frame,FVCData& data,CVehicle* pVeh)
{
	if (name.find("fc_chain") != std::string::npos)
	{
		if (data.chain.cur_chain == -1)
		{
			StoreChilds(frame, data.chain.frames);
			data.chain.cur_chain = 0;
		}
		else
		{
			unsigned int delta_time = data.timer - data.chain.last_frame_ms;
			unsigned int wait_time = static_cast<unsigned int>(abs(data.realistic_speed)*0.01);

			if (delta_time > wait_time)
			{
				if (pVeh->m_nVehicleSubClass == VEHICLE_BMX)
				{
					// Only move chain forward when pedal is rotating
					if (pVeh->m_fGasPedal && data.realistic_speed > 0)
					{
						if (data.chain.cur_chain == 0)
							data.chain.cur_chain = static_cast<short>(data.chain.frames.size() - 1);
						else
							data.chain.cur_chain -= 1;
					}
				}
				else
				{
					if (data.realistic_speed > 0)
					{
						if (data.chain.cur_chain == 0)
							data.chain.cur_chain = static_cast<short>(data.chain.frames.size() - 1);
						else
							data.chain.cur_chain -= 1;
					}

					if (data.realistic_speed < 0)
					{
						if (data.chain.cur_chain == data.chain.frames.size() - 1)
							data.chain.cur_chain = 0;
						else
							data.chain.cur_chain += 1;
					}
				}
				HideAllChilds(frame);
				ShowAllAtomics(data.chain.frames[data.chain.cur_chain]);
				data.chain.last_frame_ms = data.timer;
			}
		}
	}
}