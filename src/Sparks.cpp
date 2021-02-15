#include "Sparks.h"
#include "extensions\ScriptCommands.h"
#include "CHud.h"
#include "CTrain.h"

using namespace plugin;

void ProcessSparks(const std::string& name, RwFrame* frame, FVCData& data, CVehicle* pVeh)
{
    // Copy the code from tram.asi
    // if (name.find("fc_spk") != std::string::npos)
	// {
    //     // Let's try fetch the values from name
    //     int dir_x = std::stoi(ExtractStringValue(name, ".*x(-?[0-9]+).*", "0"));
    //     int dir_y = std::stoi(ExtractStringValue(name, ".*y(-?[0-9]+).*", "0"));
    //     int dir_z = std::stoi(ExtractStringValue(name, ".*z(-?[0-9]+).*", "1"));
    //     int density = std::stoi(ExtractStringValue(name, ".*_(-?[0-9]+).*", "80"));

    //     CHud::SetMessage((char*)std::to_string(pVeh->GetAtDirection().Magnitude()).c_str());
    //     CVector pos = frame->ltm.pos;
    //     if (data.realistic_speed > 0 && KeyPressed(83))
	// 	    Command<Commands::ADD_SPARKS>(pos.x,pos.y,pos.z+2,dir_x,dir_y,dir_z+5,density);
	// }
}