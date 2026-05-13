#include "patches.h"

extern volatile u8 stepMainLoop;
extern volatile u16 engineStateFlags;
extern volatile u32 nuGfxTaskSpool;
extern volatile u8 frameCount;
extern volatile u8 frameRate;
extern volatile u8 drawnFrameCount;
extern volatile u8 previousDrawnFrameCount;
extern volatile u8 framebufferCount;
extern volatile u8 D_80222730;

void yield_self_1ms(void);
void drawFrame(void);
void nuGfxSwapCfb(void* gfxTask);

#define HM64_COMBINED_FRAME_TASK_COUNT 1

static volatile u8 hm64_frame_swap_complete = TRUE;

RECOMP_PATCH void nuGfxTaskAllEndWait(void) {
    // @recomp Prevent recomp threads from locking up in a tight spin loop.
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

RECOMP_PATCH void handleGraphicsUpdate(int pendingGfx) {
    u8 temp;

    if ((frameCount % frameRate) == 0) {
        if (frameCount > 59) {
            previousDrawnFrameCount = drawnFrameCount;
            drawnFrameCount = 0;

            if (previousDrawnFrameCount < (60 / frameRate)) {
                D_80222730 = 2;
            }
        }

        // @recomp One drawFrame call now emits one full graphics task, so keep only one frame queued.
        if (pendingGfx < HM64_COMBINED_FRAME_TASK_COUNT) {
            temp = stepMainLoop;

            if ((temp == FALSE) && hm64_frame_swap_complete) {
                // @recomp Keep the main loop from getting ahead of the last swapped frame.
                hm64_frame_swap_complete = FALSE;
                drawFrame();
                drawnFrameCount += 1;
                temp = drawnFrameCount;
                framebufferCount = 0;
            }
        }
    }
}

RECOMP_PATCH void gfxBufferSwap(void* gfxTask) {
    nuGfxSwapCfb(gfxTask);
    hm64_frame_swap_complete = TRUE;
}
