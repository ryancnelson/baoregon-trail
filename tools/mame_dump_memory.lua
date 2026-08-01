-- tools/mame_dump_memory.lua
-- MAME autoboot script: waits N frames for the real Apple II DOS 3.3
-- boot + HELLO auto-run to complete on real, accurate MAME hardware
-- emulation (real Disk II controller, real Autostart ROM, real 6502
-- core), then dumps the machine's 64KB main RAM AND the 6502's actual
-- register state to files for use as a real, verified initial state
-- in baoregon-trail's own emulator.
--
-- Usage (see run_mame_dump.sh for the full invocation):
--   mame apple2e -flop1 <disk.dsk> -video none -sound none \
--     -autoboot_delay 0 -autoboot_script tools/mame_dump_memory.lua \
--     -seconds_to_run <N>
--
-- Output:
--   /tmp/mame_ram_dump.bin   (65536 bytes, address 0 = offset 0)
--   /tmp/mame_registers.txt  (PC, A, X, Y, S, P -- decimal, one per line)

local WAIT_FRAMES = 600   -- ~10 seconds at 60fps -- enough for boot+HELLO
local RAM_OUT_PATH = "/tmp/mame_ram_dump.bin"
local REG_OUT_PATH = "/tmp/mame_registers.txt"
local dumped = false

local function dump_state()
    local ok, err = pcall(function()
        local cpu = manager.machine.devices[":maincpu"]
        local mem = cpu.spaces["program"]

        local f = io.open(RAM_OUT_PATH, "wb")
        local bytes = {}
        for addr = 0, 0xFFFF do
            bytes[#bytes + 1] = string.char(mem:read_u8(addr))
        end
        f:write(table.concat(bytes))
        f:close()

        local regf = io.open(REG_OUT_PATH, "w")
        for name, entry in pairs(cpu.state) do
            regf:write(string.format("%s=%d\n", name, entry.value))
        end
        regf:close()

        print("MAME_DUMP: wrote " .. RAM_OUT_PATH .. " and " .. REG_OUT_PATH)
    end)
    if not ok then
        print("MAME_DUMP_ERROR: " .. tostring(err))
    end
end

local frame_count = 0
print("MAME_DUMP: script loaded, waiting for " .. WAIT_FRAMES .. " frames")
emu.register_frame(function()
    frame_count = frame_count + 1
    if frame_count % 100 == 0 then
        print("MAME_DUMP: frame_count=" .. frame_count)
    end
    if frame_count >= WAIT_FRAMES and not dumped then
        dumped = true
        dump_state()
        manager.machine:exit()
    end
end)
