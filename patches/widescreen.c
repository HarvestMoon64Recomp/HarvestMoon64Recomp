#include "patches.h"

#include "common.h"
#include "system/map.h"
#include "system/globalSprites.h"
#include "system/graphic.h"
#include "system/math.h"
#include "system/mapController.h"
#include "system/sceneGraph.h"
#include "mainproc.h"



#define HM64_BIG_MAP_DL_SIZE 30000
#define HM64_BIG_TILE_VTX_SIZE 16384

static Gfx hm64_bigMapDisplayList[2][HM64_BIG_MAP_DL_SIZE];
static Vtx hm64_bigTileVertices[2][HM64_BIG_TILE_VTX_SIZE];

extern MainMap mainMap[MAX_MAPS];

extern f32 frustumEdgeCoefficient0;
extern f32 frustumEdgeCoefficient1;
extern f32 frustumEdgeCoefficient2;
extern f32 frustumEdgeCoefficient3;

extern Gfx* renderTiles(Gfx* dl, MainMap* mainMap, u16 gridIndex, u8 textureIndex);

extern u8 gridPositionToX[1596];
extern u8 gridPositionToZ[1596];

extern volatile u32 gGraphicsBufferIndex;

extern Gfx groundObjectBitmapsDisplayList[2][0x1000];



extern Gfx* prepareTileTextures(Gfx* dl, MainMap* map, u8 textureIndex);
extern void processMapAdditions(u16 mapIndex);
extern void setupCoreMapObjectSprites(MainMap* map);
extern void setupMapObjectSprites(MainMap* map);
extern void setupWeatherSprites(MainMap* map);
extern void processMapSceneNode(u16 mapIndex, Gfx* dlStartingPosition);
extern void renderGroundObjects(MainMap* map);

static inline s32 hm64_updateMapRGBA(u16 i) {
    u16 count = 0;

    if (mainMap[i].mapGlobals.currentRGBA.r < mainMap[i].mapGlobals.targetRGBA.r) {
        mainMap[i].mapGlobals.currentRGBA.r += mainMap[i].mapGlobals.deltaRGBA.r;
        if (mainMap[i].mapGlobals.targetRGBA.r <= mainMap[i].mapGlobals.currentRGBA.r) {
            mainMap[i].mapGlobals.currentRGBA.r = mainMap[i].mapGlobals.targetRGBA.r;
        } else {
            count += 1;
        }
    }

    if (mainMap[i].mapGlobals.currentRGBA.r > mainMap[i].mapGlobals.targetRGBA.r) {
        mainMap[i].mapGlobals.currentRGBA.r -= mainMap[i].mapGlobals.deltaRGBA.r;
        if (mainMap[i].mapGlobals.currentRGBA.r <= mainMap[i].mapGlobals.targetRGBA.r) {
            mainMap[i].mapGlobals.currentRGBA.r = mainMap[i].mapGlobals.targetRGBA.r;
        } else {
            count += 1;
        }
    }

    if (mainMap[i].mapGlobals.currentRGBA.g < mainMap[i].mapGlobals.targetRGBA.g) {
        mainMap[i].mapGlobals.currentRGBA.g += mainMap[i].mapGlobals.deltaRGBA.g;
        if (mainMap[i].mapGlobals.targetRGBA.g <= mainMap[i].mapGlobals.currentRGBA.g) {
            mainMap[i].mapGlobals.currentRGBA.g = mainMap[i].mapGlobals.targetRGBA.g;
        } else {
            count += 1;
        }
    }

    if (mainMap[i].mapGlobals.currentRGBA.g > mainMap[i].mapGlobals.targetRGBA.g) {
        mainMap[i].mapGlobals.currentRGBA.g -= mainMap[i].mapGlobals.deltaRGBA.g;
        if (mainMap[i].mapGlobals.currentRGBA.g <= mainMap[i].mapGlobals.targetRGBA.g) {
            mainMap[i].mapGlobals.currentRGBA.g = mainMap[i].mapGlobals.targetRGBA.g;
        } else {
            count += 1;
        }
    }

    if (mainMap[i].mapGlobals.currentRGBA.b < mainMap[i].mapGlobals.targetRGBA.b) {
        mainMap[i].mapGlobals.currentRGBA.b += mainMap[i].mapGlobals.deltaRGBA.b;
        if (mainMap[i].mapGlobals.targetRGBA.b <= mainMap[i].mapGlobals.currentRGBA.b) {
            mainMap[i].mapGlobals.currentRGBA.b = mainMap[i].mapGlobals.targetRGBA.b;
        } else {
            count += 1;
        }
    }

    if (mainMap[i].mapGlobals.currentRGBA.b > mainMap[i].mapGlobals.targetRGBA.b) {
        mainMap[i].mapGlobals.currentRGBA.b -= mainMap[i].mapGlobals.deltaRGBA.b;
        if (mainMap[i].mapGlobals.currentRGBA.b <= mainMap[i].mapGlobals.targetRGBA.b) {
            mainMap[i].mapGlobals.currentRGBA.b = mainMap[i].mapGlobals.targetRGBA.b;
        } else {
            count += 1;
        }
    }

    if (mainMap[i].mapGlobals.currentRGBA.a < mainMap[i].mapGlobals.targetRGBA.a) {
        mainMap[i].mapGlobals.currentRGBA.a += mainMap[i].mapGlobals.deltaRGBA.a;
        if (mainMap[i].mapGlobals.targetRGBA.a <= mainMap[i].mapGlobals.currentRGBA.a) {
            mainMap[i].mapGlobals.currentRGBA.a = mainMap[i].mapGlobals.targetRGBA.a;
        } else {
            count += 1;
        }
    }

    if (mainMap[i].mapGlobals.currentRGBA.a > mainMap[i].mapGlobals.targetRGBA.a) {
        mainMap[i].mapGlobals.currentRGBA.a -= mainMap[i].mapGlobals.deltaRGBA.a;
        if (mainMap[i].mapGlobals.currentRGBA.a <= mainMap[i].mapGlobals.targetRGBA.a) {
            mainMap[i].mapGlobals.currentRGBA.a = mainMap[i].mapGlobals.targetRGBA.a;
        } else {
            count += 1;
        }
    }

    return count;
}

