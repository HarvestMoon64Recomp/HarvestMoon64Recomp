#include "patches.h"
#include "system/map.h"
extern f32 frustumEdgeCoefficient0;
extern f32 frustumEdgeCoefficient1;
extern f32 frustumEdgeCoefficient2;
extern f32 frustumEdgeCoefficient3;
RECOMP_PATCH bool checkTileVisible(MainMap* map, u8 x, u8 z) {
    f32 PAD;
    u16 w = map->mapGrid.mapWidth;
    u16 h = map->mapGrid.mapHeight;
    u32 area = (u32) w * (u32) h;

        PAD = 55.0f;


    f32 t0 = ((map->mapCameraView.frustumCorner0.z * (map->mapCameraView.frustumCorner1.x - x)) +
              (map->mapCameraView.frustumCorner1.z * (x - map->mapCameraView.frustumCorner0.x))) +
             (z * frustumEdgeCoefficient0);

    f32 t1 = ((map->mapCameraView.frustumCorner1.z * (map->mapCameraView.frustumCorner2.x - x)) +
              (map->mapCameraView.frustumCorner2.z * (x - map->mapCameraView.frustumCorner1.x))) +
             (z * frustumEdgeCoefficient1);

    f32 t2 = ((map->mapCameraView.frustumCorner2.z * (map->mapCameraView.frustumCorner3.x - x)) +
              (map->mapCameraView.frustumCorner3.z * (x - map->mapCameraView.frustumCorner2.x))) +
             (z * frustumEdgeCoefficient2);

    f32 t3 = ((map->mapCameraView.frustumCorner3.z * (map->mapCameraView.frustumCorner0.x - x)) +
              (map->mapCameraView.frustumCorner0.z * (x - map->mapCameraView.frustumCorner3.x))) +
             (z * frustumEdgeCoefficient3);

    return (t0 >= -1.0f - PAD && t1 >= -1.0f - PAD && t2 >= -1.0f - PAD && t3 >= -1.0f - PAD);
}

#include "system/globalSprites.h"
#include "system/graphic.h"
#include "system/math.h"
#include "system/mapController.h"
#include "system/sceneGraph.h"
#include "mainproc.h"

extern u8 gridIndexToTileIndexX[20 * 24];
extern u8 gridIndexToTileIndexZ[20 * 24];
extern Gfx groundObjectBitmapsDisplayList[2][0x1000];
extern CoreMapObject coreMapObjects[0x100];
extern BitmapObject bitmaps[];

extern Gfx* prepareGroundObjectBitmap(Gfx* dl, GroundObjectBitmap* sprite);
extern Gfx* renderGroundObject(Gfx* dl, MainMap* map, GroundObjectBitmap* bitmap, u16 vtxIndex);
extern f32 getTerrainHeightAtPosition(u16 mapIndex, f32 x, f32 z);
extern void addGroundObjectToSceneGraph(MainMap* map, f32 x, f32 y, f32 z, f32 camX, f32 camZ, Gfx* dl);
extern u16 setBitmap(u8* texturePtr, u16* palettePtr, u16 flags);

extern void setupMapObjectSprites(MainMap* map);
extern void setupWeatherSprites(MainMap* map);
extern void processMapSceneNode(u16 mapIndex, Gfx* dl);

RECOMP_PATCH void setupCoreMapObjectSprites(MainMap* map);
RECOMP_PATCH void renderGroundObjects(MainMap* map);

static inline u8* hm64_getTexturePtr(u8 spriteIndex, u32* textureIndex) {
    return (u8*) textureIndex + textureIndex[spriteIndex];
}

static inline u16* hm64_getPalettePtr(u8 index, u32* paletteIndex) {
    return (u16*) ((u8*) paletteIndex + paletteIndex[index]);
}

static inline bool hm64_visBounds(s32 x, s32 z) {
    return x >= 0 && x < 42 && z >= 0 && z < 38;
}

