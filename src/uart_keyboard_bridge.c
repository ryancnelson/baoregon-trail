#include "uart_keyboard_bridge.h"
#include "apple2_mem.h"
#include <stddef.h>

int uart_keyboard_bridge_poll(uart_rx_ready_fn is_ready, uart_rx_read_fn read_byte) {
    if (!is_ready || !read_byte) {
        return 0;
    }
    int drained = 0;
    while (is_ready()) {
        uint8_t ch = read_byte();
        if (ch == '\n') {
            ch = '\r';
        }
        apple2_mem_inject_key(ch);
        drained++;
    }
    return drained;
}
