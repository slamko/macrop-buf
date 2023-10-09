#include <stdint.h>
#include <stdio.h>

void get(char *cur_buf, uint64_t work_val) {
    uint64_t v = work_val;

    uint64_t mask = 0xFFFFFFFF;
    uint64_t base = 32;
    uint8_t res = 0;

    for (size_t i = 0; i < 3; i++) {
        uint64_t cur_mask = mask << base;
        uint8_t delta = (base * !!(v & cur_mask));
        v = v >> delta;
        res += delta;
        base /= 2;
        mask = ~(0xFFFFFFFF << base);
    }

    res = (res / 8) + 1;
    printf("Res: %u\n", res);

    uint8_t num_bytes = res;
    uint8_t add = 0;
    if (res == 8) {
        num_bytes = 10;
    } else {
        uint16_t v2 = v << res;

        if (v2 & 0xFF00 || v2 & 0x80) {
            num_bytes = res + 1;
        }

        if (v2 & 0x80) {
            add = 1;
        }
    }

    for (size_t i = num_bytes - 1; i > 0; i--) {
        uint16_t full = (((work_val >> ((8 * i) - 8)) << (i + add)) >> 8); 
        uint8_t f = full & 0xff;

        if (res == 8 && num_bytes - i == 1) {
            f = full >> 8;
        } else {
            f |= !!(num_bytes - i > 1) << 7;
        }

        cur_buf[i] = f;
    }

    cur_buf[0] = (work_val & 0xFF) | (!!(num_bytes > 1) << 7);
}


int main() {
    char buf[10] = {0};

    get(buf, 0x8400000000000000);

    for (size_t i = 0; i < sizeof buf; i++) {
        uint8_t x = buf[i];
        printf("%x\n", x);
    }

    
    return 0;
}