RECOMP_PATCH void setupCoreMapObjectSprites(MainMap* map) {
    Vec3f vec, vec2;
    f32 coordinateX, coordinateY, coordinateZ;
    f32 rotationX, rotationY, rotationZ;
    f32 scaleX, scaleY;
    f32 xPosition, yPosition, zPosition;
    f32 tileOffsetX, tileOffsetZ;
    f32 cameraWorldX, cameraWorldZ;
    f32 mapWorldCenterX, mapWorldCenterZ;
    u16 bitmapIndex;
    u8* texturePtr;
    u16* palettePtr;
    u8 total;
    u16 i, j, k;

    i = 0;
    while (i < map->mapState.coreMapObjectsCount) {
        j = 0;
        k = map->coreMapObjectsMetadata[i].unk_0;
        total = map->coreMapObjectsMetadata[i].repeatObjectCount;

        while (j < total) {
            coordinateX = coreMapObjects[k].coordinates.x;
            coordinateY = coreMapObjects[k].coordinates.y;
            yPosition = coordinateY;

            mapWorldCenterX = (map->mapGrid.mapWidth * map->mapGrid.tileSizeX) / 2;
            mapWorldCenterZ = (map->mapGrid.mapHeight * map->mapGrid.tileSizeZ) / 2;

            coordinateZ = coreMapObjects[k].coordinates.z;

            tileOffsetX = map->mapGrid.tileSizeX / 2;
            tileOffsetZ = map->mapGrid.tileSizeZ / 2;

            cameraWorldX = map->mapCameraView.cameraTileX * map->mapGrid.tileSizeX;
            cameraWorldZ = map->mapCameraView.cameraTileZ * map->mapGrid.tileSizeZ;

            xPosition = (coordinateX + mapWorldCenterX) - tileOffsetX - cameraWorldX;
            zPosition = (coordinateZ + mapWorldCenterZ) - tileOffsetZ - cameraWorldZ;

            xPosition += map->mapCameraView.viewOffset.x;
            yPosition += map->mapCameraView.viewOffset.y;
            zPosition += map->mapCameraView.viewOffset.z;

            vec2.x = ((coordinateX - tileOffsetX) + map->mapState.mapOriginX) / map->mapGrid.tileSizeX;
            vec2.y = 0;
            vec2.z = ((coordinateZ - tileOffsetZ) + map->mapState.mapOriginZ) / map->mapGrid.tileSizeZ;
            vec = vec2;

            if (hm64_visBounds((s32) (u8) vec.x, (s32) (u8) vec.z)) {
                texturePtr =
                    hm64_getTexturePtr(map->coreMapObjectsMetadata[i].spriteIndex, map->coreMapObjectsTextures);
                palettePtr =
                    hm64_getPalettePtr(map->coreMapObjectsMetadata[i].spriteIndex, map->coreMapObjectsPalettes);

                switch (coreMapObjects[k].flags & CORE_MAP_OBJECT_SPRITE_MODE_MASK) {
                    case 0x0:
                        scaleY = 1.0f;
                        break;
                    case 0x4:
                        scaleY = 2.0f;
                        break;
                    case 0x8:
                        scaleY = 4.0f;
                        break;
                    case 0xC:
                        scaleY = 8.0f;
                        break;
                    default:
                        scaleY = 1.0f;
                        break;
                }
                scaleX = scaleY;

                if (coreMapObjects[k].flags & CORE_MAP_OBJECT_APPLY_ROTATION) {
                    rotationX = map->mapGlobals.rotation.x;
                    rotationY = map->mapGlobals.rotation.y;
                    rotationZ = map->mapGlobals.rotation.z;

                    switch (coreMapObjects[k].flags & CORE_MAP_OBJECT_ROTATION_MODE_MASK) {
                        case 0x70:
                            rotationY = (s32) (rotationY + 45.0f) % 360;
                            break;
                        case 0x10:
                            rotationY = (s32) (rotationY + 315.0f) % 360;
                            break;
                        case 0x20:
                            rotationY = (s32) (rotationY + 270.0f) % 360;
                            break;
                        case 0x30:
                            rotationY = (s32) (rotationY + 225.0f) % 360;
                            break;
                        case 0x40:
                            rotationY = (s32) (rotationY + 180.0f) % 360;
                            break;
                        case 0x50:
                            rotationY = (s32) (rotationY + 135.0f) % 360;
                            break;
                        case 0x60:
                            rotationY = (s32) (rotationY + 90.0f) % 360;
                            break;
                        default:
                            break;
                    }
                } else {
                    rotationX = 0.0f;
                    rotationY = 0.0f;
                    rotationZ = 0.0f;
                }

                bitmapIndex =
                    setBitmap(texturePtr, palettePtr, (0x8 | SCENE_NODE_UPDATE_SCALE | SCENE_NODE_UPDATE_ROTATION));

                setBitmapViewSpacePosition(bitmapIndex, xPosition, yPosition, zPosition);
                setBitmapRotation(bitmapIndex, rotationX, rotationY, rotationZ);
                setBitmapScale(bitmapIndex, scaleX, scaleY, 1.0f);
                setBitmapTriangleWinding(bitmapIndex, 0);
                setBitmapAnchorAlignment(bitmapIndex, SPRITE_ANCHOR_CENTER, SPRITE_ANCHOR_NEGATIVE);
                setBitmapAxisMapping(bitmapIndex, 2);

                if (coreMapObjects[k].flags & CORE_MAP_OBJECT_ALPHA_BLEND) {
                    setBitmapBlendMode(bitmapIndex, SPRITE_BLEND_ALPHA_DECAL_NO_Z);
                } else {
                    setBitmapBlendMode(bitmapIndex, SPRITE_BLEND_ALPHA_DECAL);
                }

                setBitmapRGBA(bitmapIndex, map->mapGlobals.currentRGBA.r, map->mapGlobals.currentRGBA.g,
                              map->mapGlobals.currentRGBA.b, map->mapGlobals.currentRGBA.a);
                setBitmapAnchor(bitmapIndex, 0, 0);

                bitmaps[bitmapIndex].flags |= BITMAP_USE_BILINEAR_FILTERING;
            }

            asm("");
            j++;
            total = map->coreMapObjectsMetadata[i].repeatObjectCount;
            k++;
        }

        i++;
    }
}