static inline void hm64_handleRotation(u16 i) {
    mainMap[i].mapCameraView.rotation.x = 0.0f;
    mainMap[i].mapCameraView.rotation.y = 360.0f - currentWorldRotationAngles.y;
    mainMap[i].mapCameraView.rotation.z = 0.0f;

    if (mainMap[i].mapCameraView.rotation.y < 0.0f) {
        mainMap[i].mapCameraView.rotation.y += 360.0f;
    }

    if (mainMap[i].mapCameraView.rotation.y >= 360.0f) {
        mainMap[i].mapCameraView.rotation.y -= 360.0f;
    }
}

/* --------------------------------------------------------------------------
   Optional: keep your widened tile visibility.
   Remove this patch if you already have your own checkTileVisible.
   -------------------------------------------------------------------------- */
RECOMP_PATCH bool checkTileVisible(MainMap* map, u8 x, u8 z) {
    const f32 PAD = 10.0f;

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

    (void)PAD;
    return TRUE;
}

RECOMP_PATCH u32 setTileVertices(MainMap* map, u16 tileIndex, f32 x, f32 y, f32 z) {
    u8 count = 0;
    u16 vtxNumber = map->mapState.startingVertex + map->mapState.renderedVertexCount;
    s8* vtx = map->tiles[tileIndex].coordinates;
    u16 needed = map->tiles[tileIndex].verticesPerTile;

    if (!needed) {
        return 0;
    }

    /* Refuse the tile if the whole thing won't fit. */
    if ((u32) vtxNumber + needed > HM64_BIG_TILE_VTX_SIZE) {
        return 0;
    }

    do {
        hm64_bigTileVertices[gGraphicsBufferIndex][vtxNumber].v.ob[0] = *vtx++ + x;
        hm64_bigTileVertices[gGraphicsBufferIndex][vtxNumber].v.ob[1] = *(u8*) vtx++ + y;
        hm64_bigTileVertices[gGraphicsBufferIndex][vtxNumber].v.ob[2] = *vtx++ + z;

        hm64_bigTileVertices[gGraphicsBufferIndex][vtxNumber].v.cn[0] = map->mapGlobals.currentRGBA.r;
        hm64_bigTileVertices[gGraphicsBufferIndex][vtxNumber].v.cn[1] = map->mapGlobals.currentRGBA.g;
        hm64_bigTileVertices[gGraphicsBufferIndex][vtxNumber].v.cn[2] = map->mapGlobals.currentRGBA.b;
        hm64_bigTileVertices[gGraphicsBufferIndex][vtxNumber].v.cn[3] = map->mapGlobals.currentRGBA.a;

        vtxNumber++;
        count++;
    } while (count < needed);

    return count;
}

