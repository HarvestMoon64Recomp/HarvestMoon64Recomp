#include "patches.h"
#include "graphics.h"
#include "game/cutscenes.h"
#include "system/cutscene.h"
#include "system/globalSprites.h"

#define OPENING_LOGOS_CUTSCENE 1456
#define FUNERAL_INTRO_CUTSCENE 1450
#define OPENING_LOGO_BACKGROUND_SPRITE 161
#define FUNERAL_INTRO_BACKGROUND_SPRITE 128
#define FUNERAL_INTRO_FIRST_LEAF_SPRITE 129
#define FUNERAL_INTRO_LAST_LEAF_SPRITE 134
#define INTRO_WIDESCREEN_OVERSCAN_MILLI 1030
#define FUNERAL_FIRST_LEAF_HIDE_X 80.0f
#define INTRO_BACKGROUND_BASE_SCALE 20.0f

static s32 intro_visual_scale_milli = 1000;
extern CutsceneExecutor cutsceneExecutors[MAX_BYTECODE_EXECUTORS];

static f32 widen_intro_view_x(f32 x) {
    return x * (f32) intro_visual_scale_milli / 1000.0f;
}

static u16 is_funeral_intro_leaf_sprite(u16 index) {
    return ((gCutsceneFlags & CUTSCENE_ACTIVE) != 0) && (gCutsceneIndex == FUNERAL_INTRO_CUTSCENE) &&
           (index >= FUNERAL_INTRO_FIRST_LEAF_SPRITE) && (index <= FUNERAL_INTRO_LAST_LEAF_SPRITE);
}

static bool is_opening_logo_cutscene_active(void) {
    return ((gCutsceneFlags & CUTSCENE_ACTIVE) != 0) && (gCutsceneIndex == OPENING_LOGOS_CUTSCENE);
}

static bool is_funeral_intro_cutscene_active(void) {
    return ((gCutsceneFlags & CUTSCENE_ACTIVE) != 0) && (gCutsceneIndex == FUNERAL_INTRO_CUTSCENE);
}

static bool is_intro_background_sprite(u16 index) {
    return (index == OPENING_LOGO_BACKGROUND_SPRITE) || (index == FUNERAL_INTRO_BACKGROUND_SPRITE);
}

static void stretch_intro_background_scale(u16 sprite_index, f32* x, f32* y, f32* z) {
    f32 aspect_scale;

    if (!is_intro_background_sprite(sprite_index)) {
        return;
    }

    aspect_scale = (f32) intro_visual_scale_milli / 1000.0f;

    if ((*x >= INTRO_BACKGROUND_BASE_SCALE) && (*y >= INTRO_BACKGROUND_BASE_SCALE)) {
        *x *= aspect_scale;
        *z = 0.0f;
    }
}

void update_intro_visual_scale(void) {
    u32 width = 0;
    u32 height = 0;
    s32 scale_milli = intro_visual_scale_milli;

    recomp_get_window_resolution(&width, &height);

    if ((width > 0) && (height > 0)) {

        scale_milli = (s32) ((width * 3000U) / (height * 4U));

        scale_milli = (scale_milli * INTRO_WIDESCREEN_OVERSCAN_MILLI) / 1000;

        if (scale_milli < 1000) {
            scale_milli = 1000;
        }
    }

    intro_visual_scale_milli = scale_milli;
}

void update_intro_widescreen_scale(void) {
    f32 x = INTRO_BACKGROUND_BASE_SCALE;
    f32 y = INTRO_BACKGROUND_BASE_SCALE;
    f32 z = 0.0f;

    if (is_opening_logo_cutscene_active() &&
        (globalSprites[OPENING_LOGO_BACKGROUND_SPRITE].stateFlags & SPRITE_ACTIVE)) {
        stretch_intro_background_scale(OPENING_LOGO_BACKGROUND_SPRITE, &x, &y, &z);
        setSpriteScale(OPENING_LOGO_BACKGROUND_SPRITE, x, y, z);
    }

    if (is_funeral_intro_cutscene_active() &&
        (globalSprites[FUNERAL_INTRO_BACKGROUND_SPRITE].stateFlags & SPRITE_ACTIVE)) {
        x = INTRO_BACKGROUND_BASE_SCALE;
        y = INTRO_BACKGROUND_BASE_SCALE;
        z = 0.0f;
        stretch_intro_background_scale(FUNERAL_INTRO_BACKGROUND_SPRITE, &x, &y, &z);
        setSpriteScale(FUNERAL_INTRO_BACKGROUND_SPRITE, x, y, z);
    }
}

RECOMP_PATCH bool setSpriteViewSpacePosition(u16 index, f32 x, f32 y, f32 z) {
    bool result = FALSE;

    if (index < MAX_SPRITES) {
        if (globalSprites[index].stateFlags & SPRITE_ACTIVE) {
            if (is_funeral_intro_leaf_sprite(index)) {
                // Stretch the funeral intro leaves visually without changing cutscene logic.
                x = widen_intro_view_x(x);

                if ((index == FUNERAL_INTRO_FIRST_LEAF_SPRITE) && (y >= 96.0f)) {
                    f32 hidden_x = widen_intro_view_x(200.0f) + FUNERAL_FIRST_LEAF_HIDE_X;

                    if (x < hidden_x) {
                        x = hidden_x;
                    }
                }
            }

            globalSprites[index].viewSpacePosition.x = x;
            globalSprites[index].viewSpacePosition.y = y;
            globalSprites[index].viewSpacePosition.z = z;
            result = TRUE;
        }
    }

    return result;
}

RECOMP_PATCH void cutsceneHandlerSetSpriteScale(u16 index) {
    cutsceneExecutors[index].bytecodePtr += 2;

    cutsceneExecutors[index].scale.x = *(s16*) cutsceneExecutors[index].bytecodePtr;
    cutsceneExecutors[index].bytecodePtr += 2;

    cutsceneExecutors[index].scale.y = *(s16*) cutsceneExecutors[index].bytecodePtr;
    cutsceneExecutors[index].bytecodePtr += 2;

    cutsceneExecutors[index].scale.z = *(s16*) cutsceneExecutors[index].bytecodePtr;
    cutsceneExecutors[index].bytecodePtr += 2;

    if (cutsceneExecutors[index].flags & CUTSCENE_SPRITE_ASSET) {
        if ((is_opening_logo_cutscene_active() || is_funeral_intro_cutscene_active()) &&
            is_intro_background_sprite(cutsceneExecutors[index].assetIndex)) {
            stretch_intro_background_scale(cutsceneExecutors[index].assetIndex,
                                           &cutsceneExecutors[index].scale.x,
                                           &cutsceneExecutors[index].scale.y,
                                           &cutsceneExecutors[index].scale.z);
        }

        setSpriteScale(cutsceneExecutors[index].assetIndex,
                       cutsceneExecutors[index].scale.x,
                       cutsceneExecutors[index].scale.y,
                       cutsceneExecutors[index].scale.z);
    }
}