RECOMP_PATCH void renderGroundObjects(MainMap* map) {
    Gfx* dl;
    Gfx* startingPositionDl;
    u16 i, count, gridIndex;
    u32 index;
    s16 temp1, temp2;
    f32 arr[8];

    arr[6] = map->mapCameraView.cameraTileX * map->mapGrid.tileSizeX;
    index = gGraphicsBufferIndex;
    arr[7] = map->mapCameraView.cameraTileZ * map->mapGrid.tileSizeZ;

    temp1 = (-(map->mapGrid.mapWidth * map->mapGrid.tileSizeX) / 2) + (map->mapGrid.tileSizeX * map->groundObjects.x) -
            (map->mapGrid.tileSizeX / 2);

    temp2 = (-(map->mapGrid.mapHeight * map->mapGrid.tileSizeZ) / 2) + (map->mapGrid.tileSizeZ * map->groundObjects.z) -
            (map->mapGrid.tileSizeZ / 2);

    count = 0;
    dl = groundObjectBitmapsDisplayList[index];

    if (map->groundObjects.unk_12) {
        for (i = 0; i < MAX_GROUND_OBJECTS; i++) {
            if (map->groundObjects.spriteIndexToGrid[i] == 0xFFFF)
                continue;

            startingPositionDl = dl;
            dl = prepareGroundObjectBitmap(dl, &map->groundObjectBitmaps[i]);
            gridIndex = map->groundObjects.spriteIndexToGrid[i];

            do {
                s32 vx = gridIndexToTileIndexX[gridIndex] + map->groundObjects.x;
                s32 vz = gridIndexToTileIndexZ[gridIndex] + map->groundObjects.z;

                if (hm64_visBounds(vx, vz)) {
                    dl = renderGroundObject(dl, map, &map->groundObjectBitmaps[i], count);

                    addGroundObjectToSceneGraph(
                        map,
                        temp1 + (gridIndexToTileIndexX[gridIndex] * 32) + map->groundObjectBitmaps[i].coordinates.x,
                        map->groundObjects.unk_12 + map->groundObjectBitmaps[i].coordinates.y,
                        temp2 + (gridIndexToTileIndexZ[gridIndex] * 32) + map->groundObjectBitmaps[i].coordinates.z,
                        arr[6], arr[7], startingPositionDl);

                    count++;
                    startingPositionDl = dl;
                }

                gridIndex = map->groundObjects.nextGridToSpriteIndex[gridIndex];
            } while (gridIndex != 0xFFFF);
        }
    } else {
        for (i = 0; i < MAX_GROUND_OBJECTS; i++) {
            if (map->groundObjects.spriteIndexToGrid[i] == 0xFFFF)
                continue;

            startingPositionDl = dl;
            dl = prepareGroundObjectBitmap(dl, &map->groundObjectBitmaps[i]);
            gridIndex = map->groundObjects.spriteIndexToGrid[i];

            do {
                s32 vx = gridIndexToTileIndexX[gridIndex] + map->groundObjects.x;
                s32 vz = gridIndexToTileIndexZ[gridIndex] + map->groundObjects.z;

                if (hm64_visBounds(vx, vz)) {
                    dl = renderGroundObject(dl, map, &map->groundObjectBitmaps[i], count);

                    arr[2] =
                        temp1 + (gridIndexToTileIndexX[gridIndex] * 32) + map->groundObjectBitmaps[i].coordinates.x;
                    arr[4] = temp2 + (gridIndexToTileIndexZ[gridIndex] * 32) +
                             map->groundObjectBitmaps[i].coordinates.z - 4.0f;
                    arr[3] = getTerrainHeightAtPosition(0, arr[2], arr[4]);

                    addGroundObjectToSceneGraph(map, arr[2], arr[3], arr[4], arr[6], arr[7], startingPositionDl);

                    count++;
                    startingPositionDl = dl;
                }

                gridIndex = map->groundObjects.nextGridToSpriteIndex[gridIndex];
            } while (gridIndex != 0xFFFF);
        }
    }

    if (dl - groundObjectBitmapsDisplayList[gGraphicsBufferIndex] >= 0x1000) {
        return;
    }
}