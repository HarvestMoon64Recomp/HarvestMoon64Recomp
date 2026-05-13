#include "patches.h"
#include "graphics.h"
#include "os.h"

int dummy = 1;
int dummyBSS;

#define OPENING_LOGOS_CUTSCENE 1456
#define MAIN_LOOP_CALLBACK_FUNCTION_TABLE_SIZE 57

extern volatile u8 stepMainLoop;
extern volatile u16 engineStateFlags;
extern volatile u16 gCutsceneIndex;
extern volatile u16 mainLoopCallbackCurrentIndex;
extern void (*mainLoopCallbacksTable[MAIN_LOOP_CALLBACK_FUNCTION_TABLE_SIZE])();
extern volatile u16 D_80182BA0;
extern volatile u16 D_8020564C;
extern volatile u8 D_80237A04;
extern volatile u8 retraceCount;

void yield_self_1ms(void);
void func_80026284(void);
void func_8004DEC8(void);
void func_800293B8(void);
void nuGfxDisplayOn(void);
void nuGfxDisplayOff(void);
void resetBitmaps(void);
void updateAudio(void);
void resetSceneNodeCounter(void);
void updateCutsceneExecutors(void);
void updateEntities(void);
void updateMapController(void);
void updateMapGraphics(void);
void updateNumberSprites(void);
void updateSprites(void);
void dmaSprites(void);
void updateBitmaps(void);
void updateMessageBox(void);
void updateDialogues(void);

void update_intro_visual_scale(void);
void update_2d_widescreen_sprite_flags(void);

RECOMP_PATCH void mainLoop(void) {
    stepMainLoop = FALSE;
    engineStateFlags = 1;

    // Wait 60 frames until engineStateFlags |= 2.
    func_80026284();

    // Toggle flags.
    func_8004DEC8();

    D_80182BA0 = 1;
    D_8020564C = 0;

    while (TRUE) {
        nuGfxDisplayOn();

        while (engineStateFlags & 1) {
            // Prevent recomp threads from busy-waiting forever.
            while (stepMainLoop == FALSE) {
                yield_self_1ms();
            }

            if (!D_8020564C) {
                D_80182BA0 = 1;
                mainLoopCallbacksTable[mainLoopCallbackCurrentIndex]();
                D_8020564C = D_80182BA0;
            }

            D_8020564C -= 1;

            update_intro_visual_scale();
            resetBitmaps();
            updateAudio();
            resetSceneNodeCounter();
            updateCutsceneExecutors();
            updateEntities();
            updateMapController();
            if (gCutsceneIndex != OPENING_LOGOS_CUTSCENE) {
                updateMapGraphics();
            }
            updateNumberSprites();
            update_2d_widescreen_sprite_flags();
            updateSprites();
            dmaSprites();
            updateBitmaps();
            updateMessageBox();
            updateDialogues();
            func_800293B8();

            stepMainLoop = FALSE;
            D_80237A04 = retraceCount;
        }

        nuGfxDisplayOff();
    }
}
