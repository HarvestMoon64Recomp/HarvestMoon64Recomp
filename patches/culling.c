#include "patches.h"

#include "system/map.h"
#include "system/mapController.h"
#include "system/sceneGraph.h"

// @recomp disabling tile culling needs larger map DL and tile vertex buffers.
#define HM64_BIG_MAP_DL_SIZE 30000
#define HM64_BIG_TILE_VTX_SIZE 16384
#define HM64_MAP_MATRIX_GROUP_ID_BASE 0x484D6400

static Gfx hm64_bigMapDisplayList[2][HM64_BIG_MAP_DL_SIZE];
static Vtx hm64_bigTileVertices[2][HM64_BIG_TILE_VTX_SIZE];

extern Gfx* renderTiles(Gfx* dl, MainMap* mainMap, u16 gridIndex, u8 textureIndex);
extern volatile u32 gGraphicsBufferIndex;
extern void processMapAdditions(u16 mapIndex);
extern void setupCoreMapObjectSprites(MainMap* map);
extern void setupMapObjectSprites(MainMap* map);
extern void setupWeatherSprites(MainMap* map);
extern void renderGroundObjects(MainMap* map);

RECOMP_PATCH bool checkTileVisible(MainMap* map, u8 x, u8 z) {
    (void)map;
    (void)x;
    (void)z;
    // @recomp keep every tile active to avoid pop-in in widescreen.
    return TRUE;
}

