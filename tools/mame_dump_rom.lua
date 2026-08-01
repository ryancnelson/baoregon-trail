-- tools/mame_dump_rom.lua
-- Dumps the real, correctly-mapped $C000-$FFFF ROM region directly from
-- MAME's own memory space (which we've already proven boots real DOS 3.3
-- correctly) -- sidesteps guessing the 342-0134/342-0135 concatenation
-- order ourselves. This is the authoritative, MAME-verified byte layout.
local OUT_PATH = "/tmp/mame_verified_rom_c000_ffff.bin"
local cpu = manager.machine.devices[":maincpu"]
local mem = cpu.spaces["program"]
local f = io.open(OUT_PATH, "wb")
local bytes = {}
for addr = 0xC000, 0xFFFF do
    bytes[#bytes + 1] = string.char(mem:read_u8(addr))
end
f:write(table.concat(bytes))
f:close()
print("MAME_ROM_DUMP: wrote " .. OUT_PATH .. " (" .. #bytes .. " bytes)")
manager.machine:exit()