RECOMP_PATCH Gfx* appendTileToDL(Gfx* dl, MainMap* map, u16 tileIndex, f32 x, f32 y, f32 z) {
    u16 i;
    u32 count;
    Gfx* tempDl;

    count = setTileVertices(map, tileIndex, x, y, z);

    /* If we could not write the full vertex set, skip this tile entirely. */
    if (count != map->tiles[tileIndex].verticesPerTile) {
        return dl;
    }

    /*
        Also guard against gSPVertex limits.
        F3DEX expects at most 32 vertices loaded in one gSPVertex call.
    */
    if (map->tiles[tileIndex].verticesPerTile > 32) {
        return dl;
    }

    gSPVertex(
        dl++,
        &hm64_bigTileVertices[gGraphicsBufferIndex][map->mapState.startingVertex + map->mapState.renderedVertexCount],
        map->tiles[tileIndex].verticesPerTile, 0);

    map->mapState.renderedVertexCount += count;

    tempDl = &map->tileRenderingCommands[map->tiles[tileIndex].renderingCommandsOffset];
    for (i = 0; i < map->tiles[tileIndex].triCount; i++) {
        *dl++ = *tempDl++;
    }

    return dl;
}

extern Gfx* prepareTileTextures(Gfx* dl, MainMap* mainMap, u8 textureIndex);

RECOMP_PATCH Gfx* renderTiles(Gfx* dl, MainMap* mainMap, u16 gridIndex, u8 textureIndex);

RECOMP_PATCH Gfx* buildMapDisplayList(Gfx* dl, MainMap* map, u16 startingVertex) {
    u8 i;
    u16 gridIndex;

    map->mapState.renderedVertexCount = 0;
    map->mapState.startingVertex = startingVertex;

    gDPSetCombineMode(dl++, G_CC_MODULATEIA, G_CC_MODULATEIA);
    gDPSetRenderMode(dl++, G_RM_RA_ZB_OPA_SURF, G_RM_RA_ZB_OPA_SURF2);
    gDPSetTextureFilter(dl++, G_TF_BILERP);


    for (i = 0; i < MAX_TILE_TEXTURES + 1; i++) {
        gridIndex = map->textureToFirstGrid[i];
        if (gridIndex != 0xFFFF) {
            dl = renderTiles(dl, map, gridIndex, i);
        }
    }

    gSPEndDisplayList(dl++);
    map->mapState.totalVertexCount = map->mapState.renderedVertexCount;

    return dl;
}

RECOMP_PATCH void updateMapGraphics(void) {
    u16 startingCount = 0;
    u16 i;
    Gfx* dl = hm64_bigMapDisplayList[gGraphicsBufferIndex];
    Gfx* dlStartingPosition;
    u32 padding[4];

    for (i = 0; i < MAX_MAPS; i++) {
        if ((mainMap[i].mapState.flags & MAP_ACTIVE) && (mainMap[i].mapState.flags & MAP_GROUND_OBJECTS_LOADED)) {

            processMapAdditions(i);

            if (hm64_updateMapRGBA(i) == 0) {
                mainMap[i].mapState.flags |= MAP_RGBA_FINISHED;
            } else {
                mainMap[i].mapState.flags &= ~MAP_RGBA_FINISHED;
            }

            if (mainMap[i].mapGlobals.rotation.x < 0.0f) {
                mainMap[i].mapGlobals.rotation.x += 360.0f;
            }
            if (mainMap[i].mapGlobals.rotation.x >= 360.0f) {
                mainMap[i].mapGlobals.rotation.x -= 360.0f;
            }

            if (mainMap[i].mapGlobals.rotation.y < 0.0f) {
                mainMap[i].mapGlobals.rotation.y += 360.0f;
            }
            if (mainMap[i].mapGlobals.rotation.y >= 360.0f) {
                mainMap[i].mapGlobals.rotation.y -= 360.0f;
            }

            if (mainMap[i].mapGlobals.rotation.z < 0.0f) {
                mainMap[i].mapGlobals.rotation.z += 360.0f;
            }
            if (mainMap[i].mapGlobals.rotation.z >= 360.0f) {
                mainMap[i].mapGlobals.rotation.z -= 360.0f;
            }

            hm64_handleRotation(i);

            dlStartingPosition = dl;
            dl = buildMapDisplayList(dlStartingPosition, &mainMap[i], startingCount);

            setupCoreMapObjectSprites(&mainMap[i]);
            setupMapObjectSprites(&mainMap[i]);
            setupWeatherSprites(&mainMap[i]);

            processMapSceneNode(i, dlStartingPosition);
            renderGroundObjects(&mainMap[i]);

            startingCount += mainMap[i].mapState.totalVertexCount;

            if ((dl - hm64_bigMapDisplayList[gGraphicsBufferIndex]) >= (HM64_BIG_MAP_DL_SIZE - 16)) {
                break;
            }

            if (startingCount >= (HM64_BIG_TILE_VTX_SIZE - 64)) {
                break;
            }
        }
    }

}
