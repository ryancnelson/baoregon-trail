-- tools/mame_dump_memory.lua
-- MAME autoboot script: waits N frames for the real Apple II DOS 3.3
-- boot + HELLO auto-run to complete on real, accurate MAME hardware
-- emulation (real Disk II controller, real Autostart ROM, real 6502
-- core), then dumps the machine's 64KB main RAM to a raw binary file.
--
-- Usage (see run_mame_dump.sh for the full invocation):
--   mame apple2e -flop1 <disk.dsk> -video none -sound none \
--     -autoboot_delay 0 -autoboot_script tools/mame_dump_memory.lua \
--     -seconds_to_run <N>
--
-- Output: writes /tmp/mame_ram_dump.bin (65536 bytes, address 0 = offset 0)
-- once WAIT_FRAMES have elapsed, then requests MAME exit.

local WAIT_FRAMES = 600   -- ~10 seconds at 60fps -- enough for boot+HELLO
local OUT_PATH = "/tmp/mame_ram_dump.bin"
local dumped = false

local function dump_ram()
    local cpu = manager.machine.devices[":maincpu"]
    local mem = cpu.spaces["program"]
    local f = io.open(OUT_PATH, "wb")
    local bytes = {}
    for addr = 0, 0xFFFF do
        bytes[#bytes + 1] = string.char(mem:read_u8(addr))
    end
    f:write(table.concat(bytes))
    f:close()
    print("MAME_DUMP: wrote " .. OUT_PATH .. " (65536 bytes) at frame " ..
          tostring(manager.machine.video.frame_number))
end

local frame_count = 0
emu.register_frame(function()
    frame_count = frame_count + 1
    if frame_count >= WAIT_FRAMES and not dumped then
        dumped = true
        dump_ram()
        manager.machine:exit()
    end
end)
