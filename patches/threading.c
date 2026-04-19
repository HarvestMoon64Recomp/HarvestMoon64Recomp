#include "patches.h"

extern volatile u8 stepMainLoop;
extern volatile u16 engineStateFlags;
extern volatile u32 nuGfxTaskSpool;

void yield_self_1ms(void);

RECOMP_PATCH void nuGfxTaskAllEndWait(void) {
    // Prevent recomp threads from locking up in a tight spin loop.
    while (nuGfxTaskSpool) {
        yield_self_1ms();
    }
}

RECOMP_PATCH void func_80026284(void) {
    u32 count = 1;

    goto loop_end;

    while (!(engineStateFlags & 2)) {
        u16 counter = 1;
        u16 current_count;

        if (count != 0) {
            do {
                stepMainLoop = FALSE;

                while (stepMainLoop == FALSE) {
                    yield_self_1ms();
                }

                current_count = counter;
                counter++;
            } while (current_count < count);
        }

loop_end:
        ;
    }
}
