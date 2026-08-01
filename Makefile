CC = cc
CFLAGS = -std=c99 -Wall -Wextra -g -Isrc

SRC_DIR = src
TEST_DIR = tests
TOOLS_DIR = tools
BUILD_DIR = build

.PHONY: test clean

test: $(BUILD_DIR)/test_reset $(BUILD_DIR)/test_opcodes $(BUILD_DIR)/test_functional_suite $(BUILD_DIR)/test_interrupts $(BUILD_DIR)/test_stack_wraparound $(BUILD_DIR)/test_exec6502 $(BUILD_DIR)/test_disk_sector_layout $(BUILD_DIR)/test_disk_sector_layout_null_safety $(BUILD_DIR)/test_disk_sector_layout_boundary_cases $(BUILD_DIR)/test_disk_trap $(BUILD_DIR)/test_disk_trap_safe_defaults $(BUILD_DIR)/test_disk_trap_unload_image $(BUILD_DIR)/test_disk_trap_unmounted_image_safety $(BUILD_DIR)/test_disk_trap_has_selection $(BUILD_DIR)/test_disk_trap_get_selected_offset $(BUILD_DIR)/test_disk_trap_clear_image $(BUILD_DIR)/test_disk_trap_get_image_ptr $(BUILD_DIR)/test_apple2_mem_reset_clears_disk_trap_selection $(BUILD_DIR)/test_video_apple2 $(BUILD_DIR)/test_video_apple2_safety $(BUILD_DIR)/test_video_apple2_color $(BUILD_DIR)/test_video_apple2_color_edges $(BUILD_DIR)/test_video_apple2_fullframe $(BUILD_DIR)/test_video_apple2_realbus $(BUILD_DIR)/test_video_apple2_page2 $(BUILD_DIR)/test_video_apple2_color_page2 $(BUILD_DIR)/test_lores_apple2 $(BUILD_DIR)/test_lores_apple2_palette $(BUILD_DIR)/test_lores_apple2_safety $(BUILD_DIR)/test_lores_apple2_color_bounds $(BUILD_DIR)/test_bunnie_audio $(BUILD_DIR)/test_bunnie_audio_null_safety $(BUILD_DIR)/test_bunnie_audio_toggle_toggle_no_lost_edges $(BUILD_DIR)/test_apple2_mem $(BUILD_DIR)/test_apple2_mem_button_getter $(BUILD_DIR)/test_apple2_mem_button_bounds_safety $(BUILD_DIR)/test_apple2_mem_clear_buttons $(BUILD_DIR)/test_apple2_mem_clear_annunciators $(BUILD_DIR)/test_apple2_mem_reset_uses_clear_helpers $(BUILD_DIR)/test_apple2_mem_reset_clears_paddle_state $(BUILD_DIR)/test_bio_display $(BUILD_DIR)/test_bio_display_page2 $(BUILD_DIR)/test_bio_display_mixed $(BUILD_DIR)/test_bio_display_auto $(BUILD_DIR)/test_bio_display_lores_mixed $(BUILD_DIR)/test_bio_display_safety $(BUILD_DIR)/test_bio_display_text_mode $(BUILD_DIR)/test_fb_terminal_viewer $(BUILD_DIR)/test_rram_driver $(BUILD_DIR)/test_rram_driver_read_bounds $(BUILD_DIR)/test_rram_driver_docstring_accuracy $(BUILD_DIR)/test_rram_driver_page_aligned_boundary_write $(BUILD_DIR)/test_rram_driver_erase_bounds_safety $(BUILD_DIR)/test_rram_cartridge_write_bounds $(BUILD_DIR)/test_cartridge_layout $(BUILD_DIR)/test_cartridge_layout_bounds_safety $(BUILD_DIR)/test_cartridge_layout_get_slot_null_safety $(BUILD_DIR)/test_rram_cartridge_integration $(BUILD_DIR)/test_rram_disk_trap_pipeline $(BUILD_DIR)/test_boot_splash $(BUILD_DIR)/test_boot_splash_apple2_mem_poll $(BUILD_DIR)/test_boot_splash_multibutton_tiebreak $(BUILD_DIR)/test_boot_splash_null_safety $(BUILD_DIR)/test_boot_splash_invalid_index_clamping $(BUILD_DIR)/test_emulator_loop $(BUILD_DIR)/test_emulator_loop_get_framebuffer_nonnull $(BUILD_DIR)/test_emulator_loop_is_audio_active $(BUILD_DIR)/test_emulator_loop_reset_combo $(BUILD_DIR)/test_emulator_loop_reset_combo_no_spurious_nav $(BUILD_DIR)/test_emulator_loop_reset_combo_partial_release $(BUILD_DIR)/test_emulator_loop_video_mode $(BUILD_DIR)/test_emulator_loop_copy_framebuffer $(BUILD_DIR)/test_emulator_loop_copy_framebuffer_null_safety $(BUILD_DIR)/test_emulator_loop_framebuffer_bounds $(BUILD_DIR)/test_emulator_loop_current_slot $(BUILD_DIR)/test_emulator_loop_select_attaches_disk_image $(BUILD_DIR)/test_emulator_loop_select_slot1_attaches_disk_image $(BUILD_DIR)/test_emulator_loop_game_switch_updates_disk_image $(BUILD_DIR)/test_emulator_loop_reselect_same_slot_disk_image $(BUILD_DIR)/test_emulator_loop_game_running_execution_cycles $(BUILD_DIR)/test_emulator_loop_splash_active $(BUILD_DIR)/test_emulator_loop_splash_active_matches_canonical $(BUILD_DIR)/test_emulator_loop_reset $(BUILD_DIR)/test_emulator_loop_game_running $(BUILD_DIR)/test_emulator_loop_reset_to_splash $(BUILD_DIR)/test_emulator_loop_reset_to_splash_cycles $(BUILD_DIR)/test_emulator_loop_init_reset_parity $(BUILD_DIR)/test_emulator_loop_no_stale_framebuffer $(BUILD_DIR)/test_emulator_loop_no_stale_framebuffer_combo_reset $(BUILD_DIR)/test_emulator_loop_total_cycles $(BUILD_DIR)/test_emulator_loop_audio_active $(BUILD_DIR)/test_emulator_loop_cycles_per_frame $(BUILD_DIR)/test_emulator_loop_dma_push $(BUILD_DIR)/test_boot_perf $(BUILD_DIR)/test_boot_perf_safety $(BUILD_DIR)/test_main_boot_perf $(BUILD_DIR)/test_main_boot_metrics_getter $(BUILD_DIR)/test_dos33_composed_boot
	@$(BUILD_DIR)/test_reset
	@$(BUILD_DIR)/test_opcodes
	@./tests/fetch_functional_test.sh
	@$(BUILD_DIR)/test_functional_suite
	@$(BUILD_DIR)/test_interrupts
	@$(BUILD_DIR)/test_stack_wraparound
	@$(BUILD_DIR)/test_exec6502
	@$(BUILD_DIR)/test_disk_sector_layout
	@$(BUILD_DIR)/test_disk_sector_layout_null_safety
	@$(BUILD_DIR)/test_disk_sector_layout_boundary_cases
	@$(BUILD_DIR)/test_disk_trap
	@$(BUILD_DIR)/test_disk_trap_safe_defaults
	@$(BUILD_DIR)/test_disk_trap_unload_image
	@$(BUILD_DIR)/test_disk_trap_unmounted_image_safety
	@$(BUILD_DIR)/test_disk_trap_has_selection
	@$(BUILD_DIR)/test_disk_trap_get_selected_offset
	@$(BUILD_DIR)/test_disk_trap_clear_image
	@$(BUILD_DIR)/test_disk_trap_get_image_ptr
	@$(BUILD_DIR)/test_apple2_mem_reset_clears_disk_trap_selection
	@$(BUILD_DIR)/test_video_apple2
	@$(BUILD_DIR)/test_video_apple2_safety
	@$(BUILD_DIR)/test_video_apple2_color
	@$(BUILD_DIR)/test_video_apple2_color_edges
	@$(BUILD_DIR)/test_video_apple2_fullframe
	@$(BUILD_DIR)/test_video_apple2_realbus
	@$(BUILD_DIR)/test_video_apple2_page2
	@$(BUILD_DIR)/test_video_apple2_color_page2
	@$(BUILD_DIR)/test_lores_apple2
	@$(BUILD_DIR)/test_lores_apple2_palette
	@$(BUILD_DIR)/test_lores_apple2_safety
	@$(BUILD_DIR)/test_lores_apple2_color_bounds
	@$(BUILD_DIR)/test_bunnie_audio
	@$(BUILD_DIR)/test_bunnie_audio_null_safety
	@$(BUILD_DIR)/test_bunnie_audio_toggle_toggle_no_lost_edges
	@$(BUILD_DIR)/test_apple2_mem
	@$(BUILD_DIR)/test_apple2_mem_button_getter
	@$(BUILD_DIR)/test_apple2_mem_button_bounds_safety
	@$(BUILD_DIR)/test_apple2_mem_clear_buttons
	@$(BUILD_DIR)/test_apple2_mem_clear_annunciators
	@$(BUILD_DIR)/test_apple2_mem_reset_uses_clear_helpers
	@$(BUILD_DIR)/test_apple2_mem_reset_clears_paddle_state
	@$(BUILD_DIR)/test_bio_display
	@$(BUILD_DIR)/test_bio_display_page2
	@$(BUILD_DIR)/test_bio_display_mixed
	@$(BUILD_DIR)/test_bio_display_auto
	@$(BUILD_DIR)/test_bio_display_lores_mixed
	@$(BUILD_DIR)/test_bio_display_safety
	@$(BUILD_DIR)/test_bio_display_text_mode
	@$(BUILD_DIR)/test_fb_terminal_viewer
	@$(BUILD_DIR)/test_rram_driver
	@$(BUILD_DIR)/test_rram_driver_read_bounds
	@$(BUILD_DIR)/test_rram_driver_docstring_accuracy
	@$(BUILD_DIR)/test_rram_driver_page_aligned_boundary_write
	@$(BUILD_DIR)/test_rram_driver_erase_bounds_safety
	@$(BUILD_DIR)/test_rram_cartridge_write_bounds
	@$(BUILD_DIR)/test_cartridge_layout
	@$(BUILD_DIR)/test_cartridge_layout_bounds_safety
	@$(BUILD_DIR)/test_cartridge_layout_get_slot_null_safety
	@$(BUILD_DIR)/test_rram_cartridge_integration
	@$(BUILD_DIR)/test_rram_disk_trap_pipeline
	@$(BUILD_DIR)/test_boot_splash
	@$(BUILD_DIR)/test_boot_splash_apple2_mem_poll
	@$(BUILD_DIR)/test_boot_splash_multibutton_tiebreak
	@$(BUILD_DIR)/test_boot_splash_null_safety
	@$(BUILD_DIR)/test_boot_splash_invalid_index_clamping
	@$(BUILD_DIR)/test_emulator_loop
	@$(BUILD_DIR)/test_emulator_loop_get_framebuffer_nonnull
	@$(BUILD_DIR)/test_emulator_loop_is_audio_active
	@$(BUILD_DIR)/test_emulator_loop_reset_combo
	@$(BUILD_DIR)/test_emulator_loop_reset_combo_no_spurious_nav
	@$(BUILD_DIR)/test_emulator_loop_reset_combo_partial_release
	@$(BUILD_DIR)/test_emulator_loop_video_mode
	@$(BUILD_DIR)/test_emulator_loop_copy_framebuffer
	@$(BUILD_DIR)/test_emulator_loop_copy_framebuffer_null_safety
	@$(BUILD_DIR)/test_emulator_loop_framebuffer_bounds
	@$(BUILD_DIR)/test_emulator_loop_current_slot
	@$(BUILD_DIR)/test_emulator_loop_select_attaches_disk_image
	@$(BUILD_DIR)/test_emulator_loop_select_slot1_attaches_disk_image
	@$(BUILD_DIR)/test_emulator_loop_game_switch_updates_disk_image
	@$(BUILD_DIR)/test_emulator_loop_reselect_same_slot_disk_image
	@$(BUILD_DIR)/test_emulator_loop_game_running_execution_cycles
	@$(BUILD_DIR)/test_emulator_loop_splash_active
	@$(BUILD_DIR)/test_emulator_loop_splash_active_matches_canonical
	@$(BUILD_DIR)/test_emulator_loop_reset
	@$(BUILD_DIR)/test_emulator_loop_game_running
	@$(BUILD_DIR)/test_emulator_loop_reset_to_splash
	@$(BUILD_DIR)/test_emulator_loop_reset_to_splash_cycles
	@$(BUILD_DIR)/test_emulator_loop_init_reset_parity
	@$(BUILD_DIR)/test_emulator_loop_no_stale_framebuffer
	@$(BUILD_DIR)/test_emulator_loop_no_stale_framebuffer_combo_reset
	@$(BUILD_DIR)/test_emulator_loop_total_cycles
	@$(BUILD_DIR)/test_emulator_loop_audio_active
	@$(BUILD_DIR)/test_emulator_loop_cycles_per_frame
	@$(BUILD_DIR)/test_emulator_loop_dma_push
	@$(BUILD_DIR)/test_boot_perf
	@$(BUILD_DIR)/test_boot_perf_safety
	@$(BUILD_DIR)/test_main_boot_perf
	@$(BUILD_DIR)/test_main_boot_metrics_getter
	@$(BUILD_DIR)/test_dos33_composed_boot
	@python3 -m unittest tests.test_embed_disk -v
	@python3 -m unittest tests.test_embed_disk_section_placement -v
	@python3 -m unittest tests.test_embed_disk_output_write_error -v
	@python3 -m unittest tests.test_embed_disk_missing_file -v
	@python3 -m unittest tests.test_check_linker_placement_missing_file -v
	@python3 -m unittest tests.test_build_cartridge_image -v
	@python3 -m unittest tests.test_build_cartridge_image_constants -v
	@python3 -m unittest tests.test_build_cartridge_image_write_error -v
	@python3 -m unittest tests.test_create_sample_boot_dsk_write_error -v

