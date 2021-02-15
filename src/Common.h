#pragma once       
#include <fstream>
#include <Vector>
#include "plugin.h"


enum {
	FRAME_AT_ORIGIN,
	FRAME_MOVING,
	FRAME_AT_OFFSET,
};

struct FVCData {

public:

	int realistic_speed;
	uint timer = 0;

	struct
	{
		std::vector<RwFrame*> frames;
		short cur_chain = -1;
		uint last_frame_ms = 0;
	} chain;

	struct
	{
		bool init = false;
		int cur_rot = 0;
		int max_rot = 0;
		uint wait_time = 0;
		uint last_frame_ms = 0;
	} fbrake, rbrake;

	struct
	{
		bool init = false;
		int state = FRAME_AT_ORIGIN;
		int cal_value = 1;
		int cur_rot = 0;
		int rot_offset = 0;
		uint wait_time = 0;
		uint last_frame_ms = 0;
		short last_gear = -2;
	} clutch, gearlever;

	struct {
		int gear_shown = -2;
		std::vector<RwFrame*> frames;
	} gearmeter;

	struct {
		bool init = false;
		int bac_val = 0;
		std::string val_shown = "000000";
		std::vector<RwFrame*> frames;
		float mul = 160.9f;
	} odometer;

	struct {

	} spdometer;


	/*struct
	{
		bool init = false;
		int cur_rot = 0;
		CVector rot = CVector();
		uint wait_time = 0;
		uint last_frame_ms = 0;
	} throttle;*/


	FVCData(CVehicle *vehicle){}
	~FVCData(){}
};

struct HSL {
	float h, s, l;
};

struct RGB {
	float r, g, b;
};

extern std::fstream lg;

std::string ExtractStringValue(const std::string& src_str, const std::string&& pattern, const std::string&& default_val);
void RotateFrameX(RwFrame* frame, int angle);
void RotateFrameY(RwFrame* frame, int angle);
void RotateFrameZ(RwFrame* frame, int angle);
void StoreChilds(RwFrame * parent_frame, std::vector<RwFrame*>& frame);
void ShowAllAtomics(RwFrame * frame);
void HideAllAtomics(RwFrame * frame);
void HideAllChilds(RwFrame *parent_frame);
void ShowAllChilds(RwFrame *parent_frame);
float GetVehicleSpeedRealistic(CVehicle * vehicle);
