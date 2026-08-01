CC = cc
CFLAGS = -std=c99 -Wall -Wextra -g

SRC_DIR = src
TEST_DIR = tests
BUILD_DIR = build

.PHONY: test clean

test: $(BUILD_DIR)/test_reset $(BUILD_DIR)/test_opcodes $(BUILD_DIR)/test_disk_sector_layout $(BUILD_DIR)/test_disk_trap $(BUILD_DIR)/test_video_apple2 $(BUILD_DIR)/test_video_apple2_color $(BUILD_DIR)/test_video_apple2_color_edges $(BUILD_DIR)/test_video_apple2_fullframe $(BUILD_DIR)/test_video_apple2_realbus $(BUILD_DIR)/test_bunnie_audio $(BUILD_DIR)/test_apple2_mem $(BUILD_DIR)/test_bio_display $(BUILD_DIR)/test_rram_driver $(BUILD_DIR)/test_cartridge_layout
	@$(BUILD_DIR)/test_reset
	@$(BUILD_DIR)/test_opcodes
	@$(BUILD_DIR)/test_disk_sector_layout
	@$(BUILD_DIR)/test_disk_trap
	@$(BUILD_DIR)/test_video_apple2
	@$(BUILD_DIR)/test_video_apple2_color
	@$(BUILD_DIR)/test_video_apple2_color_edges
	@$(BUILD_DIR)/test_video_apple2_fullframe
	@$(BUILD_DIR)/test_video_apple2_realbus
	@$(BUILD_DIR)/test_bunnie_audio
	@$(BUILD_DIR)/test_apple2_mem
	@$(BUILD_DIR)/test_bio_display
	@$(BUILD_DIR)/test_rram_driver
	@$(BUILD_DIR)/test_cartridge_layout
	@python3 -m unittest tests.test_embed_disk -v

$(BUILD_DIR)/test_reset: $(TEST_DIR)/test_reset.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/cpu6502.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_reset.c $(SRC_DIR)/cpu6502.c

$(BUILD_DIR)/test_opcodes: $(TEST_DIR)/test_opcodes.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/cpu6502.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_opcodes.c $(SRC_DIR)/cpu6502.c

$(BUILD_DIR)/test_disk_sector_layout: $(TEST_DIR)/test_disk_sector_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_sector_layout.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_disk_sector_layout.c $(SRC_DIR)/disk_sector_layout.c

$(BUILD_DIR)/test_disk_trap: $(TEST_DIR)/test_disk_trap.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/disk_trap.h $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_sector_layout.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_disk_trap.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c

$(BUILD_DIR)/test_video_apple2: $(TEST_DIR)/test_video_apple2.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/video_apple2.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_video_apple2.c $(SRC_DIR)/video_apple2.c

$(BUILD_DIR)/test_video_apple2_color: $(TEST_DIR)/test_video_apple2_color.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/video_apple2.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_video_apple2_color.c $(SRC_DIR)/video_apple2.c

$(BUILD_DIR)/test_video_apple2_color_edges: $(TEST_DIR)/test_video_apple2_color_edges.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/video_apple2.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_video_apple2_color_edges.c $(SRC_DIR)/video_apple2.c

$(BUILD_DIR)/test_video_apple2_fullframe: $(TEST_DIR)/test_video_apple2_fullframe.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/video_apple2.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_video_apple2_fullframe.c $(SRC_DIR)/video_apple2.c

$(BUILD_DIR)/test_video_apple2_realbus: $(TEST_DIR)/test_video_apple2_realbus.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/video_apple2.h $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/apple2_mem.h $(SRC_DIR)/cpu6502.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/bunnie_audio.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_video_apple2_realbus.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/bunnie_audio.c

$(BUILD_DIR)/test_bunnie_audio: $(TEST_DIR)/test_bunnie_audio.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/bunnie_audio.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_bunnie_audio.c $(SRC_DIR)/bunnie_audio.c

$(BUILD_DIR)/test_apple2_mem: $(TEST_DIR)/test_apple2_mem.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/apple2_mem.h $(SRC_DIR)/cpu6502.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/bunnie_audio.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_apple2_mem.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/bunnie_audio.c

$(BUILD_DIR)/test_bio_display: $(TEST_DIR)/test_bio_display.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/bio_display.h $(SRC_DIR)/video_apple2.c $(SRC_DIR)/video_apple2.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_bio_display.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/video_apple2.c

$(BUILD_DIR)/test_rram_driver: $(TEST_DIR)/test_rram_driver.c $(SRC_DIR)/rram_driver.c $(SRC_DIR)/rram_driver.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_rram_driver.c $(SRC_DIR)/rram_driver.c

$(BUILD_DIR)/test_cartridge_layout: $(TEST_DIR)/test_cartridge_layout.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/cartridge_layout.h $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_sector_layout.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_cartridge_layout.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c

# Firmware-level tests requiring a RISC-V cross-compiler + QEMU (not part of
# the default host `test` target, which should have zero extra toolchain
# dependencies beyond a native C compiler + Python). Run explicitly via
# `make test-firmware`.
.PHONY: test-firmware
test-firmware:
	@./tools/run_bio_audio_qemu_test.sh

# BIO Core RTL-level test requiring an external bio-sim checkout
# (github.com/baochip/bio-sim) + Verilator + a ziglang-equipped Python venv.
# Not part of the default `test` target -- see bio-sim-tests/README.md for
# one-time setup. Override BIO_SIM_DIR / ZIGLANG_PYTHON as needed.
.PHONY: test-biosim
test-biosim:
	@python3 tools/run_bio_display_palette_biosim.py

clean:
	rm -rf $(BUILD_DIR)
