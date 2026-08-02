#include <stdio.h>
#include <stdint.h>

int main(void) {
    FILE *f = fopen("/Volumes/T9/ryan-homedir/Downloads/Apple_DOS_3.3_Master.dsk", "rb");
    if (!f) {
        perror("fopen");
        return 1;
    }
    uint8_t sector0[256];
    size_t got = fread(sector0, 1, 256, f);
    fclose(f);

    printf("Sector 0 read %zu bytes:\n", got);
    for (int i = 0; i < 256; i++) {
        printf("%02X ", sector0[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    return 0;
}
