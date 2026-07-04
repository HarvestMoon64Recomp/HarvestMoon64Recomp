#include "patches.h"

#include "graphics.h"
#include "system/globalSprites.h"
#include "system/map.h"
#include "system/math.h"
#include "system/sprite.h"

RECOMP_PATCH void setupWeatherSprites(MainMap* mainMap) {
    for (u16 i = 0; i < MAX_WEATHER_SPRITES; i++) {
        if (mainMap->weatherSprites[i].flags & MAP_WEATHER_SPRITE_ACTIVE) {
            if (getSpriteAnimationStateChangedFlag(mainMap->weatherSprites[i].spriteIndex) ||
                !(mainMap->weatherSprites[i].flags & MAP_WEATHER_SPRITE_ANIMATION_STARTED)) {
                resetAnimationState(mainMap->weatherSprites[i].spriteIndex);

                if (!getRandomNumberInRange(0, 4)) {
                    f32 weatherWidth = 240.0f * recomp_get_target_aspect_ratio(4.0f / 3.0f);

                    globalSprites[mainMap->weatherSprites[i].spriteIndex].stateFlags &=
                        ~SPRITE_ANIMATION_STATE_CHANGED;
                    startSpriteAnimation(mainMap->weatherSprites[i].spriteIndex,
                                         mainMap->weatherSprites[i].animationIndex, 0xFE);
                    mainMap->weatherSprites[i].flags |= MAP_WEATHER_SPRITE_ANIMATION_STARTED;
                    mainMap->weatherSprites[i].coordinates.x =
                        getRandomNumberInRange(0, (u16)weatherWidth) +
                        ((mainMap->mapCameraView.cameraTileX - (mainMap->mapGrid.mapWidth / 2)) *
                         mainMap->mapGrid.tileSizeX) -
                        (weatherWidth / 2.0f);
                    mainMap->weatherSprites[i].coordinates.y = 0.0f;
                    mainMap->weatherSprites[i].coordinates.z =
                        getRandomNumberInRange(0, 240) +
                        ((mainMap->mapCameraView.cameraTileZ - (mainMap->mapGrid.mapHeight / 2)) *
                         mainMap->mapGrid.tileSizeZ) -
                        120.0f;
                }
            }

            setSpriteViewSpacePosition(
                mainMap->weatherSprites[i].spriteIndex,
                mainMap->weatherSprites[i].coordinates.x +
                    ((mainMap->mapGrid.mapWidth * mainMap->mapGrid.tileSizeX) / 2) -
                    (mainMap->mapGrid.tileSizeX / 2) -
                    (mainMap->mapCameraView.cameraTileX * mainMap->mapGrid.tileSizeX) +
                    mainMap->mapCameraView.viewOffset.x,
                mainMap->weatherSprites[i].coordinates.y +
                    ((globalSprites[mainMap->weatherSprites[i].spriteIndex].renderingFlags &
                      SPRITE_RENDERING_REVERSE_WINDING)
                         ? -mainMap->mapCameraView.viewOffset.y
                         : mainMap->mapCameraView.viewOffset.y),
                mainMap->weatherSprites[i].coordinates.z +
                    ((mainMap->mapGrid.mapHeight * mainMap->mapGrid.tileSizeZ) / 2) -
                    (mainMap->mapGrid.tileSizeZ / 2) -
                    (mainMap->mapCameraView.cameraTileZ * mainMap->mapGrid.tileSizeZ) +
                    mainMap->mapCameraView.viewOffset.z);

            setSpriteColor(mainMap->weatherSprites[i].spriteIndex, mainMap->mapGlobals.currentRGBA.r,
                           mainMap->mapGlobals.currentRGBA.g, mainMap->mapGlobals.currentRGBA.b,
                           mainMap->mapGlobals.currentRGBA.a);
            setBilinearFiltering(mainMap->weatherSprites[i].spriteIndex, TRUE);
        }
    }
}
