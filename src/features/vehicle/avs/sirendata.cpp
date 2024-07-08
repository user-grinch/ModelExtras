#include "pch.h"
#include "sirendata.h"
#include "common.h"

using json = nlohmann::json;

VehicleSirenData::VehicleSirenData(json data, CVehicle* vehicle, std::string key) {
	Key = key;

	JsonData = data;

	State = (int)data.find("state").value();

	PatternCount = 0;

	Time = Common::TimeSinceEpochMillisec();

	int index = 0;

	json patternData = data.find("pattern").value();

	Size = data.find("size").value();

	Color.red = (RwUInt8)data.find("color").value().find("red").value();
	Color.green = (RwUInt8)data.find("color").value().find("green").value();
	Color.blue = (RwUInt8)data.find("color").value().find("blue").value();
	Color.alpha = (RwUInt8)data.find("color").value().find("alpha").value();

	for (json::iterator element = patternData.begin(); element != patternData.end(); ++element) {
		Pattern.push_back(element.value());
	}

	RwFrame* rootFrame = (RwFrame*)vehicle->m_pRwClump->object.parent;
	FindNodesRecursive(rootFrame, vehicle, false, false);

	ResetSirenData();
};

void VehicleSirenData::ResetSirenData() {
	State = (int)JsonData.find("state").value();

	Time = Common::TimeSinceEpochMillisec();

	PatternCount = 0;
};

void VehicleSirenData::FindNodesRecursive(RwFrame* frame, CVehicle* vehicle, bool bReSearch, bool bOnExtras) {
	while (frame) {
		const std::string name = GetFrameNodeName(frame);

		if (name.rfind("siren" + Key, 0) != 0) {
			if (RwFrame* newFrame = frame->child)  FindNodesRecursive(newFrame, vehicle, bReSearch, bOnExtras);
			if (RwFrame* newFrame = frame->next)   FindNodesRecursive(newFrame, vehicle, bReSearch, bOnExtras);
			
			return;
		}

		//frame->modelling.pos

		//AddChatMessage(std::string(name + " at " + std::to_string(frame->modelling.pos.x) + ", " + std::to_string(frame->modelling.pos.y) + ", " + std::to_string(frame->modelling.pos.z)).c_str());

		//AddChatMessage(std::string(name + ": " + std::to_string(frame->modelling.right.x) + "," + std::to_string(frame->modelling.right.y) + "," + std::to_string(frame->modelling.right.z)).c_str());
		//((1 + frame->modelling.right.x) * 3.14f) - 3.14f)

		float differencor = frame->modelling.right.x;

		differencor = ((differencor > 0.0f) ? (1.0f - differencor) : (differencor)) * 3.14f;

		//AddChatMessage(std::string(name + " angles " + std::to_string(frame->modelling.right.x) + ", differencor: " + std::to_string(differencor)).c_str());

		//differencor = ((differencor > 0.0f) ? (1.0f - differencor) : (differencor)) * 3.14f;

		Dummies.push_back(VehicleDummyData(CVector(frame->modelling.pos.x, frame->modelling.pos.y, frame->modelling.pos.z), differencor));

		return;
	}
}