$(BUILD_DIR)/test_reset: $(TEST_DIR)/test_reset.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/cpu6502.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_reset.c $(SRC_DIR)/cpu6502.c

$(BUILD_DIR)/test_opcodes: $(TEST_DIR)/test_opcodes.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/cpu6502.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_opcodes.c $(SRC_DIR)/cpu6502.c

$(BUILD_DIR)/test_interrupts: $(TEST_DIR)/test_interrupts.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/cpu6502.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_interrupts.c $(SRC_DIR)/cpu6502.c

$(BUILD_DIR)/test_stack_wraparound: $(TEST_DIR)/test_stack_wraparound.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/cpu6502.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_stack_wraparound.c $(SRC_DIR)/cpu6502.c

$(BUILD_DIR)/test_exec6502: $(TEST_DIR)/test_exec6502.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/cpu6502.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_exec6502.c $(SRC_DIR)/cpu6502.c

$(BUILD_DIR)/test_disk_sector_layout: $(TEST_DIR)/test_disk_sector_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_sector_layout.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_disk_sector_layout.c $(SRC_DIR)/disk_sector_layout.c

$(BUILD_DIR)/test_disk_sector_layout_null_safety: $(TEST_DIR)/test_disk_sector_layout_null_safety.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_sector_layout.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_disk_sector_layout_null_safety.c $(SRC_DIR)/disk_sector_layout.c

