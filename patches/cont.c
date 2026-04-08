#include "patches.h"
#include "common.h"
#include "system/controller.h"
#include "mainproc.h"

extern u16 buttonRepeatRate;
extern u16 buttonRepeatModeTriggerDelayFrames;

extern OSMesgQueue nuSiMesgQ;
extern OSContPad nuContData[NU_CONT_MAXCONTROLLERS];


RECOMP_PATCH u8 nuContInit(void) {

    u8 pattern;

    // Initialize the SI manager (creates the SI thread, calls osContInit internally)
    pattern = nuSiMgrInit();

    // Initialize the controller, pak, and rumble managers
    nuContMgrInit();
    nuContPakMgrInit();
    nuContRmbMgrInit();

    // @recomp Force controller 0 to be recognized as connected.
    nuContStatus[0].errno = 0;
    
    // Make sure the pattern reflects controller 0 is connected
    pattern |= 1;

    return pattern;
}

// @recomp Patch nuContDataGetExAll to directly poll input from the runtime.
RECOMP_PATCH void nuContDataGetExAll(NUContData *contdata) {

    u16 button;
    u32 cnt;

    osContStartReadData(&nuSiMesgQ);
    osRecvMesg(&nuSiMesgQ, (OSMesg*)0, OS_MESG_BLOCK);
    osContGetReadData(nuContData);

    for (cnt = 0; cnt < NU_CONT_MAXCONTROLLERS; cnt++) {

        button = contdata[cnt].button;

        // Copy OSContPad fields into NUContData
        contdata[cnt].button  = nuContData[cnt].button;
        contdata[cnt].stick_x = nuContData[cnt].stick_x;
        contdata[cnt].stick_y = nuContData[cnt].stick_y;
        contdata[cnt].errno   = nuContData[cnt].errno;

        // Calculate trigger
        contdata[cnt].trigger = nuContData[cnt].button & (~button);
    }
}

RECOMP_PATCH void readControllerData(void) {
    u8 j;

    nuContDataGetExAll(contData);

    controllers[0].analogStick.rawX = contData[0].stick_x;
    controllers[0].analogStick.rawY = contData[0].stick_y;
    controllers[0].button = contData[0].button;

    calculateAnalogStickDirection(0);

    controllers[0].buttonPressed = (controllers[0].button ^ controllers[0].buttonHeld) & controllers[0].button;
    controllers[0].buttonReleased = (controllers[0].button ^ controllers[0].buttonHeld) & controllers[0].buttonHeld;
    controllers[0].buttonHeld = controllers[0].button;

    for (j = 0; j < 24; j++) {
        if ((controllers[0].button >> j) & 1) {
            if ((controllers[0].buttonRepeatState >> j) & 1) {
                if (controllers[0].buttonFrameCounters[j] > buttonRepeatRate) {
                    controllers[0].buttonRepeat |= ((controllers[0].button >> j) & 1) << j;
                    controllers[0].buttonFrameCounters[j] = 0;
                } else {
                    controllers[0].buttonRepeat &= ~(1 << j);
                }
            } else if (controllers[0].buttonFrameCounters[j] > buttonRepeatModeTriggerDelayFrames) {
                controllers[0].buttonRepeat |= ((controllers[0].button >> j) & 1) << j;
                controllers[0].buttonRepeatState |= ((controllers[0].button >> j) & 1) << j;
                controllers[0].buttonFrameCounters[j] = 0;
            } else {
                controllers[0].buttonRepeat = controllers[0].buttonPressed;
            }
            controllers[0].buttonFrameCounters[j]++;
        } else {
            controllers[0].buttonFrameCounters[j] = 0;
            controllers[0].buttonRepeat &= ~(1 << j);
            controllers[0].buttonRepeatState &= ~(1 << j);
        }
    }

    gControllers[0].button = controllers[0].button;
    gControllers[0].buttonPressed = controllers[0].buttonPressed;
    gControllers[0].buttonReleased = controllers[0].buttonReleased;
    gControllers[0].buttonHeld = controllers[0].buttonHeld;
    gControllers[0].analogStick.rawX = controllers[0].analogStick.rawX;
    gControllers[0].analogStick.rawY = controllers[0].analogStick.rawY;
    gControllers[0].analogStick.direction = controllers[0].analogStick.direction;
    gControllers[0].analogStick.magnitude = controllers[0].analogStick.magnitude;
}