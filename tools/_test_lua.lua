print("PROBE5_START")
local frame = 0
local typed = false
emu.register_frame(function()
    frame = frame + 1
    if frame == 1600 and not typed then
        typed = true
        manager.machine.natkeyboard:post("RUN COLOR DEMOSOFT\r")
        print("TYPED at frame 1600")
    end
    if frame > 1600 and (frame - 1600) % 300 == 0 then
        local cpu = manager.machine.devices[":maincpu"]
        local mem = cpu.spaces["program"]
        local txt = {}
        for addr = 0x0400, 0x07FF do
            local b = mem:read_u8(addr) & 0x7F
            if b >= 0x20 and b < 0x7F then txt[#txt+1] = string.char(b) else txt[#txt+1] = "." end
        end
        print("frame=" .. frame .. " SCREEN: " .. table.concat(txt):sub(1,800))
    end
    if frame >= 4600 then
        manager.machine:exit()
    end
end)