$(BUILD_DIR)/test_disk_sector_layout_boundary_cases: $(TEST_DIR)/test_disk_sector_layout_boundary_cases.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_sector_layout.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_disk_sector_layout_boundary_cases.c $(SRC_DIR)/disk_sector_layout.c

$(BUILD_DIR)/test_disk_trap: $(TEST_DIR)/test_disk_trap.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/disk_trap.h $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_sector_layout.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_disk_trap.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c

$(BUILD_DIR)/test_disk_trap_safe_defaults: $(TEST_DIR)/test_disk_trap_safe_defaults.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/disk_trap.h $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_sector_layout.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_disk_trap_safe_defaults.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c

$(BUILD_DIR)/test_disk_trap_unload_image: $(TEST_DIR)/test_disk_trap_unload_image.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/disk_trap.h $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_sector_layout.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_disk_trap_unload_image.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c

$(BUILD_DIR)/test_disk_trap_unmounted_image_safety: $(TEST_DIR)/test_disk_trap_unmounted_image_safety.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/disk_trap.h $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_sector_layout.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_disk_trap_unmounted_image_safety.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c

$(BUILD_DIR)/test_disk_trap_has_selection: $(TEST_DIR)/test_disk_trap_has_selection.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/disk_trap.h $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_sector_layout.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_disk_trap_has_selection.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c

