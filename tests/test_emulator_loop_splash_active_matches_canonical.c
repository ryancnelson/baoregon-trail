/*
 * tests/test_emulator_loop_splash_active_matches_canonical.c -- lock
 * baoregon_emulator_is_splash_menu_active() and
 * baoregon_emulator_is_in_splash_menu() as permanently equivalent
 * (delegating alias, not two independently-maintained copies).
 */
#include <assert.h>
#include <stdio.h>

#include "../src/emulator_loop.h"
#include "../src/apple2_mem.h"

static void assert_both_getters_agree(const char *label) {
    int canonical = baoregon_emulator_is_in_splash_menu();
    int alias = baoregon_emulator_is_splash_menu_active();
    if (canonical != alias) {
        fprintf(stderr, "FAIL: %s: is_in_splash_menu()=%d but "
                        "is_splash_menu_active()=%d -- the two names "
                        "have drifted apart\n", label, canonical, alias);
        assert(0);
    }
    printf("PASS: %s (both report %d)\n", label, canonical);
}

int main(void) {
    baoregon_emulator_init();
    assert_both_getters_agree("after init");

    apple2_mem_set_button_state(2, 1);
    baoregon_emulator_poll_input();
    apple2_mem_set_button_state(2, 0);
    baoregon_emulator_poll_input();
    assert_both_getters_agree("after entering a game via SELECT");

    baoregon_emulator_reset_to_splash();
    assert_both_getters_agree("after reset_to_splash");

    printf("All tests passed.\n");
    return 0;
}
