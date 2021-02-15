local module = {}

local log_name = script.this.name .. ".log"
local file_path = string.format("%s//%s", getGameDirectory(), log_name)

function module.Write(text)
    if WRITE_INFO_TO_LOG then
        local file = io.open(file_path,'a')
        file:write(string.format("[%s] %s\n",os.date(),text))
        file:close()
    end
end

function module.Close()
    module.Write("Log closed")
end

function module.Start()
    os.remove(file_path)
    module.Write("Log started")
    module.Write(string.format("Script version %s (%d)",script.this.version,script.this.version_num))
    module.Write("Please provide both 'moonloader.log' & '" .. log_name .. "' for debugging")
end

function module.ProcessingComponent(comp_name)
    module.Write(string.format("Processing component %s",comp_name))
end

return module