$(BUILD_DIR)/test_disk_trap_get_selected_offset: $(TEST_DIR)/test_disk_trap_get_selected_offset.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/disk_trap.h $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_sector_layout.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_disk_trap_get_selected_offset.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c

$(BUILD_DIR)/test_disk_trap_clear_image: $(TEST_DIR)/test_disk_trap_clear_image.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/disk_trap.h $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_sector_layout.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_disk_trap_clear_image.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c

$(BUILD_DIR)/test_disk_trap_get_image_ptr: $(TEST_DIR)/test_disk_trap_get_image_ptr.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/disk_trap.h $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_sector_layout.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_disk_trap_get_image_ptr.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c

$(BUILD_DIR)/test_apple2_mem_reset_clears_disk_trap_selection: $(TEST_DIR)/test_apple2_mem_reset_clears_disk_trap_selection.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/apple2_mem.h $(SRC_DIR)/cpu6502.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/bunnie_audio.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_apple2_mem_reset_clears_disk_trap_selection.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/bunnie_audio.c

$(BUILD_DIR)/test_video_apple2: $(TEST_DIR)/test_video_apple2.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/video_apple2.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_video_apple2.c $(SRC_DIR)/video_apple2.c

$(BUILD_DIR)/test_video_apple2_safety: $(TEST_DIR)/test_video_apple2_safety.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/video_apple2.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_video_apple2_safety.c $(SRC_DIR)/video_apple2.c

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

$(BUILD_DIR)/test_video_apple2_page2: $(TEST_DIR)/test_video_apple2_page2.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/video_apple2.h $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/apple2_mem.h $(SRC_DIR)/cpu6502.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/bunnie_audio.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_video_apple2_page2.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/bunnie_audio.c

$(BUILD_DIR)/test_video_apple2_color_page2: $(TEST_DIR)/test_video_apple2_color_page2.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/video_apple2.h $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/apple2_mem.h $(SRC_DIR)/cpu6502.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/bunnie_audio.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_video_apple2_color_page2.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/bunnie_audio.c

$(BUILD_DIR)/test_lores_apple2: $(TEST_DIR)/test_lores_apple2.c $(SRC_DIR)/lores_apple2.c $(SRC_DIR)/lores_apple2.h $(SRC_DIR)/video_apple2.c $(SRC_DIR)/video_apple2.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_lores_apple2.c $(SRC_DIR)/lores_apple2.c $(SRC_DIR)/video_apple2.c

$(BUILD_DIR)/test_lores_apple2_palette: $(TEST_DIR)/test_lores_apple2_palette.c $(SRC_DIR)/lores_apple2.c $(SRC_DIR)/lores_apple2.h $(SRC_DIR)/video_apple2.c $(SRC_DIR)/video_apple2.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_lores_apple2_palette.c $(SRC_DIR)/lores_apple2.c $(SRC_DIR)/video_apple2.c

$(BUILD_DIR)/test_lores_apple2_safety: $(TEST_DIR)/test_lores_apple2_safety.c $(SRC_DIR)/lores_apple2.c $(SRC_DIR)/lores_apple2.h $(SRC_DIR)/video_apple2.c $(SRC_DIR)/video_apple2.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_lores_apple2_safety.c $(SRC_DIR)/lores_apple2.c $(SRC_DIR)/video_apple2.c

