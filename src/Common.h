#pragma once       
#include <fstream>
#include <Vector>
#include "plugin.h"
#include "Bass.h"

enum {
	FRAME_AT_ORIGIN,
	FRAME_MOVING,
	FRAME_AT_OFFSET,
};

struct FCData {

public:

	float delta = 0.0f;
	int realistic_speed = 0;
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
		float cal_value = 1.0f;
		float cur_rot = 0.0f;
		int rot_offset = 0;
		uint wait_time = 0;
		uint last_frame_ms = 0;
		short last_gear = -2;
	} clutch, gearlever;

	struct {
		bool init = false;
		uint current_gear = 0;
		HSTREAM upSound, downSound;
	} gearsound;

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
		bool init = false;
		float mul = 160.9f;
		float rot = 0.0f;
		float max_rot = 0.0f;
		int max_sp = 0;
	} spdometer;

	struct {
		bool init = false;
		float cur_rot = 0.0f;
		float max_rot = 0.0f;
		int max_rpm = 0;
	} rpmmeter;

	/*struct
	{
		bool init = false;
		int cur_rot = 0;
		CVector rot = CVector();
		uint wait_time = 0;
		uint last_frame_ms = 0;
	} throttle;*/


	FCData(CVehicle *vehicle){}
	~FCData(){}
};

struct HSL {
	float h, s, l;
};

struct RGB {
	float r, g, b;
};

std::string ExtractStringValue(const std::string& src_str, const std::string&& pattern, const std::string&& default_val);
void RotateFrameX(RwFrame* frame, float angle);
void RotateFrameY(RwFrame* frame, float angle);
void RotateFrameZ(RwFrame* frame, float angle);
void StoreChilds(RwFrame * parent_frame, std::vector<RwFrame*>& frame);
void ShowAllAtomics(RwFrame * frame);
void HideAllAtomics(RwFrame * frame);
void HideAllChilds(RwFrame *parent_frame);
void ShowAllChilds(RwFrame *parent_frame);
float GetVehicleSpeedRealistic(CVehicle * vehicle);
