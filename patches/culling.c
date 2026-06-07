#include "patches.h"

#include "system/map.h"
#include "system/mapController.h"
#include "system/sceneGraph.h"

// @recomp Disabling tile culling makes every tile render each frame, and grid-scan emission (see
// buildMapDisplayList) reloads textures more often than texture-batched emission, so these buffers
// need headroom.
#define HM64_BIG_MAP_DL_SIZE 64000
#define HM64_BIG_TILE_VTX_SIZE 49152
#define HM64_MAP_MATRIX_GROUP_ID_BASE 0x484D6400

static Gfx hm64_bigMapDisplayList[2][HM64_BIG_MAP_DL_SIZE];
static Vtx hm64_bigTileVertices[2][HM64_BIG_TILE_VTX_SIZE];

extern Gfx* prepareTileTextures(Gfx* dl, MainMap* map, u8 textureIndex);
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
    u32 mapIndex = (u32)(map - mainMap);
    u32 mapMatrixGroupId = HM64_MAP_MATRIX_GROUP_ID_BASE + mapIndex;
    u32 cellCount = (u32)map->mapGrid.mapWidth * (u32)map->mapGrid.mapHeight;
    Gfx* dlEnd = &hm64_bigMapDisplayList[gGraphicsBufferIndex][HM64_BIG_MAP_DL_SIZE];
    u16 lastTexSlot = 0xFFFF;
    u32 gridIndex;
    u16 additionIndex;
    bool snapGround = FALSE;

    map->mapState.renderedVertexCount = 0;
    map->mapState.startingVertex = startingVertex;

    // @recomp Detect a one-shot tile animation that's currently playing -- specifically the shipping bins. 
    // The bins' "bounce" animation swaps their 2x2 footprints through boxes of different vertex
    // counts AND different texture S/T rectangles each frame; RT64 interpolates both vertices
    // and the texture region by index, distorting the box and scrolling the grass texels inside the tile
    // into view -> flickering green at any framerate above Original. While this kind of animation is active,
    // should snap the whole ground (skip vertex + tile interpolation).
    for (additionIndex = 0; additionIndex < MAX_MAP_ADDITIONS; additionIndex++) {
        u16 additionFlags = map->mapAdditions[additionIndex].flags;
        if ((additionFlags & MAP_ADDITION_ACTIVE) && !(additionFlags & MAP_ADDITION_LOOPING)) {
            snapGround = TRUE;
            break;
        }
    }

    // @recomp Emit the map tiles in stable grid-scan order instead of texture-batched order. RT64
    // matches vertices for interpolation by their position in the vertex stream, so that order must be
    // stable frame to frame. The original texture batching reorders the stream whenever a cell's
    // texture group changes.
    //
    // Scanning the grid in a fixed order keeps every cell at the same stream position even as its tile
    // changes, so RT64 interpolates the ground correctly.
    //
    // The ground is opaque and Z-buffered, so draw order doesn't affect the image
    if (snapGround) {
        gEXMatrixGroupDecomposedVertsTilesSkipOrderAuto(dl, mapMatrixGroupId, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
    } else {
        gEXMatrixGroupDecomposedVertsOrderAuto(dl, mapMatrixGroupId, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
    }
    dl += 2;

    gDPSetCombineMode(dl++, G_CC_MODULATEIA, G_CC_MODULATEIA);
    gDPSetRenderMode(dl++, G_RM_RA_ZB_OPA_SURF, G_RM_RA_ZB_OPA_SURF2);
    gDPSetTextureFilter(dl++, G_TF_BILERP);

    for (gridIndex = 0; gridIndex < cellCount; gridIndex++) {
        u8 gx = (u8)(gridIndex % map->mapGrid.mapWidth);
        u8 gz = (u8)(gridIndex / map->mapGrid.mapWidth);
        u16 raw = map->mapGrid.gridToTileIndex[gridIndex];
        u16 swapped = (u16)((raw >> 8) | (raw << 8)); // matches swap16TileIndex (big-endian tile value)
        u16 tileIndex = (u16)(swapped - 1);
        u8 textureIndex;
        u16 texSlot;

        // @recomp Guard against display-list overflow (the patched builder had no per-tile check).
        if (dl + 64 >= dlEnd) {
            break;
        }

        if (swapped == 0) {
            // @recomp Empty cell: nothing to draw. Empty cells are part of the static map layout, so
            // skipping them keeps the stream order stable.
            continue;
        }

        textureIndex = map->tiles[tileIndex].textureIndex;
        texSlot = (textureIndex & 0x80) ? (u16)(textureIndex & 0x7F) : (u16)MAX_TILE_TEXTURES;

        // @recomp Load the texture only when it changes along the scan; solid-colour tiles (slot
        // MAX_TILE_TEXTURES) never load one and leave the currently loaded texture untouched.
        if (texSlot != (u16)MAX_TILE_TEXTURES && texSlot != lastTexSlot) {
            dl = prepareTileTextures(dl, map, (u8)texSlot);
            lastTexSlot = texSlot;
        }

        dl = appendTileToDL(dl, map, tileIndex,
                            (gx - map->mapCameraView.cameraTileX) * map->mapGrid.tileSizeX,
                            map->tiles[tileIndex].yOffset,
                            (gz - map->mapCameraView.cameraTileZ) * map->mapGrid.tileSizeZ);

        map->visibilityGrid[gz][gx] = TRUE;
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