$(BUILD_DIR)/test_lores_apple2_color_bounds: $(TEST_DIR)/test_lores_apple2_color_bounds.c $(SRC_DIR)/lores_apple2.c $(SRC_DIR)/lores_apple2.h $(SRC_DIR)/video_apple2.c $(SRC_DIR)/video_apple2.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_lores_apple2_color_bounds.c $(SRC_DIR)/lores_apple2.c $(SRC_DIR)/video_apple2.c

$(BUILD_DIR)/test_bunnie_audio: $(TEST_DIR)/test_bunnie_audio.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/bunnie_audio.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_bunnie_audio.c $(SRC_DIR)/bunnie_audio.c

$(BUILD_DIR)/test_bunnie_audio_null_safety: $(TEST_DIR)/test_bunnie_audio_null_safety.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/bunnie_audio.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_bunnie_audio_null_safety.c $(SRC_DIR)/bunnie_audio.c

$(BUILD_DIR)/test_bunnie_audio_toggle_toggle_no_lost_edges: $(TEST_DIR)/test_bunnie_audio_toggle_toggle_no_lost_edges.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/bunnie_audio.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_bunnie_audio_toggle_toggle_no_lost_edges.c $(SRC_DIR)/bunnie_audio.c

$(BUILD_DIR)/test_apple2_mem: $(TEST_DIR)/test_apple2_mem.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/apple2_mem.h $(SRC_DIR)/cpu6502.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/bunnie_audio.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_apple2_mem.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/bunnie_audio.c

$(BUILD_DIR)/test_apple2_mem_button_getter: $(TEST_DIR)/test_apple2_mem_button_getter.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/apple2_mem.h $(SRC_DIR)/cpu6502.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/bunnie_audio.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_apple2_mem_button_getter.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/bunnie_audio.c

$(BUILD_DIR)/test_apple2_mem_button_bounds_safety: $(TEST_DIR)/test_apple2_mem_button_bounds_safety.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/apple2_mem.h $(SRC_DIR)/cpu6502.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/bunnie_audio.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_apple2_mem_button_bounds_safety.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/bunnie_audio.c

$(BUILD_DIR)/test_apple2_mem_clear_buttons: $(TEST_DIR)/test_apple2_mem_clear_buttons.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/apple2_mem.h $(SRC_DIR)/cpu6502.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/bunnie_audio.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_apple2_mem_clear_buttons.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/bunnie_audio.c

$(BUILD_DIR)/test_apple2_mem_clear_annunciators: $(TEST_DIR)/test_apple2_mem_clear_annunciators.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/apple2_mem.h $(SRC_DIR)/cpu6502.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/bunnie_audio.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_apple2_mem_clear_annunciators.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/bunnie_audio.c

$(BUILD_DIR)/test_apple2_mem_reset_uses_clear_helpers: $(TEST_DIR)/test_apple2_mem_reset_uses_clear_helpers.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/apple2_mem.h $(SRC_DIR)/cpu6502.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/bunnie_audio.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_apple2_mem_reset_uses_clear_helpers.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/bunnie_audio.c

$(BUILD_DIR)/test_apple2_mem_reset_clears_paddle_state: $(TEST_DIR)/test_apple2_mem_reset_clears_paddle_state.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/apple2_mem.h $(SRC_DIR)/cpu6502.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/bunnie_audio.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_apple2_mem_reset_clears_paddle_state.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/bunnie_audio.c

$(BUILD_DIR)/test_bio_display: $(TEST_DIR)/test_bio_display.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/bio_display.h $(SRC_DIR)/video_apple2.c $(SRC_DIR)/video_apple2.h $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_bio_display.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_bio_display_page2: $(TEST_DIR)/test_bio_display_page2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/bio_display.h $(SRC_DIR)/video_apple2.c $(SRC_DIR)/video_apple2.h $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_bio_display_page2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_bio_display_mixed: $(TEST_DIR)/test_bio_display_mixed.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/bio_display.h $(SRC_DIR)/video_apple2.c $(SRC_DIR)/video_apple2.h $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_bio_display_mixed.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_bio_display_auto: $(TEST_DIR)/test_bio_display_auto.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/bio_display.h $(SRC_DIR)/video_apple2.c $(SRC_DIR)/video_apple2.h $(SRC_DIR)/lores_apple2.c $(SRC_DIR)/lores_apple2.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_bio_display_auto.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_bio_display_lores_mixed: $(TEST_DIR)/test_bio_display_lores_mixed.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/bio_display.h $(SRC_DIR)/video_apple2.c $(SRC_DIR)/video_apple2.h $(SRC_DIR)/lores_apple2.c $(SRC_DIR)/lores_apple2.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_bio_display_lores_mixed.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_bio_display_safety: $(TEST_DIR)/test_bio_display_safety.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/bio_display.h $(SRC_DIR)/video_apple2.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_bio_display_safety.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_bio_display_text_mode: $(TEST_DIR)/test_bio_display_text_mode.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/bio_display.h $(SRC_DIR)/video_apple2.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_bio_display_text_mode.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_fb_terminal_viewer: $(TEST_DIR)/test_fb_terminal_viewer.c $(TOOLS_DIR)/fb_terminal_viewer.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/bio_display.h $(SRC_DIR)/video_apple2.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_fb_terminal_viewer.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/fb_terminal_viewer: $(TOOLS_DIR)/fb_terminal_viewer.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/bio_display.h $(SRC_DIR)/video_apple2.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TOOLS_DIR)/fb_terminal_viewer.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/dump_framebuffer: $(TOOLS_DIR)/dump_framebuffer.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TOOLS_DIR)/dump_framebuffer.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/terminal_emulator: $(TOOLS_DIR)/terminal_emulator.c $(TOOLS_DIR)/fb_terminal_viewer.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -DFB_TERMINAL_VIEWER_NO_MAIN -o $@ $(TOOLS_DIR)/terminal_emulator.c $(TOOLS_DIR)/fb_terminal_viewer.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

