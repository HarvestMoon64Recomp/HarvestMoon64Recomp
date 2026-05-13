#include "patches.h"
#include "graphics.h"
#include "os.h"

typedef struct Vec3f {
    f32 x;
    f32 y;
    f32 z;
} Vec3f;

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

extern volatile u32 gGraphicsBufferIndex;
extern volatile u8 gfxTaskNo;
extern SceneMatrices sceneMatrices[2];
extern Gfx viewportDL[3];
extern Camera gCamera;

extern Gfx* renderSceneGraph(Gfx* dl, SceneMatrices* arg1);

Gfx* setupCameraMatrices(Gfx* dl, Camera* camera, SceneMatrices* matrices);
Gfx* clearFramebuffer(Gfx* dl);
Gfx* initRcp(Gfx* dl);
void nuGfxTaskStart(Gfx* gfxList_ptr, u32 gfxListSize, u32 ucode, u32 flag);

#define NU_GFX_UCODE_F3DEX 0
#define NU_SC_SWAPBUFFER 0x0001

static Gfx combinedGfxList[2][0x2800];

// @recomp Draw everything in a single task so present-early does not flicker.
RECOMP_PATCH void drawFrame(void) {
    Gfx* dl;

    gfxTaskNo = 0;
    dl = combinedGfxList[gGraphicsBufferIndex];
    gEXEnable(dl++);

    dl = initRcp(dl);
    dl = clearFramebuffer(dl);

    gSPDisplayList(dl++, OS_K0_TO_PHYSICAL(&viewportDL));
    dl = setupCameraMatrices(dl++, &gCamera, &sceneMatrices[gGraphicsBufferIndex]);
    dl = renderSceneGraph(dl++, &sceneMatrices[gGraphicsBufferIndex]);
    gSPDisplayList(dl++, OS_K0_TO_PHYSICAL(&viewportDL));

    gDPFullSync(dl++);
    gSPEndDisplayList(dl++);

    nuGfxTaskStart(
        combinedGfxList[gGraphicsBufferIndex],
        (s32)(dl - combinedGfxList[gGraphicsBufferIndex]) * sizeof(Gfx),
        NU_GFX_UCODE_F3DEX,
        NU_SC_SWAPBUFFER
    );

    gfxTaskNo += 1;
    gGraphicsBufferIndex ^= 1;
}
