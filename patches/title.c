#include "patches.h"

#include "common.h"
#include "system/globalSprites.h"
#include "system/sprite.h"
#include "assetIndices/sprites.h"

#define HM64_TITLE_WRAP_RIGHT 426.66669f
#define HM64_TITLE_WRAP_RESET -533.33334f

RECOMP_PATCH void resetWrappingSpritePositions(void) {


    if (globalSprites[CLOUD_2_1].viewSpacePosition.x > HM64_TITLE_WRAP_RIGHT) {
        setSpriteViewSpacePosition(CLOUD_2_1, HM64_TITLE_WRAP_RESET, 64.0f, 64.0f);
    }
    if (globalSprites[CLOUD_3_1].viewSpacePosition.x > HM64_TITLE_WRAP_RIGHT) {
        setSpriteViewSpacePosition(CLOUD_3_1, HM64_TITLE_WRAP_RESET, 96.0f, 64.0f);
    }
    if (globalSprites[LICENSED_BY_NINTENDO_1].viewSpacePosition.x > HM64_TITLE_WRAP_RIGHT) {
        setSpriteViewSpacePosition(LICENSED_BY_NINTENDO_1, HM64_TITLE_WRAP_RESET, 80.0f, 64.0f);
    }
    if (globalSprites[LICENSED_BY_NINTENDO_2].viewSpacePosition.x > HM64_TITLE_WRAP_RIGHT) {
        setSpriteViewSpacePosition(LICENSED_BY_NINTENDO_2, HM64_TITLE_WRAP_RESET, 72.0f, 64.0f);
    }
    if (globalSprites[CLOUD_3_2].viewSpacePosition.x > HM64_TITLE_WRAP_RIGHT) {
        setSpriteViewSpacePosition(CLOUD_3_2, HM64_TITLE_WRAP_RESET, 108.0f, 64.0f);
    }
    if (globalSprites[CLOUD_2_2].viewSpacePosition.x > HM64_TITLE_WRAP_RIGHT) {
        setSpriteViewSpacePosition(CLOUD_2_2, HM64_TITLE_WRAP_RESET, 88.0f, 64.0f);
    }

    adjustSpriteViewSpacePosition(CLOUD_2_1, 0.2f, 0.0f, 0.0f);
    adjustSpriteViewSpacePosition(CLOUD_3_1, 0.1f, 0.0f, 0.0f);
    adjustSpriteViewSpacePosition(LICENSED_BY_NINTENDO_1, 0.3f, 0.0f, 0.0f);
    adjustSpriteViewSpacePosition(LICENSED_BY_NINTENDO_2, 0.4f, 0.0f, 0.0f);
    adjustSpriteViewSpacePosition(CLOUD_3_2, 0.1f, 0.0f, 0.0f);
    adjustSpriteViewSpacePosition(CLOUD_2_2, 0.2f, 0.0f, 0.0f);


//Extra right-side fill.


    setSpriteViewSpacePosition(CLOUD_1_2, 160.0f, 0.0f, 56.0f);

    setSpriteViewSpacePosition(FAR_GRASS_1_2, 160.0f, 0.0f, 8.0f);
    setSpriteViewSpacePosition(BACK_GRASS_2, 160.0f, 0.0f, 16.0f);
    setSpriteViewSpacePosition(BACK_GRASS_3_2, 160.0f, 0.0f, 24.0f);
    setSpriteViewSpacePosition(FRONT_GRASS_2_2, 160.0f, 0.0f, 32.0f);
}