# Interactive Playable Terminal Runner target (Fable-5 Item 2)
.PHONY: fb-view terminal-play
fb-view: $(BUILD_DIR)/dump_framebuffer $(BUILD_DIR)/fb_terminal_viewer
	@$(BUILD_DIR)/dump_framebuffer /tmp/baoregon_fb.raw
	@$(BUILD_DIR)/fb_terminal_viewer /tmp/baoregon_fb.raw

terminal-play: $(BUILD_DIR)/terminal_emulator
	@$(BUILD_DIR)/terminal_emulator

disks/dos33_sample.dsk: tools/create_sample_boot_dsk.py
	@mkdir -p disks
	python3 tools/create_sample_boot_dsk.py disks/dos33_sample.dsk

src/embedded_disk_dos33.h: disks/dos33_sample.dsk tools/embed_disk.py
	python3 tools/embed_disk.py disks/dos33_sample.dsk g_dos33_sample_dsk -o src/embedded_disk_dos33.h

$(BUILD_DIR)/test_dos33_composed_boot: $(TEST_DIR)/test_dos33_composed_boot.c src/embedded_disk_dos33.h $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/bunnie_audio.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_dos33_composed_boot.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/bunnie_audio.c

$(BUILD_DIR)/test_rram_driver: $(TEST_DIR)/test_rram_driver.c $(SRC_DIR)/rram_driver.c $(SRC_DIR)/rram_driver.h $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/cartridge_layout.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_rram_driver.c $(SRC_DIR)/rram_driver.c $(SRC_DIR)/cartridge_layout.c

$(BUILD_DIR)/test_rram_driver_read_bounds: $(TEST_DIR)/test_rram_driver_read_bounds.c $(SRC_DIR)/rram_driver.c $(SRC_DIR)/rram_driver.h $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/cartridge_layout.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_rram_driver_read_bounds.c $(SRC_DIR)/rram_driver.c $(SRC_DIR)/cartridge_layout.c

$(BUILD_DIR)/test_rram_driver_docstring_accuracy: $(TEST_DIR)/test_rram_driver_docstring_accuracy.c $(SRC_DIR)/rram_driver.c $(SRC_DIR)/rram_driver.h $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/cartridge_layout.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_rram_driver_docstring_accuracy.c $(SRC_DIR)/rram_driver.c $(SRC_DIR)/cartridge_layout.c

$(BUILD_DIR)/test_rram_driver_page_aligned_boundary_write: $(TEST_DIR)/test_rram_driver_page_aligned_boundary_write.c $(SRC_DIR)/rram_driver.c $(SRC_DIR)/rram_driver.h $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/cartridge_layout.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_rram_driver_page_aligned_boundary_write.c $(SRC_DIR)/rram_driver.c $(SRC_DIR)/cartridge_layout.c

$(BUILD_DIR)/test_rram_driver_erase_bounds_safety: $(TEST_DIR)/test_rram_driver_erase_bounds_safety.c $(SRC_DIR)/rram_driver.c $(SRC_DIR)/rram_driver.h $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/cartridge_layout.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_rram_driver_erase_bounds_safety.c $(SRC_DIR)/rram_driver.c $(SRC_DIR)/cartridge_layout.c

$(BUILD_DIR)/test_rram_cartridge_write_bounds: $(TEST_DIR)/test_rram_cartridge_write_bounds.c $(SRC_DIR)/rram_driver.c $(SRC_DIR)/rram_driver.h $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/cartridge_layout.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_rram_cartridge_write_bounds.c $(SRC_DIR)/rram_driver.c $(SRC_DIR)/cartridge_layout.c

$(BUILD_DIR)/test_cartridge_layout: $(TEST_DIR)/test_cartridge_layout.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/cartridge_layout.h $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_sector_layout.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_cartridge_layout.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c

$(BUILD_DIR)/test_cartridge_layout_bounds_safety: $(TEST_DIR)/test_cartridge_layout_bounds_safety.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/cartridge_layout.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_cartridge_layout_bounds_safety.c $(SRC_DIR)/cartridge_layout.c

