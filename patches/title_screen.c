#include "patches.h"
#include "system/globalSprites.h"

#define TITLE_SCREEN_CALLBACK 0x32
#define NAMING_SCREEN_CALLBACK 0x34
#define TITLE_TILE_WIDTH 320.0f
#define TITLE_DUPLICATE_FIRST_SLOT 162
#define TITLE_DUPLICATE_SLOT_COUNT 10
#define TITLE_DUPLICATE_LAST_SLOT (TITLE_DUPLICATE_FIRST_SLOT + TITLE_DUPLICATE_SLOT_COUNT)
#define CHECKERBOARD_BACKGROUND 0x80
#define LANDSCAPE_BACKGROUND 0x90
#define CHECKERBOARD_DUPLICATE_LEFT_SLOT 190
#define CHECKERBOARD_DUPLICATE_RIGHT_SLOT 191
#define CHECKERBOARD_OVERLAP_X 14.0f
#define CHECKERBOARD_DUPLICATE_OFFSET_X (TITLE_TILE_WIDTH - CHECKERBOARD_OVERLAP_X)
#define TITLE_SCREEN_CLOUD_1_1 0x54
#define TITLE_SCREEN_CLOUD_1_2 0x55
#define TITLE_SCREEN_FRONT_GRASS_2_1 0x4C
#define TITLE_SCREEN_FAR_GRASS_3_1 0x4D
#define TITLE_SCREEN_FAR_GRASS_2_1 0x4E
#define TITLE_SCREEN_FRONT_GRASS_2_2 0x4F
#define TITLE_SCREEN_BACK_GRASS_3_2 0x50
#define TITLE_SCREEN_BACK_GRASS_2 0x51
#define TITLE_SCREEN_FAR_GRASS_1_1 0x52
#define TITLE_SCREEN_FAR_GRASS_1_2 0x53

typedef struct {
    u16 source_index;
    f32 duplicate_offset_x;
} TitleTileSource;

static const TitleTileSource title_tiling_source_sprites[] = {
    { TITLE_SCREEN_CLOUD_1_1, TITLE_TILE_WIDTH },       { TITLE_SCREEN_CLOUD_1_2, -TITLE_TILE_WIDTH },
    { TITLE_SCREEN_FRONT_GRASS_2_1, TITLE_TILE_WIDTH }, { TITLE_SCREEN_FAR_GRASS_3_1, TITLE_TILE_WIDTH },
    { TITLE_SCREEN_FAR_GRASS_2_1, TITLE_TILE_WIDTH },   { TITLE_SCREEN_FRONT_GRASS_2_2, -TITLE_TILE_WIDTH },
    { TITLE_SCREEN_BACK_GRASS_3_2, -TITLE_TILE_WIDTH }, { TITLE_SCREEN_BACK_GRASS_2, -TITLE_TILE_WIDTH },
    { TITLE_SCREEN_FAR_GRASS_1_1, TITLE_TILE_WIDTH },   { TITLE_SCREEN_FAR_GRASS_1_2, -TITLE_TILE_WIDTH }
};

extern volatile u16 mainLoopCallbackCurrentIndex;

static void copy_sprite_object(SpriteObject* dst, SpriteObject* src) {
    u16 i;

    dst->animationIndexPtr = src->animationIndexPtr;
    dst->animationDataPtr = src->animationDataPtr;
    dst->spritesheetIndexPtr = src->spritesheetIndexPtr;
    dst->paletteIndexPtr = src->paletteIndexPtr;
    dst->spriteToPaletteMappingPtr = src->spriteToPaletteMappingPtr;
    dst->texturePtr[0] = src->texturePtr[0];
    dst->texturePtr[1] = src->texturePtr[1];
    dst->romTexturePtr = src->romTexturePtr;
    dst->animationFrameMetadataPtr = src->animationFrameMetadataPtr;
    dst->bitmapMetadataPtr = src->bitmapMetadataPtr;
    dst->animationFrameMetadata = src->animationFrameMetadata;
    dst->viewSpacePosition = src->viewSpacePosition;
    dst->scale = src->scale;
    dst->rotation = src->rotation;
    dst->baseRGBA = src->baseRGBA;
    dst->currentRGBA = src->currentRGBA;
    dst->targetRGBA = src->targetRGBA;
    dst->deltaRGBA = src->deltaRGBA;
    dst->currentAnimationFrame = src->currentAnimationFrame;
    dst->frameTickCounter = src->frameTickCounter;
    dst->audioTrigger = src->audioTrigger;
    dst->paletteIndex = src->paletteIndex;
    for (i = 0; i < 2; i++) {
        dst->animationCounter[i] = src->animationCounter[i];
    }
    dst->renderingFlags = src->renderingFlags;
    dst->stateFlags = src->stateFlags;
}