RECOMP_PATCH u32 setTileVertices(MainMap* map, u16 tileIndex, f32 x, f32 y, f32 z) {
    u8 count = 0;
    u16 vtxNumber;
    s8* vtx = map->tiles[tileIndex].coordinates;
    u16 needed = map->tiles[tileIndex].verticesPerTile;

    vtxNumber = map->mapState.startingVertex + map->mapState.renderedVertexCount;

    if (!needed) {
        return 0;
    }

    if ((u32)vtxNumber + needed > HM64_BIG_TILE_VTX_SIZE) {
        return 0;
    }

    do {
        hm64_bigTileVertices[gGraphicsBufferIndex][vtxNumber].v.ob[0] = *vtx + x;
        vtx++;

        hm64_bigTileVertices[gGraphicsBufferIndex][vtxNumber].v.ob[1] = *(u8*)vtx + y;
        vtx++;

        hm64_bigTileVertices[gGraphicsBufferIndex][vtxNumber].v.ob[2] = *vtx + z;
        vtx++;

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
    Gfx* tileCommands;

    count = setTileVertices(map, tileIndex, x, y, z);

    if (count != map->tiles[tileIndex].verticesPerTile) {
        return dl;
    }

    if (map->tiles[tileIndex].verticesPerTile > 32) {
        return dl;
    }

    gSPVertex(dl++,
              &hm64_bigTileVertices[gGraphicsBufferIndex][map->mapState.startingVertex +
                                                          map->mapState.renderedVertexCount],
              map->tiles[tileIndex].verticesPerTile, 0);

    map->mapState.renderedVertexCount += count;

    tileCommands = &map->tileRenderingCommands[map->tiles[tileIndex].renderingCommandsOffset];
    for (i = 0; i < map->tiles[tileIndex].triCount; i++) {
        *dl++ = *tileCommands++;
    }

    return dl;
}

RECOMP_PATCH Gfx* buildMapDisplayList(Gfx* dl, MainMap* map, u16 startingVertex) {
    u8 i;
    u16 gridIndex;
    u32 mapMatrixGroupId = HM64_MAP_MATRIX_GROUP_ID_BASE + (u32)(map - mainMap);

    map->mapState.renderedVertexCount = 0;
    map->mapState.startingVertex = startingVertex;

    // @recomp group the map for RT64 interpolation.
    gEXMatrixGroupDecomposedVertsOrderAuto(dl, mapMatrixGroupId, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
    dl += 2;

    gDPSetCombineMode(dl++, G_CC_MODULATEIA, G_CC_MODULATEIA);
    gDPSetRenderMode(dl++, G_RM_RA_ZB_OPA_SURF, G_RM_RA_ZB_OPA_SURF2);
    gDPSetTextureFilter(dl++, G_TF_BILERP);

    for (i = 0; i < MAX_TILE_TEXTURES + 1; i++) {
        gridIndex = map->textureToFirstGrid[i];
        if (gridIndex != 0xFFFF) {
            dl = renderTiles(dl, map, gridIndex, i);
        }
    }

    gEXPopMatrixGroup(dl++, G_MTX_MODELVIEW);
    gSPEndDisplayList(dl++);
    map->mapState.totalVertexCount = map->mapState.renderedVertexCount;

    return dl;
}

RECOMP_PATCH void processMapSceneNode(u16 mapIndex, Gfx* dl) {
    u16 temp = addSceneNode(dl, (0x8 | SCENE_NODE_UPDATE_ROTATION));

    addSceneNodePosition(temp,
                         mainMap[mapIndex].mapGlobals.translation.x + mainMap[mapIndex].mapCameraView.viewOffset.x,
                         mainMap[mapIndex].mapGlobals.translation.y + mainMap[mapIndex].mapCameraView.viewOffset.y,
                         mainMap[mapIndex].mapGlobals.translation.z + mainMap[mapIndex].mapCameraView.viewOffset.z);
    addSceneNodeScaling(temp,
                        mainMap[mapIndex].mapGlobals.scale.x,
                        mainMap[mapIndex].mapGlobals.scale.y,
                        mainMap[mapIndex].mapGlobals.scale.z);
    addSceneNodeRotation(temp,
                         mainMap[mapIndex].mapGlobals.rotation.x,
                         mainMap[mapIndex].mapGlobals.rotation.y,
                         mainMap[mapIndex].mapGlobals.rotation.z);
}

RECOMP_PATCH void updateMapGraphics(void) {
    u16 startingCount = 0;
    u16 i;
    u8 rgbaInFlight;
    Gfx* dl = hm64_bigMapDisplayList[gGraphicsBufferIndex];
    Gfx* dlStartingPosition;

    for (i = 0; i < MAX_MAPS; i++) {
        if ((mainMap[i].mapState.flags & MAP_ACTIVE) && (mainMap[i].mapState.flags & MAP_GROUND_OBJECTS_LOADED)) {
            processMapAdditions(i);
            rgbaInFlight = 0;

            if (mainMap[i].mapGlobals.currentRGBA.r < mainMap[i].mapGlobals.targetRGBA.r) {
                mainMap[i].mapGlobals.currentRGBA.r += mainMap[i].mapGlobals.deltaRGBA.r;
                if (mainMap[i].mapGlobals.targetRGBA.r <= mainMap[i].mapGlobals.currentRGBA.r) {
                    mainMap[i].mapGlobals.currentRGBA.r = mainMap[i].mapGlobals.targetRGBA.r;
                } else {
                    rgbaInFlight += 1;
                }
            }
            if (mainMap[i].mapGlobals.currentRGBA.r > mainMap[i].mapGlobals.targetRGBA.r) {
                mainMap[i].mapGlobals.currentRGBA.r -= mainMap[i].mapGlobals.deltaRGBA.r;
                if (mainMap[i].mapGlobals.currentRGBA.r <= mainMap[i].mapGlobals.targetRGBA.r) {
                    mainMap[i].mapGlobals.currentRGBA.r = mainMap[i].mapGlobals.targetRGBA.r;
                } else {
                    rgbaInFlight += 1;
                }
            }
            if (mainMap[i].mapGlobals.currentRGBA.g < mainMap[i].mapGlobals.targetRGBA.g) {
                mainMap[i].mapGlobals.currentRGBA.g += mainMap[i].mapGlobals.deltaRGBA.g;
                if (mainMap[i].mapGlobals.targetRGBA.g <= mainMap[i].mapGlobals.currentRGBA.g) {
                    mainMap[i].mapGlobals.currentRGBA.g = mainMap[i].mapGlobals.targetRGBA.g;
                } else {
                    rgbaInFlight += 1;
                }
            }
            if (mainMap[i].mapGlobals.currentRGBA.g > mainMap[i].mapGlobals.targetRGBA.g) {
                mainMap[i].mapGlobals.currentRGBA.g -= mainMap[i].mapGlobals.deltaRGBA.g;
                if (mainMap[i].mapGlobals.currentRGBA.g <= mainMap[i].mapGlobals.targetRGBA.g) {
                    mainMap[i].mapGlobals.currentRGBA.g = mainMap[i].mapGlobals.targetRGBA.g;
                } else {
                    rgbaInFlight += 1;
                }
            }
            if (mainMap[i].mapGlobals.currentRGBA.b < mainMap[i].mapGlobals.targetRGBA.b) {
                mainMap[i].mapGlobals.currentRGBA.b += mainMap[i].mapGlobals.deltaRGBA.b;
                if (mainMap[i].mapGlobals.targetRGBA.b <= mainMap[i].mapGlobals.currentRGBA.b) {
                    mainMap[i].mapGlobals.currentRGBA.b = mainMap[i].mapGlobals.targetRGBA.b;
                } else {
                    rgbaInFlight += 1;
                }
            }
            if (mainMap[i].mapGlobals.currentRGBA.b > mainMap[i].mapGlobals.targetRGBA.b) {
                mainMap[i].mapGlobals.currentRGBA.b -= mainMap[i].mapGlobals.deltaRGBA.b;
                if (mainMap[i].mapGlobals.currentRGBA.b <= mainMap[i].mapGlobals.targetRGBA.b) {
                    mainMap[i].mapGlobals.currentRGBA.b = mainMap[i].mapGlobals.targetRGBA.b;
                } else {
                    rgbaInFlight += 1;
                }
            }
            if (mainMap[i].mapGlobals.currentRGBA.a < mainMap[i].mapGlobals.targetRGBA.a) {
                mainMap[i].mapGlobals.currentRGBA.a += mainMap[i].mapGlobals.deltaRGBA.a;
                if (mainMap[i].mapGlobals.targetRGBA.a <= mainMap[i].mapGlobals.currentRGBA.a) {
                    mainMap[i].mapGlobals.currentRGBA.a = mainMap[i].mapGlobals.targetRGBA.a;
                } else {
                    rgbaInFlight += 1;
                }
            }
            if (mainMap[i].mapGlobals.currentRGBA.a > mainMap[i].mapGlobals.targetRGBA.a) {
                mainMap[i].mapGlobals.currentRGBA.a -= mainMap[i].mapGlobals.deltaRGBA.a;
                if (mainMap[i].mapGlobals.currentRGBA.a <= mainMap[i].mapGlobals.targetRGBA.a) {
                    mainMap[i].mapGlobals.currentRGBA.a = mainMap[i].mapGlobals.targetRGBA.a;
                } else {
                    rgbaInFlight += 1;
                }
            }

            if (rgbaInFlight == 0) {
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

            mainMap[i].mapCameraView.rotation.x = 0.0f;
            mainMap[i].mapCameraView.rotation.y = 360.0f - currentWorldRotationAngles.y;
            mainMap[i].mapCameraView.rotation.z = 0.0f;

            if (mainMap[i].mapCameraView.rotation.y < 0.0f) {
                mainMap[i].mapCameraView.rotation.y += 360.0f;
            }
            if (mainMap[i].mapCameraView.rotation.y >= 360.0f) {
                mainMap[i].mapCameraView.rotation.y -= 360.0f;
            }

            dlStartingPosition = dl;
            dl = buildMapDisplayList(dlStartingPosition, &mainMap[i], startingCount);

            setupCoreMapObjectSprites(&mainMap[i]);
            setupMapObjectSprites(&mainMap[i]);
            setupWeatherSprites(&mainMap[i]);

            processMapSceneNode(i, dlStartingPosition);
            renderGroundObjects(&mainMap[i]);

            startingCount += mainMap[i].mapState.totalVertexCount;
        }
    }
}
