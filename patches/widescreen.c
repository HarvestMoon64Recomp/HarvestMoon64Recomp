#include "patches.h"
#include "game/cutscenes.h"
#include "system/globalSprites.h"

// Widescreen sprite tagging
#define TITLE_SCREEN_CALLBACK  0x32
#define NAMING_SCREEN_CALLBACK 0x34

#define CHECKERBOARD_BACKGROUND         0x80
#define LANDSCAPE_BACKGROUND            0x90
#define OPENING_LOGO_BACKGROUND_SPRITE  161
#define FUNERAL_INTRO_BACKGROUND_SPRITE 128
#define SOWING_FESTIVAL_MOUNTAIN_BACKGROUND_SPRITE 96
#define SOWING_FESTIVAL_SCROLLING_BACKGROUND_1     150
#define SOWING_FESTIVAL_SCROLLING_BACKGROUND_2     151
#define HORSE_RACE_BACKGROUND_FIRST_SPRITE         160
#define HORSE_RACE_BACKGROUND_LAST_SPRITE          168

#define TITLE_SCREEN_CLOUD_1_1        0x54
#define TITLE_SCREEN_CLOUD_1_2        0x55
#define TITLE_SCREEN_FRONT_GRASS_2_1  0x4C
#define TITLE_SCREEN_FAR_GRASS_3_1    0x4D
#define TITLE_SCREEN_FAR_GRASS_2_1    0x4E
#define TITLE_SCREEN_FRONT_GRASS_2_2  0x4F
#define TITLE_SCREEN_BACK_GRASS_3_2   0x50
#define TITLE_SCREEN_BACK_GRASS_2     0x51
#define TITLE_SCREEN_FAR_GRASS_1_1    0x52
#define TITLE_SCREEN_FAR_GRASS_1_2    0x53

#define OPENING_LOGOS_CUTSCENE  1456
#define FUNERAL_INTRO_CUTSCENE  1450
#define SOWING_FESTIVAL_FIRST_CUTSCENE 850
#define SOWING_FESTIVAL_LAST_CUTSCENE  852
#define HORSE_RACE_FIRST_CUTSCENE 900
#define HORSE_RACE_LAST_CUTSCENE  903

#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))

typedef struct {
    u16 sprite_index;
    u16 widescreen_flags;
} WidescreenSpriteTag;

static const WidescreenSpriteTag title_screen_sprite_tags[] = {
    { TITLE_SCREEN_CLOUD_1_1,        HM64_WIDESCREEN_EXTEND_RIGHT },
    { TITLE_SCREEN_CLOUD_1_2,        HM64_WIDESCREEN_EXTEND_LEFT },
    { TITLE_SCREEN_FRONT_GRASS_2_1,  HM64_WIDESCREEN_EXTEND_RIGHT | HM64_WIDESCREEN_MIRROR_COPIES },
    { TITLE_SCREEN_FAR_GRASS_3_1,    HM64_WIDESCREEN_EXTEND_RIGHT | HM64_WIDESCREEN_MIRROR_COPIES },
    { TITLE_SCREEN_FAR_GRASS_2_1,    HM64_WIDESCREEN_EXTEND_RIGHT | HM64_WIDESCREEN_MIRROR_COPIES },
    { TITLE_SCREEN_FRONT_GRASS_2_2,  HM64_WIDESCREEN_EXTEND_LEFT  | HM64_WIDESCREEN_MIRROR_COPIES },
    { TITLE_SCREEN_BACK_GRASS_3_2,   HM64_WIDESCREEN_EXTEND_LEFT  | HM64_WIDESCREEN_MIRROR_COPIES },
    { TITLE_SCREEN_BACK_GRASS_2,     HM64_WIDESCREEN_EXTEND_LEFT  | HM64_WIDESCREEN_MIRROR_COPIES },
    { TITLE_SCREEN_FAR_GRASS_1_1,    HM64_WIDESCREEN_EXTEND_RIGHT | HM64_WIDESCREEN_MIRROR_COPIES },
    { TITLE_SCREEN_FAR_GRASS_1_2,    HM64_WIDESCREEN_EXTEND_LEFT  | HM64_WIDESCREEN_MIRROR_COPIES }
};

static const WidescreenSpriteTag sowing_festival_sprite_tags[] = {
    { SOWING_FESTIVAL_MOUNTAIN_BACKGROUND_SPRITE,
      HM64_WIDESCREEN_FULLSCREEN | HM64_WIDESCREEN_MIRROR_COPIES },
    { SOWING_FESTIVAL_SCROLLING_BACKGROUND_1,
      HM64_WIDESCREEN_FULLSCREEN | HM64_WIDESCREEN_MIRROR_COPIES },
    { SOWING_FESTIVAL_SCROLLING_BACKGROUND_2,
      HM64_WIDESCREEN_FULLSCREEN | HM64_WIDESCREEN_MIRROR_COPIES }
};

