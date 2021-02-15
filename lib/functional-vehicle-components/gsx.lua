local module = {}

--------------------------------------------------
-- Garage Save Extender (GSX) https://gtaforums.com/topic/925563-garage-save-extender/
-- gsx-data https://forum.mixmods.com.br/f16-utilidades/t2954-gsx-data-usar-gsx-em-scripts-lua-facilmente

if tmain.gsx.handle == 0 then
    flog.Write("You do not have Garage Save Extender (GSX) installed. Please download it from https://gtaforums.com/topic/925563-garage-save-extender/")
else
    ffi.cdef[[
    typedef uint32_t GSXCVehicle;
    typedef struct { float x, y, z; } CVector;
    typedef struct __attribute__((packed, aligned(1))) {
        CVector pos;
        uint32_t handling_flags;
        uint8_t flags;
        uint8_t field_11;
        uint16_t model;
        uint16_t carmods[15];
        uint8_t colour[4];
        uint8_t radio_station;
        uint8_t extra1;
        uint8_t extra2;
        uint8_t bomb_type;
        uint8_t paintjob;
        uint8_t nitro_count;
        uint8_t angleX;
        uint8_t angleY;
        uint8_t angleZ;
        uint8_t field_3F;
    } CStoredCar;
    typedef struct __attribute__((packed, aligned(1))) { GSXCVehicle veh; int32_t status; size_t when; } journalNews;
    typedef struct __attribute__((packed, aligned(1))) { GSXCVehicle veh; int32_t status; } apiCarNotify;
    typedef struct __attribute__((packed, aligned(1))) { GSXCVehicle veh; int32_t status; CStoredCar *gameStoredData; } externalCallbackStructure;
    typedef void(__cdecl externalCbFun_t)(const externalCallbackStructure*);
    int __cdecl addNotifyCallback(externalCbFun_t fun);
    void __cdecl removeNotifyCallback(int cbRef);
    void __cdecl pushDirectlyToSavedData(GSXCVehicle veh, const char *name, int size, void *ptr);
    int __cdecl dataToLoadExists(GSXCVehicle veh, const char *name);
    void*  __cdecl getLoadDataByVehPtr(GSXCVehicle veh, const char *name);
    int __cdecl getDataToLoadSize(GSXCVehicle veh, const char *name);
    ]]

    local gsx = ffi.load("gsx.asi")

    DataToLoadExists = ffi.cast("int (*)(int,const char*)",gsx.dataToLoadExists)
    GetDataToLoadSize = ffi.cast("int (*)(int,const char*)",gsx.getDataToLoadSize)
    GetLoadDataByVehPtr = ffi.cast("int (*)(int,const char*)",gsx.getLoadDataByVehPtr)
    PushDirectlyToSavedData = ffi.cast("int (*)(int,const char*,int,int)",gsx.pushDirectlyToSavedData)
    module.RemoveNotifyCallback = ffi.cast("void (*)(int)",gsx.removeNotifyCallback)

    module.pNotifyCallback = gsx.addNotifyCallback(function(data)
        local t = tmain.gsx.veh_data
        if data.status == 1 then -- save vehicle
            local wdata = encodeJson(t[getVehiclePointerHandle(data.veh)])
            if wdata ~= nil then
                PushDirectlyToSavedData(data.veh,"FVC_DATA",#wdata,memory.strptr(wdata));
            end
        end
    end)
end

function module.Set(veh,str,val)
    local t = tmain.gsx.veh_data
    if doesVehicleExist(veh) then 
        if  t[veh] == nil then 
            t[veh] = {}
        end
        t[veh][str] = val
    end
end

function module.Get(veh,str)
    local t = tmain.gsx.veh_data
    if doesVehicleExist(veh) then 
        if t[veh] then
            return t[veh][str]
        end
    end
    return nil
end

return module