$(BUILD_DIR)/test_cartridge_layout_get_slot_null_safety: $(TEST_DIR)/test_cartridge_layout_get_slot_null_safety.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/cartridge_layout.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_cartridge_layout_get_slot_null_safety.c $(SRC_DIR)/cartridge_layout.c

$(BUILD_DIR)/test_rram_cartridge_integration: $(TEST_DIR)/test_rram_cartridge_integration.c $(SRC_DIR)/rram_driver.c $(SRC_DIR)/rram_driver.h $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/cartridge_layout.h $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_sector_layout.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_rram_cartridge_integration.c $(SRC_DIR)/rram_driver.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c

$(BUILD_DIR)/test_rram_disk_trap_pipeline: $(TEST_DIR)/test_rram_disk_trap_pipeline.c $(SRC_DIR)/rram_driver.c $(SRC_DIR)/rram_driver.h $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/cartridge_layout.h $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_sector_layout.h $(SRC_DIR)/disk_trap.c $(SRC_DIR)/disk_trap.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_rram_disk_trap_pipeline.c $(SRC_DIR)/rram_driver.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c

$(BUILD_DIR)/test_boot_splash: $(TEST_DIR)/test_boot_splash.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/boot_splash.h $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/cartridge_layout.h $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/disk_trap.h $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/apple2_mem.h $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_boot_splash.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c

$(BUILD_DIR)/test_boot_splash_apple2_mem_poll: $(TEST_DIR)/test_boot_splash_apple2_mem_poll.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/boot_splash.h $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/cartridge_layout.h $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/disk_trap.h $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/apple2_mem.h $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_boot_splash_apple2_mem_poll.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c

$(BUILD_DIR)/test_boot_splash_multibutton_tiebreak: $(TEST_DIR)/test_boot_splash_multibutton_tiebreak.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/boot_splash.h $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/cartridge_layout.h $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/disk_trap.h $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/apple2_mem.h $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_boot_splash_multibutton_tiebreak.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c

$(BUILD_DIR)/test_boot_splash_null_safety: $(TEST_DIR)/test_boot_splash_null_safety.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/boot_splash.h $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/cartridge_layout.h $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/disk_trap.h $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/apple2_mem.h $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_boot_splash_null_safety.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c

$(BUILD_DIR)/test_boot_splash_invalid_index_clamping: $(TEST_DIR)/test_boot_splash_invalid_index_clamping.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/boot_splash.h $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/cartridge_layout.h $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/disk_trap.h $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/apple2_mem.h $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_boot_splash_invalid_index_clamping.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c

$(BUILD_DIR)/test_emulator_loop: $(TEST_DIR)/test_emulator_loop.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_emulator_loop.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_emulator_loop_get_framebuffer_nonnull: $(TEST_DIR)/test_emulator_loop_get_framebuffer_nonnull.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_emulator_loop_get_framebuffer_nonnull.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_emulator_loop_is_audio_active: $(TEST_DIR)/test_emulator_loop_is_audio_active.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_emulator_loop_is_audio_active.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_emulator_loop_reset_combo: $(TEST_DIR)/test_emulator_loop_reset_combo.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_emulator_loop_reset_combo.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_emulator_loop_reset_combo_no_spurious_nav: $(TEST_DIR)/test_emulator_loop_reset_combo_no_spurious_nav.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_emulator_loop_reset_combo_no_spurious_nav.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_emulator_loop_reset_combo_partial_release: $(TEST_DIR)/test_emulator_loop_reset_combo_partial_release.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_emulator_loop_reset_combo_partial_release.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_emulator_loop_video_mode: $(TEST_DIR)/test_emulator_loop_video_mode.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_emulator_loop_video_mode.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_emulator_loop_copy_framebuffer: $(TEST_DIR)/test_emulator_loop_copy_framebuffer.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_emulator_loop_copy_framebuffer.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_emulator_loop_copy_framebuffer_null_safety: $(TEST_DIR)/test_emulator_loop_copy_framebuffer_null_safety.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_emulator_loop_copy_framebuffer_null_safety.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_emulator_loop_framebuffer_bounds: $(TEST_DIR)/test_emulator_loop_framebuffer_bounds.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_emulator_loop_framebuffer_bounds.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_emulator_loop_current_slot: $(TEST_DIR)/test_emulator_loop_current_slot.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_emulator_loop_current_slot.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_emulator_loop_splash_active: $(TEST_DIR)/test_emulator_loop_splash_active.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_emulator_loop_splash_active.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_emulator_loop_splash_active_matches_canonical: $(TEST_DIR)/test_emulator_loop_splash_active_matches_canonical.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_emulator_loop_splash_active_matches_canonical.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_emulator_loop_reset: $(TEST_DIR)/test_emulator_loop_reset.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_emulator_loop_reset.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_emulator_loop_game_running: $(TEST_DIR)/test_emulator_loop_game_running.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_emulator_loop_game_running.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_emulator_loop_reset_to_splash: $(TEST_DIR)/test_emulator_loop_reset_to_splash.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_emulator_loop_reset_to_splash.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_emulator_loop_reset_to_splash_cycles: $(TEST_DIR)/test_emulator_loop_reset_to_splash_cycles.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_emulator_loop_reset_to_splash_cycles.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_emulator_loop_init_reset_parity: $(TEST_DIR)/test_emulator_loop_init_reset_parity.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_emulator_loop_init_reset_parity.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_emulator_loop_no_stale_framebuffer: $(TEST_DIR)/test_emulator_loop_no_stale_framebuffer.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_emulator_loop_no_stale_framebuffer.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_emulator_loop_no_stale_framebuffer_combo_reset: $(TEST_DIR)/test_emulator_loop_no_stale_framebuffer_combo_reset.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_emulator_loop_no_stale_framebuffer_combo_reset.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_emulator_loop_total_cycles: $(TEST_DIR)/test_emulator_loop_total_cycles.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_emulator_loop_total_cycles.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_emulator_loop_select_attaches_disk_image: $(TEST_DIR)/test_emulator_loop_select_attaches_disk_image.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_emulator_loop_select_attaches_disk_image.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_emulator_loop_select_slot1_attaches_disk_image: $(TEST_DIR)/test_emulator_loop_select_slot1_attaches_disk_image.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_emulator_loop_select_slot1_attaches_disk_image.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_emulator_loop_game_switch_updates_disk_image: $(TEST_DIR)/test_emulator_loop_game_switch_updates_disk_image.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_emulator_loop_game_switch_updates_disk_image.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_emulator_loop_reselect_same_slot_disk_image: $(TEST_DIR)/test_emulator_loop_reselect_same_slot_disk_image.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_emulator_loop_reselect_same_slot_disk_image.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_emulator_loop_game_running_execution_cycles: $(TEST_DIR)/test_emulator_loop_game_running_execution_cycles.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_emulator_loop_game_running_execution_cycles.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_emulator_loop_audio_active: $(TEST_DIR)/test_emulator_loop_audio_active.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_emulator_loop_audio_active.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_emulator_loop_cycles_per_frame: $(TEST_DIR)/test_emulator_loop_cycles_per_frame.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_emulator_loop_cycles_per_frame.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_emulator_loop_dma_push: $(TEST_DIR)/test_emulator_loop_dma_push.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/emulator_loop.h $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_emulator_loop_dma_push.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_boot_perf: $(TEST_DIR)/test_boot_perf.c $(SRC_DIR)/boot_perf.c $(SRC_DIR)/boot_perf.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_boot_perf.c $(SRC_DIR)/boot_perf.c

