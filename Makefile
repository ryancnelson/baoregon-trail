CC = cc
CFLAGS = -std=c99 -Wall -Wextra -g

SRC_DIR = src
TEST_DIR = tests
BUILD_DIR = build

.PHONY: test clean

test: $(BUILD_DIR)/test_reset $(BUILD_DIR)/test_opcodes $(BUILD_DIR)/test_disk_sector_layout $(BUILD_DIR)/test_disk_trap $(BUILD_DIR)/test_video_apple2 $(BUILD_DIR)/test_bunnie_audio
	@$(BUILD_DIR)/test_reset
	@$(BUILD_DIR)/test_opcodes
	@$(BUILD_DIR)/test_disk_sector_layout
	@$(BUILD_DIR)/test_disk_trap
	@$(BUILD_DIR)/test_video_apple2
	@$(BUILD_DIR)/test_bunnie_audio

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

$(BUILD_DIR)/test_bunnie_audio: $(TEST_DIR)/test_bunnie_audio.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/bunnie_audio.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_bunnie_audio.c $(SRC_DIR)/bunnie_audio.c

clean:
	rm -rf $(BUILD_DIR)