static void clear_title_duplicate_slots(void) {
    u16 slot_index;

    for (slot_index = TITLE_DUPLICATE_FIRST_SLOT; slot_index < TITLE_DUPLICATE_LAST_SLOT; slot_index++) {
        globalSprites[slot_index].stateFlags = 0;
    }
}

static void clear_checkerboard_duplicate_slots(void) {
    globalSprites[CHECKERBOARD_DUPLICATE_LEFT_SLOT].stateFlags = 0;
    globalSprites[CHECKERBOARD_DUPLICATE_RIGHT_SLOT].stateFlags = 0;
}

static void update_checkerboard_duplicate_slot(u16 slot_index,
                                               SpriteObject* source_sprite,
                                               u16 source_rendering_flags,
                                               f32 offset_x) {
    copy_sprite_object(&globalSprites[slot_index], source_sprite);
    globalSprites[slot_index].viewSpacePosition.x += offset_x;
    globalSprites[slot_index].renderingFlags =
        source_rendering_flags ^ SPRITE_RENDERING_FLIP_HORIZONTAL;
}

static void update_checkerboard_widescreen_tiles(void) {
    SpriteObject* source_sprite;
    u16 source_rendering_flags;

    if (!(globalSprites[CHECKERBOARD_BACKGROUND].stateFlags & SPRITE_ACTIVE)) {
        clear_checkerboard_duplicate_slots();
        return;
    }

    source_sprite = &globalSprites[CHECKERBOARD_BACKGROUND];
    source_rendering_flags = source_sprite->renderingFlags;

    if ((source_sprite->scale.x != 2.0f) || (source_sprite->scale.y != 2.0f) ||
        (source_sprite->viewSpacePosition.x != 0.0f) || (source_sprite->viewSpacePosition.y != 0.0f)) {
        clear_checkerboard_duplicate_slots();
        return;
    }

    update_checkerboard_duplicate_slot(CHECKERBOARD_DUPLICATE_LEFT_SLOT,
                                       source_sprite,
                                       source_rendering_flags,
                                       -CHECKERBOARD_DUPLICATE_OFFSET_X);
    update_checkerboard_duplicate_slot(CHECKERBOARD_DUPLICATE_RIGHT_SLOT,
                                       source_sprite,
                                       source_rendering_flags,
                                       CHECKERBOARD_DUPLICATE_OFFSET_X);
}

static void update_naming_background_widescreen_tiles(void) {
    SpriteObject* source_sprite;
    u16 source_rendering_flags;

    if (!(globalSprites[LANDSCAPE_BACKGROUND].stateFlags & SPRITE_ACTIVE)) {
        clear_checkerboard_duplicate_slots();
        return;
    }

    source_sprite = &globalSprites[LANDSCAPE_BACKGROUND];
    source_rendering_flags = source_sprite->renderingFlags;

    if ((source_sprite->scale.x != 2.0f) || (source_sprite->scale.y != 2.0f) ||
        (source_sprite->viewSpacePosition.x != 0.0f) || (source_sprite->viewSpacePosition.y != 0.0f)) {
        clear_checkerboard_duplicate_slots();
        return;
    }

    update_checkerboard_duplicate_slot(CHECKERBOARD_DUPLICATE_LEFT_SLOT,
                                       source_sprite,
                                       source_rendering_flags,
                                       -CHECKERBOARD_DUPLICATE_OFFSET_X);
    update_checkerboard_duplicate_slot(CHECKERBOARD_DUPLICATE_RIGHT_SLOT,
                                       source_sprite,
                                       source_rendering_flags,
                                       CHECKERBOARD_DUPLICATE_OFFSET_X);
}

static void update_title_widescreen_tiles(void) {
    u16 i;
    SpriteObject* source_sprite;

    for (i = 0; i < (sizeof(title_tiling_source_sprites) / sizeof(title_tiling_source_sprites[0])); i++) {
        u16 slot_index = TITLE_DUPLICATE_FIRST_SLOT + i;

        if (!(globalSprites[title_tiling_source_sprites[i].source_index].stateFlags & SPRITE_ACTIVE)) {
            globalSprites[slot_index].stateFlags = 0;
            continue;
        }

        source_sprite = &globalSprites[title_tiling_source_sprites[i].source_index];
        copy_sprite_object(&globalSprites[slot_index], source_sprite);
        globalSprites[slot_index].viewSpacePosition.x += title_tiling_source_sprites[i].duplicate_offset_x;
    }
}

void update_title_screen_widescreen_tiles(void) {
    if (mainLoopCallbackCurrentIndex == TITLE_SCREEN_CALLBACK) {
        clear_checkerboard_duplicate_slots();
        update_title_widescreen_tiles();
        return;
    }

    if (mainLoopCallbackCurrentIndex == NAMING_SCREEN_CALLBACK) {
        clear_title_duplicate_slots();
        update_naming_background_widescreen_tiles();
        return;
    }

    clear_title_duplicate_slots();
    update_checkerboard_widescreen_tiles();
}
