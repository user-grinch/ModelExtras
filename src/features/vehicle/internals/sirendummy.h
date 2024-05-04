#pragma once
#include <string>
#include "materials.h"
#include "game_sa/CGeneral.h"
#include "game_sa/RenderWare.h"

class VehicleSirenDummy {
	public:
		RwFrame* Frame;
		RwRGBA Color = { 255, 255, 255, 128 };
		CVector Position;
		int Type;
		float Size;
		float Angle;
		float CurrentAngle = 0.0f;

		VehicleSirenDummy(RwFrame* frame, std::string name, int start, bool parent, int type = 0, RwRGBA color = { 255, 255, 255, 128 });

		CVector GetPosition();

		void ResetAngle() {
			if (CurrentAngle == 0.0f)
				return;

			ReduceAngle(CurrentAngle);
		};

		void AddAngle(float angle) {
			if (angle == 0.0f)
				return;

			RwFrameRotate(Frame, (RwV3d*)0x008D2E18, angle, rwCOMBINEPRECONCAT);
			
			CurrentAngle += angle;
		};

		void ReduceAngle(float angle) {
			if (angle == 0.0f)
				return;

			RwFrameRotate(Frame, (RwV3d*)0x008D2E18, -angle, rwCOMBINEPRECONCAT);

			CurrentAngle -= angle;
		};

		void SetAngle(float angle) {
			ResetAngle();

			AddAngle(angle);
		}

	private:
		static int ReadHex(char a, char b);

		bool hasParent = false;
};