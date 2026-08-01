/*
 * RED-then-refactor test: apple2_mem_reset() currently duplicates the
 * exact zeroing logic that apple2_mem_clear_button_states() and
 * apple2_mem_clear_annunciator_states() already implement, as two
 * separate copy-pasted blocks. This is the same drift-risk pattern
 * already caught and fixed once in emulator_loop.c's
 * init()/reset_to_splash() duplication (see commit 65114df) -- if
 * someone adds a new button/annunciator-related field, it's easy to
 * update one of the three copies (apple2_mem_reset(),
 * clear_button_states(), clear_annunciator_states()) and forget the
 * others. This test doesn't assert anything new about behavior (it's
 * a straightforward regression lock), but its purpose is to make the
 * refactor to share one implementation safe: confirms
 * apple2_mem_reset() clears button/annunciator state exactly as
 * apple2_mem_clear_button_states()/apple2_mem_clear_annunciator_states()
 * do, before and after refactoring apple2_mem_reset() to call them
 * directly instead of duplicating the zeroing inline.
 */
#include <assert.h>
#include <stdio.h>
#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"

static void test_reset_clears_buttons_and_annunciators_same_as_dedicated_helpers(void) {
    apple2_mem_reset();

    apple2_mem_set_button_state(0, 1);
    apple2_mem_set_button_state(1, 1);
    apple2_mem_set_button_state(2, 1);
    write6502(0xC059, 0); /* AN0 on */
    write6502(0xC05B, 0); /* AN1 on */
    write6502(0xC05D, 0); /* AN2 on */
    write6502(0xC05F, 0); /* AN3 on */

    assert(apple2_mem_get_button_state(0) == 1);
    assert(apple2_mem_get_button_state(1) == 1);
    assert(apple2_mem_get_button_state(2) == 1);
    assert(apple2_mem_get_annunciator_state(0) == 1);
    assert(apple2_mem_get_annunciator_state(1) == 1);
    assert(apple2_mem_get_annunciator_state(2) == 1);
    assert(apple2_mem_get_annunciator_state(3) == 1);

    apple2_mem_reset();

    assert(apple2_mem_get_button_state(0) == 0);
    assert(apple2_mem_get_button_state(1) == 0);
    assert(apple2_mem_get_button_state(2) == 0);
    assert(apple2_mem_get_annunciator_state(0) == 0);
    assert(apple2_mem_get_annunciator_state(1) == 0);
    assert(apple2_mem_get_annunciator_state(2) == 0);
    assert(apple2_mem_get_annunciator_state(3) == 0);

    printf("PASS: test_reset_clears_buttons_and_annunciators_same_as_dedicated_helpers\n");
}

int main(void) {
    test_reset_clears_buttons_and_annunciators_same_as_dedicated_helpers();
    printf("All tests passed.\n");
    return 0;
}
