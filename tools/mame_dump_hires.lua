-- tools/mame_dump_hires.lua
-- Boots the real DOS 3.3 disk, waits for HELLO to auto-run, types a
-- command to RUN a real graphics program from the disk (e.g. "COLOR
-- DEMO"), waits for it to draw, then dumps ONLY the Hi-Res video buffer
-- ($2000-$3FFF, 8192 bytes) -- real pixel data from real Apple II
-- software, for rendering through baoregon-trail's own tested Hi-Res
-- color decode pipeline.

local BOOT_WAIT_FRAMES = 1600   -- real DOS+HELLO needs ~1200-1500 frames to reach the ']' prompt (measured)
local TYPE_WAIT_FRAMES = 60    -- settle time after typing before checking again
local DRAW_WAIT_FRAMES = 1800   -- ~30 emulated seconds -- real Disk II seek/read is slow
local OUT_PATH = "/tmp/mame_hires_dump.bin"

local phase = 0   -- 0=waiting to type command, 1=waiting for draw, 2=done
local frame_count = 0
local typed = false

local function type_text(text)
    local nat = manager.machine.natkeyboard
    nat:post(text)
end

local function dump_hires()
    local ok, err = pcall(function()
        local cpu = manager.machine.devices[":maincpu"]
        local mem = cpu.spaces["program"]
        local f = io.open(OUT_PATH, "wb")
        local bytes = {}
        for addr = 0x2000, 0x3FFF do
            bytes[#bytes + 1] = string.char(mem:read_u8(addr))
        end
        f:write(table.concat(bytes))
        f:close()
        print("MAME_HIRES_DUMP: wrote " .. OUT_PATH .. " (" .. #bytes .. " bytes)")

        -- Also dump text screen + softswitch-relevant zero page for
        -- diagnosis: did the program actually finish running and switch
        -- to graphics mode, or are we looking at stale/in-progress state?
        local txt = {}
        for addr = 0x0400, 0x07FF do
            local b = mem:read_u8(addr) & 0x7F
            if b >= 0x20 and b < 0x7F then
                txt[#txt + 1] = string.char(b)
            else
                txt[#txt + 1] = "."
            end
        end
        print("TEXT_SCREEN: " .. table.concat(txt):sub(1, 400))
    end)
    if not ok then
        print("MAME_HIRES_DUMP_ERROR: " .. tostring(err))
    end
end

print("MAME_HIRES: script loaded")
emu.register_frame(function()
    frame_count = frame_count + 1

    if phase == 0 and frame_count >= BOOT_WAIT_FRAMES then
        print("MAME_HIRES: typing RUN command at frame " .. frame_count)
        type_text("RUN COLOR DEMOSOFT\r")
        phase = 1
        frame_count = 0
    elseif phase == 1 and frame_count >= DRAW_WAIT_FRAMES then
        print("MAME_HIRES: dumping at frame " .. frame_count)
        dump_hires()
        phase = 2
        manager.machine:exit()
    end
end)
