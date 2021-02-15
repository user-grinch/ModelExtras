
function module.NeutralLed(veh, comp)

    flog.ProcessingComponent(comp.name)
    while true do
        if futil.VehicleCheck(veh) then return end

        if getCarCurrentGear(veh) == 0 and isCarEngineOn(veh) then
            futil.SetMaterialColor(comp,0,200,0,255)
        else
            futil.SetMaterialColor(comp,futil.GetSkygfxVehpipeColor({10,25,10,255}, {2,4,2,255}))
        end
        
        wait(0)
    end
end

function module.DamageLed(veh, comp)

    flog.ProcessingComponent(comp.name)
    while true do
        if futil.VehicleCheck(veh) then return end

        if getCarHealth(veh) < 650 and isCarEngineOn(veh) then
            futil.SetMaterialColor(comp,255,30,21,255)
        else
            futil.SetMaterialColor(comp,futil.GetSkygfxVehpipeColor({30,10,10,255}, {5,3,3,255}))
        end
        wait(0)
    end
end

function module.PowerLed(veh, comp)

    flog.ProcessingComponent(comp.name)
    while true do
        local speed = futil.GetRealisticSpeed(veh, 1)
        if futil.VehicleCheck(veh) then return end

        if isCarEngineOn(veh) then
            if speed >= 100 then
                futil.SetMaterialColor(comp,240,0,0,255)
            else
                futil.SetMaterialColor(comp,0,200,0,255)
            end
        else
            futil.SetMaterialColor(comp,futil.GetSkygfxVehpipeColor({30,30,30,255}, {5,5,5,255}))
        end
        wait(0)
    end
end