extern volatile u16 mainLoopCallbackCurrentIndex;

static inline void clear_sprite_widescreen_flags(u16 sprite_index) {
    globalSprites[sprite_index].stateFlags &= ~HM64_WIDESCREEN_FLAGS;
}

static void clear_sprite_tag_flags(const WidescreenSpriteTag* tags, u16 count) {
    for (u16 i = 0; i < count; i++) {
        clear_sprite_widescreen_flags(tags[i].sprite_index);
    }
}

static void clear_sprite_range_flags(u16 first, u16 last) {
    for (u16 i = first; i <= last; i++) {
        clear_sprite_widescreen_flags(i);
    }
}

static void clear_2d_widescreen_flags(void) {
    clear_sprite_widescreen_flags(CHECKERBOARD_BACKGROUND);
    clear_sprite_widescreen_flags(LANDSCAPE_BACKGROUND);
    clear_sprite_widescreen_flags(OPENING_LOGO_BACKGROUND_SPRITE);
    clear_sprite_widescreen_flags(FUNERAL_INTRO_BACKGROUND_SPRITE);
    clear_sprite_tag_flags(sowing_festival_sprite_tags, ARRAY_COUNT(sowing_festival_sprite_tags));
    clear_sprite_tag_flags(title_screen_sprite_tags, ARRAY_COUNT(title_screen_sprite_tags));
    clear_sprite_range_flags(HORSE_RACE_BACKGROUND_FIRST_SPRITE, HORSE_RACE_BACKGROUND_LAST_SPRITE);
}

static void tag_active_sprite(u16 sprite_index, u16 widescreen_flags) {
    if (globalSprites[sprite_index].stateFlags & SPRITE_ACTIVE) {
        globalSprites[sprite_index].stateFlags |= widescreen_flags;
    }
}

static void tag_active_sprites(const WidescreenSpriteTag* tags, u16 count) {
    for (u16 i = 0; i < count; i++) {
        tag_active_sprite(tags[i].sprite_index, tags[i].widescreen_flags);
    }
}

static void tag_active_sprite_range(u16 first, u16 last, u16 widescreen_flags) {
    for (u16 i = first; i <= last; i++) {
        tag_active_sprite(i, widescreen_flags);
    }
}

static void tag_title_callback_sprites(void) {
    switch (mainLoopCallbackCurrentIndex) {
        case TITLE_SCREEN_CALLBACK:
            tag_active_sprites(title_screen_sprite_tags, ARRAY_COUNT(title_screen_sprite_tags));
            break;

        case NAMING_SCREEN_CALLBACK:
            tag_active_sprite(LANDSCAPE_BACKGROUND, HM64_WIDESCREEN_FULLSCREEN | HM64_WIDESCREEN_MIRROR_COPIES);
            break;

        default:
            tag_active_sprite(CHECKERBOARD_BACKGROUND, HM64_WIDESCREEN_FULLSCREEN | HM64_WIDESCREEN_MIRROR_COPIES);
            break;
    }
}

static void tag_intro_background_sprites(void) {
    if (!(gCutsceneFlags & CUTSCENE_ACTIVE)) {
        return;
    }

    switch (gCutsceneIndex) {
        case SOWING_FESTIVAL_FIRST_CUTSCENE ... SOWING_FESTIVAL_LAST_CUTSCENE:
            tag_active_sprites(sowing_festival_sprite_tags, ARRAY_COUNT(sowing_festival_sprite_tags));
            break;

        case HORSE_RACE_FIRST_CUTSCENE ... HORSE_RACE_LAST_CUTSCENE:
            tag_active_sprite_range(HORSE_RACE_BACKGROUND_FIRST_SPRITE,
                                    HORSE_RACE_BACKGROUND_LAST_SPRITE,
                                    HM64_WIDESCREEN_FULLSCREEN);
            break;

        case OPENING_LOGOS_CUTSCENE:
            tag_active_sprite(OPENING_LOGO_BACKGROUND_SPRITE, HM64_WIDESCREEN_FULLSCREEN);
            break;

        case FUNERAL_INTRO_CUTSCENE:
            tag_active_sprite(FUNERAL_INTRO_BACKGROUND_SPRITE, HM64_WIDESCREEN_FULLSCREEN);
            break;
    }
}

void update_2d_widescreen_sprite_flags(void) {
    clear_2d_widescreen_flags();
    tag_title_callback_sprites();
    tag_intro_background_sprites();
}
