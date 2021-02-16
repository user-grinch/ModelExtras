#include "pch.h"

std::string ExtractStringValue(const std::string& src_str, const std::string&& pattern, const std::string&& default_val)
{
	std::smatch match;
	std::regex_search(src_str.begin(), src_str.end(), match, std::regex(pattern));

	if (match.empty())
		return default_val;
	else
		return match[1];

}

void RotateFrameX(RwFrame* frame, float angle)
{
	RwFrameRotate(frame, (RwV3d *)0x008D2E00, (RwReal)angle, rwCOMBINEPRECONCAT);
	RwFrameUpdateObjects(frame);
}

void RotateFrameY(RwFrame* frame, float angle)
{
	RwFrameRotate(frame, (RwV3d *)0x008D2E0C, (RwReal)angle, rwCOMBINEPRECONCAT);
	RwFrameUpdateObjects(frame);
}

void RotateFrameZ(RwFrame* frame, float angle)
{
	RwFrameRotate(frame, (RwV3d *)0x008D2E18, (RwReal)angle, rwCOMBINEPRECONCAT);
	RwFrameUpdateObjects(frame);
}

void StoreChilds(RwFrame * parent_frame, std::vector<RwFrame*>& frame)
{
	RwFrame* child = parent_frame->child;
	while (child)
	{
		const std::string name = GetFrameNodeName(child);

		frame.push_back(child);
		child = child->next;
	}
}


void ShowAllAtomics(RwFrame * frame)
{
	if (!rwLinkListEmpty(&frame->objectList))
	{
		RwObjectHasFrame * atomic;

		RwLLLink * current = rwLinkListGetFirstLLLink(&frame->objectList);
		RwLLLink * end = rwLinkListGetTerminator(&frame->objectList);

		while (current != end) {
			atomic = rwLLLinkGetData(current, RwObjectHasFrame, lFrame);
			atomic->object.flags |= rpATOMICRENDER; // clear

			current = rwLLLinkGetNext(current);
		}
	}
	return;
}

void HideAllAtomics(RwFrame * frame)
{
	if (!rwLinkListEmpty(&frame->objectList))
	{
		RwObjectHasFrame * atomic;

		RwLLLink * current = rwLinkListGetFirstLLLink(&frame->objectList);
		RwLLLink * end = rwLinkListGetTerminator(&frame->objectList);

		while (current != end) {
			atomic = rwLLLinkGetData(current, RwObjectHasFrame, lFrame);
			atomic->object.flags &= ~rpATOMICRENDER;

			current = rwLLLinkGetNext(current);
		}
	}
	return;
}

void HideAllChilds(RwFrame *parent_frame)
{
	RwFrame* child = parent_frame->child;
	while (child)
	{
		HideAllAtomics(child);
		child = child->next;
	}
}

void ShowAllChilds(RwFrame *parent_frame)
{
	RwFrame* child = parent_frame->child;
	while (child)
	{
		ShowAllAtomics(child);
		child = child->next;
	}
}

// Taken from vehfuncs
float GetVehicleSpeedRealistic(CVehicle * vehicle)
{
	float wheelSpeed = 0.0;
	CVehicleModelInfo * vehicleModelInfo = (CVehicleModelInfo *)CModelInfo::GetModelInfo(vehicle->m_nModelIndex);
	if (vehicle->m_nVehicleSubClass == VEHICLE_BIKE || vehicle->m_nVehicleSubClass == VEHICLE_BMX)
	{
		CBike * bike = (CBike *)vehicle;
		wheelSpeed = ((bike->m_fWheelSpeed[0] * vehicleModelInfo->m_fWheelSizeFront) +
			(bike->m_fWheelSpeed[1] * vehicleModelInfo->m_fWheelSizeRear)) / 2.0f;
	}
	else if (vehicle->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || vehicle->m_nVehicleSubClass == VEHICLE_MTRUCK || vehicle->m_nVehicleSubClass == VEHICLE_QUAD)
	{
		CAutomobile * automobile = (CAutomobile *)vehicle;
		wheelSpeed = ((automobile->m_fWheelSpeed[0] + automobile->m_fWheelSpeed[1] * vehicleModelInfo->m_fWheelSizeFront) +
			(automobile->m_fWheelSpeed[2] + automobile->m_fWheelSpeed[3] * vehicleModelInfo->m_fWheelSizeRear)) / 4.0f;
	}
	else
	{
		return (vehicle->m_vecMoveSpeed.Magnitude() * 50.0f) * 3.6f;
	}
	wheelSpeed /= 2.45f; // tweak based on distance (manually testing)
	wheelSpeed *= -186.0f; // tweak based on km/h

	return wheelSpeed;
}

