CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -O0 -g

BUILD_DIR := build

.PHONY: test clean

test: $(BUILD_DIR)/test_video_apple2
	./$(BUILD_DIR)/test_video_apple2

$(BUILD_DIR)/test_video_apple2: tests/test_video_apple2.c src/video_apple2.c src/video_apple2.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ tests/test_video_apple2.c src/video_apple2.c

clean:
	rm -rf $(BUILD_DIR)
