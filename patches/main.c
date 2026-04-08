#include "patches.h"
#include "os.h"

int dummy = 1;
int dummyBSS;

extern volatile u8 stepMainLoop;
extern volatile u16 engineStateFlags;

void yield_self_1ms(void);
extern volatile u32	nuGfxTaskSpool;	/* number of tasks in the queue */

RECOMP_PATCH void nuGfxTaskAllEndWait(void) {
    //@recomp patch to prevent threads from locking up
    while (nuGfxTaskSpool) {
        yield_self_1ms();
    }
}

// start up before main loop
RECOMP_PATCH void func_80026284(void) {
    u32 count = 1;
    goto loop_end;
    
    // first 60 frames
    while (!(engineStateFlags & 2)) {
        
        u16 counter = 1;
        u16 currentCount;

        if (count != 0) {
            
            do {
            
                stepMainLoop = FALSE;
                //@recomp patch to prevent threads from locking up
                while (stepMainLoop == FALSE) {
                    yield_self_1ms();
                }

                currentCount = counter;
                counter++;

            } while (currentCount < count);

        }
        // func_80026248(1);

loop_end:     
        
    }

}

#define MAIN_LOOP_CALLBACK_FUNCTION_TABLE_SIZE 57
extern volatile u16 mainLoopCallbackCurrentIndex;
extern void (*mainLoopCallbacksTable[MAIN_LOOP_CALLBACK_FUNCTION_TABLE_SIZE])();

extern volatile u16 D_80182BA0;
extern volatile u16 D_8020564C;
extern void *currentGfxTaskPtr;

void nuGfxDisplayOn(void);

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
void func_800293B8(void); 
// per 60; 1 = 60 fps
extern volatile u8 frameRate;
extern volatile u8 D_80237A04;
extern volatile u8 retraceCount;
void func_8004DEC8(void);
int rand(void);
void nuGfxDisplayOff(void);

extern s32 sRandNext;

RECOMP_PATCH void mainLoop(void) {

    stepMainLoop = FALSE;
    engineStateFlags = 1;
    
    // wait 60 frames until engineStateFlags |= 2
    func_80026284();

    // toggle flags
    func_8004DEC8();
    
    // could be inline func_80026230
    D_80182BA0 = 1;
    D_8020564C = 0;

    while (TRUE) {
      
        nuGfxDisplayOn();
          
        while (engineStateFlags & 1) {
            //@recomp patch to prevent threads from locking up
            while (stepMainLoop == FALSE) {
                yield_self_1ms();
            }
            
            if (!D_8020564C) { 
              
              D_80182BA0 = 1;
              
              // handle specific logic depending on game mode/screen
              mainLoopCallbacksTable[mainLoopCallbackCurrentIndex](); 

              D_8020564C = D_80182BA0; 

            } 
            
            D_8020564C -= 1;    

            // dead code
            if (D_8020564C) {

            }
            
            resetBitmaps();
            updateAudio(); 
            resetSceneNodeCounter();
            updateCutsceneExecutors();
            updateEntities();
            updateMapController();
            updateMapGraphics();
            updateNumberSprites();
            updateSprites(); 
            dmaSprites(); 
            updateBitmaps(); 
            updateMessageBox(); 
            updateDialogues();

            // no op
            // shelved or debug code
            func_800293B8(); 

            stepMainLoop = FALSE; 

            // unused
            D_80237A04 = retraceCount;
              
            // update RNG seed
            // TODO: implement this somehow
            // the name `rand` conflicts with other things named rand
            // and then the sRandNext is required to be static...so you cant reference it directly
            //rand();
            //sRandNext = sRandNext * 1103515245 + 12345;
        }
        nuGfxDisplayOff();
    }
    
}

extern volatile u32 gGraphicsBufferIndex;
extern volatile u8 gfxTaskNo;

typedef struct Vec3f {
    f32 x;
    f32 y;
    f32 z;
} Vec3f;

typedef struct {
    f32 r, g, b, a;
} Vec4f;

typedef struct {
    Mtx projection;
    Mtx viewing;
    f32 l;
    f32 r;
    f32 t;
    f32 b;
    f32 n;
    f32 f;
    f32 fov;
    f32 aspect;
    f32 near;
    f32 far;
    Vec3f eye;
    Vec3f at;
    Vec3f up;
    u8 perspectiveMode;
} Camera;

typedef struct {
    Mtx projection;
    Mtx orthographic;
    Mtx translationM;
    Mtx scale;
    Mtx rotationX;
    Mtx rotationY;
    Mtx rotationZ;
    Mtx viewing;
    Vec3f translation;
    Vec3f scaling;
    Vec3f rotation;
    u32 unknown[0x2B];
} SceneMatrices;

extern Gfx* renderSceneGraph(Gfx* dl, SceneMatrices* arg1);
extern SceneMatrices sceneMatrices[2];
extern Gfx viewportDL[3];
Gfx* setupCameraMatrices(Gfx*, Camera*, SceneMatrices*);
Gfx* clearFramebuffer(Gfx* dl);
Gfx* initRcp(Gfx*);
volatile u8 startGfxTask(void);
extern Camera gCamera;
void nuGfxTaskStart(Gfx* gfxList_ptr, u32 gfxListSize, u32 ucode, u32 flag);

#define NU_GFX_UCODE_F3DEX 0
#define NU_SC_NOSWAPBUFFER 0x0000
#define NU_SC_SWAPBUFFER 0x0001

Gfx combinedGfxList[2][0x2800]; //should be plenty large enough to hold everything

//@recomp modify the drawing logic so that everything is drawn in one task
//this allows present early to work without flickering
RECOMP_PATCH void drawFrame(void) {

    gfxTaskNo = 0;

    Gfx* dl = combinedGfxList[gGraphicsBufferIndex];

    // --- init / clear ---
    dl = initRcp(dl);
    dl = clearFramebuffer(dl);

    // --- scene ---
    gSPDisplayList(dl++, OS_K0_TO_PHYSICAL(&viewportDL));
    dl = setupCameraMatrices(dl, &gCamera, &sceneMatrices[gGraphicsBufferIndex]);
    dl = renderSceneGraph(dl, &sceneMatrices[gGraphicsBufferIndex]);

    // --- viewport ---
    gSPDisplayList(dl++, OS_K0_TO_PHYSICAL(&viewportDL));

    // --- finish ---
    gDPFullSync(dl++);
    gSPEndDisplayList(dl++);

    nuGfxTaskStart(combinedGfxList[gGraphicsBufferIndex],
                   (s32) (dl - combinedGfxList[gGraphicsBufferIndex]) * sizeof(Gfx), NU_GFX_UCODE_F3DEX,
                   NU_SC_SWAPBUFFER);

    gfxTaskNo += 1;

    gGraphicsBufferIndex ^= 1;
}