$(BUILD_DIR)/test_boot_perf_safety: $(TEST_DIR)/test_boot_perf_safety.c $(SRC_DIR)/boot_perf.c $(SRC_DIR)/boot_perf.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_boot_perf_safety.c $(SRC_DIR)/boot_perf.c

$(BUILD_DIR)/test_main_boot_perf: $(TEST_DIR)/test_main_boot_perf.c $(SRC_DIR)/main.c $(SRC_DIR)/boot_perf.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -DTEST_BUILD -o $@ $(TEST_DIR)/test_main_boot_perf.c $(SRC_DIR)/main.c $(SRC_DIR)/boot_perf.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

$(BUILD_DIR)/test_main_boot_metrics_getter: $(TEST_DIR)/test_main_boot_metrics_getter.c $(SRC_DIR)/main.c $(SRC_DIR)/boot_perf.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -DTEST_BUILD -o $@ $(TEST_DIR)/test_main_boot_metrics_getter.c $(SRC_DIR)/main.c $(SRC_DIR)/boot_perf.c $(SRC_DIR)/emulator_loop.c $(SRC_DIR)/boot_splash.c $(SRC_DIR)/cartridge_layout.c $(SRC_DIR)/disk_sector_layout.c $(SRC_DIR)/disk_trap.c $(SRC_DIR)/apple2_mem.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/bunnie_audio.c $(SRC_DIR)/video_apple2.c $(SRC_DIR)/bio_display.c $(SRC_DIR)/lores_apple2.c

# Full 6502 functional-correctness integration test against Klaus Dormann's
# 6502_functional_test.bin (see tests/test_functional_suite.c). Promoted
# into the main `test` target above -- it PASSES (all documented NMOS 6502
# opcodes + flags, including BCD decimal-mode ADC/SBC, verified against
# Klaus Dormann's suite; trapped at the real success address $3469).
# `make test-functional` remains as a standalone convenience alias for
# running just this one check without the rest of the suite.
# Requires the fixture binary: run tests/fetch_functional_test.sh once
# (downloads to tests/fixtures/, gitignored) -- `make test` does this
# automatically.
.PHONY: test-functional
test-functional: $(BUILD_DIR)/test_functional_suite
	@./tests/fetch_functional_test.sh
	@$(BUILD_DIR)/test_functional_suite

$(BUILD_DIR)/test_functional_suite: $(TEST_DIR)/test_functional_suite.c $(SRC_DIR)/cpu6502.c $(SRC_DIR)/cpu6502.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_functional_suite.c $(SRC_DIR)/cpu6502.c

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
	@python3 tools/run_lores_palette_biosim.py

clean:
	rm -rf $(BUILD_DIR)