HSL RGBtoHSL(const CRGBA& color)
{
	HSL rtn;

	// 0-255 -> 0-1
	float r = color.r/255.0f;
	float g = color.g/255.0f;
	float b = color.b/255.0f;

	// calculate the min & max of those 3 values
	float min = (r > g) ? ((g > b) ? b : g) : ((r > b) ? b : r);
	float max = (r < g) ? ((g < b) ? b : g) : ((r < b) ? b : r);

	rtn.l = (min+max)/2;

	// calculate saturation
	if (min == max) 
		rtn.s = 0;
	else
	{
		if (rtn.l <= 0.5f)
			rtn.s = (max-min)/(max+min);
		else
			rtn.s = (max-min)/(2.0f-max-min);
	}

	// calculate hue
	if (r == max)
		rtn.h = (g-b)/(max-min);

	if (g == max)
		rtn.h = 2.0f + (b-r)/(max-min);

	if (b == max)
		rtn.h = 4.0f + (r-g)/(max-min);

	rtn.h *= 60; // convert to degrees on the color circle
	
	if (rtn.h < 0)
		rtn.h += 360;

	return rtn;
}

RGB HSLtoRGB(HSL& color)
{
	if (color.s == 0) // shade of gray
		return {color.l*255, color.l*255, color.l*255};
	
	float temp_1, temp_2;
	if (color.l < 0.5)
		temp_1 = color.l * (1.0f + color.s);
	else
		temp_1 = color.l + color.s - (color.l*color.s);

	temp_2 = 2 * color.l - temp_1;
	
	float hue = color.h / 360; // circle angle -> 0-1

	// convert rgb 0-1
	float temp_r, temp_g, temp_b;
	temp_r = color.h + 0.333f;
	temp_g = color.h;
	temp_b = color.h - 0.333f;

	// negate 1 if val > 1
	temp_r = (temp_r > 1) ? temp_r-1 : temp_r;
	temp_g = (temp_g > 1) ? temp_g-1 : temp_g;
	temp_b = (temp_b > 1) ? temp_b-1 : temp_b;

	// add 1 if val < 0
	temp_r = (temp_r < 1) ? temp_r+1 : temp_r;
	temp_g = (temp_g < 1) ? temp_g+1 : temp_g;
	temp_b = (temp_b < 1) ? temp_b+1 : temp_b;

	// red
	if (6*temp_r < 1)
		temp_r = temp_2 + (temp_1 - temp_2)* 6 * temp_r;

	if (2*temp_r < 1)
		temp_r = temp_1;

	if (3*temp_r < 2)
		temp_r = temp_2 + (temp_1 - temp_2) * (0.666f - temp_r) * 6;

	temp_r = temp_2;

	// green
	if (6*temp_g < 1)
		temp_g = temp_2 + (temp_1 - temp_2)* 6 * temp_g;

	if (2*temp_g < 1)
		temp_g = temp_1;

	if (3*temp_g < 2)
		temp_g = temp_2 + (temp_1 - temp_2) * (0.666f - temp_g) * 6;

	temp_g = temp_2;

	// blue
	if (6*temp_b < 1)
		temp_b = temp_2 + (temp_1 - temp_2)* 6 * temp_b;

	if (2*temp_b < 1)
		temp_b = temp_1;

	if (3*temp_b < 2)
		temp_b = temp_2 + (temp_1 - temp_2) * (0.666f - temp_b) * 6;

	temp_b = temp_2;

	// 0-1 -> 0-255
	return {temp_r*255, temp_g*255, temp_b*255};
}