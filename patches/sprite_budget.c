#include "patches.h"

#include "system/graphic.h"
#include "system/map.h"
#include "system/sceneGraph.h"
#include "system/sprite.h"

#define HM64_BIG_SPRITE_DL_SIZE 0x2000
#define HM64_BIG_BITMAP_VERTEX_SLOTS 0x200
#define HM64_RESERVED_DYNAMIC_BITMAPS 48
#define HM64_CORE_MAP_BITMAP_LIMIT (MAX_BITMAPS - HM64_RESERVED_DYNAMIC_BITMAPS)

static Gfx hm64_bigSpriteDisplayList[2][HM64_BIG_SPRITE_DL_SIZE];
static Vtx hm64_bigBitmapVertices[2][HM64_BIG_BITMAP_VERTEX_SLOTS][4];

extern volatile u32 gGraphicsBufferIndex;
extern u16 bitmapCounter;
extern CoreMapObject coreMapObjects[0x100];

static Gfx* hm64_set_bitmap_blend_mode_display_list(Gfx* dl, u16 flag) {
    switch (flag) {
        case SPRITE_BLEND_DEFAULT:
            break;
        case SPRITE_BLEND_OPAQUE:
            gDPSetCombineMode(dl++, G_CC_MODULATEIDECALA, G_CC_MODULATEIDECALA);
            gDPSetRenderMode(dl++, G_RM_AA_ZB_OPA_SURF, G_RM_NOOP2);
            break;
        case SPRITE_BLEND_ALPHA_MODULATED:
            gDPSetCombineMode(dl++, G_CC_MODULATEIA, G_CC_MODULATEIA);
            gDPSetRenderMode(dl++, G_RM_AA_ZB_XLU_SURF, G_RM_AA_ZB_XLU_SURF2);
            break;
        case SPRITE_BLEND_ALPHA_DECAL:
            gDPSetCombineMode(dl++, G_CC_MODULATEIDECALA, G_CC_MODULATEIDECALA);
            gDPSetRenderMode(dl++, G_RM_AA_ZB_TEX_EDGE, G_RM_AA_ZB_TEX_EDGE2);
            break;
        case SPRITE_BLEND_ALPHA_DECAL_NO_Z:
            gDPSetCombineMode(dl++, G_CC_MODULATEIDECALA, G_CC_MODULATEIDECALA);
            gDPSetRenderMode(dl++, G_RM_AA_TEX_EDGE, G_RM_AA_TEX_EDGE2);
            break;
    }

    return dl;
}

static void hm64_calculate_scene_node_position(Vec3f* calculatedPosition, BitmapObject* sprite) {
    s32 scaledWidth;
    s32 scaledHeight;

    calculatedPosition->x = sprite->viewSpacePosition.x;
    calculatedPosition->y = sprite->viewSpacePosition.y;
    calculatedPosition->z = sprite->viewSpacePosition.z;

    scaledWidth = (sprite->width / 2) * sprite->scaling.x;
    scaledHeight = (sprite->height / 2) * sprite->scaling.y;

    if (((sprite->renderingFlags >> 3) & 3) == SPRITE_ANCHOR_POSITIVE) {
        calculatedPosition->x = sprite->viewSpacePosition.x + scaledWidth;
    }
    if (((sprite->renderingFlags >> 3) & 3) == SPRITE_ANCHOR_NEGATIVE) {
        calculatedPosition->x = sprite->viewSpacePosition.x - scaledWidth;
    }
    if (((sprite->renderingFlags >> 5) & 3) == SPRITE_ANCHOR_POSITIVE) {
        calculatedPosition->y = sprite->viewSpacePosition.y + scaledHeight;
    }
    if (((sprite->renderingFlags >> 5) & 3) == SPRITE_ANCHOR_NEGATIVE) {
        calculatedPosition->y = sprite->viewSpacePosition.y - scaledHeight;
    }
}

static void hm64_process_bitmap_scene_node(BitmapObject* sprite, Gfx* dl) {
    Vec3f vec;
    u16 spriteIndex;

    hm64_calculate_scene_node_position(&vec, sprite);

    spriteIndex = addSceneNode(dl, (sprite->flags & (0x8 | SCENE_NODE_UPDATE_SCALE | SCENE_NODE_UPDATE_ROTATION |
                                                     SCENE_NODE_TRANSFORM_EXEMPT)) |
                                       SCENE_NODE_Z_OFFSET);

    addSceneNodePosition(spriteIndex, vec.x, vec.y, vec.z);
    addSceneNodeScaling(spriteIndex, sprite->scaling.x, sprite->scaling.y, sprite->scaling.z);
    addSceneNodeRotation(spriteIndex, sprite->rotation.x, sprite->rotation.y, sprite->rotation.z);
}

static inline u8* hm64_get_texture_ptr_inline(u8 spriteIndex, u32* textureIndex) {
    return (u8*)textureIndex + textureIndex[spriteIndex];
}

static inline u8* hm64_get_palette_ptr_type_inline(u8 index, u32* paletteIndex) {
    return (u8*)paletteIndex + paletteIndex[index];
}

RECOMP_PATCH Gfx* generateBitmapDisplayList(Gfx* dl, BitmapObject* bitmap, u16 spriteNumber) {
    u16 vtxIndex;
    u32 textureDimensions;
    u16 textureOffset;
    u16 textureHeight;
    u16 textureWidth;
    u16 textureSize;
    u16 remainingSize;

    bitmap->spriteNumber = spriteNumber;
    dl = hm64_set_bitmap_blend_mode_display_list(dl, (bitmap->renderingFlags >> 10) & 7);

    gSPTexture(dl++, 0x8000, 0x8000, 0, G_TX_RENDERTILE, G_ON);

    if (bitmap->flags & BITMAP_USE_BILINEAR_FILTERING) {
        gDPSetTextureFilter(dl++, G_TF_BILERP);
    } else {
        gDPSetTextureFilter(dl++, G_TF_POINT);
    }

    switch (bitmap->pixelSize) {
        case G_IM_SIZ_4b:
            textureHeight = 4096 / bitmap->width;
            textureWidth = bitmap->width / 2;
            break;
        case G_IM_SIZ_8b:
            textureHeight = 2048 / bitmap->width;
            textureWidth = bitmap->width;
            break;
        case G_IM_SIZ_16b:
            textureHeight = 2048 / bitmap->width;
            textureWidth = bitmap->width * 2;
            break;
        case G_IM_SIZ_32b:
            textureHeight = 2048 / bitmap->width;
            textureWidth = bitmap->width * 4;
            break;
        default:
            bitmap->vtxIndex = 0;
            return dl;
    }

    remainingSize = bitmap->height;
    textureOffset = 0;
    textureDimensions = 0;
    vtxIndex = 0;

    do {
        if (((u32)bitmap->spriteNumber + vtxIndex) >= HM64_BIG_BITMAP_VERTEX_SLOTS) {
            break;
        }

        textureSize = remainingSize;

        if (textureSize > textureHeight) {
            textureSize = textureHeight;
        }

        setupBitmapVertices(&hm64_bigBitmapVertices[gGraphicsBufferIndex][bitmap->spriteNumber + vtxIndex][0],
                            bitmap->width, bitmap->height, textureSize, textureOffset,
                            bitmap->renderingFlags & BITMAP_RENDERING_FLIP_HORIZONTAL,
                            bitmap->renderingFlags & BITMAP_RENDERING_FLIP_VERTICAL, bitmap->anchorX,
                            bitmap->anchorY, bitmap->renderingFlags, bitmap->rgba.r, bitmap->rgba.g,
                            bitmap->rgba.b, bitmap->rgba.a);

        dl = loadBitmapTexture(dl, bitmap, textureDimensions, textureSize);
        gSPVertex(dl++, &hm64_bigBitmapVertices[gGraphicsBufferIndex][bitmap->spriteNumber + vtxIndex][0], 4, 0);

        if (bitmap->renderingFlags & SPRITE_RENDERING_REVERSE_WINDING) {
            gSP2Triangles(dl++, 0, 2, 1, 0, 0, 3, 2, 0);
        } else {
            gSP2Triangles(dl++, 0, 1, 2, 0, 0, 2, 3, 0);
        }

        remainingSize -= textureSize;
        textureOffset += textureSize;
        vtxIndex++;
        textureDimensions += textureSize * textureWidth;
        gDPPipeSync(dl++);
    } while (remainingSize);

    gSPEndDisplayList(dl++);
    bitmap->vtxIndex = vtxIndex;

    return dl;
}

RECOMP_PATCH void updateBitmaps(void) {
    u16 i;
    u16 spriteNumber;
    Gfx* dl;
    Gfx* dlStartPosition;

    dl = hm64_bigSpriteDisplayList[gGraphicsBufferIndex];
    spriteNumber = 0;

    for (i = 0; i < MAX_BITMAPS; i++) {
        if (bitmaps[i].flags & BITMAP_ACTIVE) {
            setBitmapFormat(&bitmaps[i], (Texture*)bitmaps[i].timg, bitmaps[i].pal);

            dlStartPosition = dl;
            dl = generateBitmapDisplayList(dl, &bitmaps[i], spriteNumber);
            hm64_process_bitmap_scene_node(&bitmaps[i], dlStartPosition);

            spriteNumber += bitmaps[i].vtxIndex;
            bitmaps[i].flags &= ~BITMAP_ACTIVE;

            if ((dl - hm64_bigSpriteDisplayList[gGraphicsBufferIndex]) >= (HM64_BIG_SPRITE_DL_SIZE - 16)) {
                break;
            }
        }
    }

    (void)spriteNumber;
}

RECOMP_PATCH void setupCoreMapObjectSprites(MainMap* mainMap) {
    Vec3f vec;
    Vec3f vec2;
    f32 coordinateX;
    f32 coordinateY;
    f32 coordinateZ;
    f32 rotationX;
    f32 rotationY;
    f32 rotationZ;
    f32 scaleX;
    f32 scaleY;
    f32 xPosition;
    f32 yPosition;
    f32 zPosition;
    f32 tileOffsetX;
    f32 tileOffsetZ;
    f32 cameraWorldX;
    f32 cameraWorldZ;
    f32 mapWorldCenterX;
    f32 mapWorldCenterZ;
    u16 bitmapIndex;
    u8* texturePtr;
    u16* palettePtr;
    u8 total;
    u16 i;
    u16 j;
    u16 k;

    i = 0;

    while (i < mainMap->mapState.coreMapObjectsCount) {
        j = 0;
        k = mainMap->coreMapObjectsMetadata[i].unk_0;
        total = mainMap->coreMapObjectsMetadata[i].repeatObjectCount;

        while (j < total) {
            coordinateX = coreMapObjects[k].coordinates.x;
            coordinateY = coreMapObjects[k].coordinates.y;
            yPosition = coordinateY;
            mapWorldCenterX = (mainMap->mapGrid.mapWidth * mainMap->mapGrid.tileSizeX) / 2;
            mapWorldCenterZ = (mainMap->mapGrid.mapHeight * mainMap->mapGrid.tileSizeZ) / 2;
            coordinateZ = coreMapObjects[k].coordinates.z;
            tileOffsetX = mainMap->mapGrid.tileSizeX / 2;
            tileOffsetZ = mainMap->mapGrid.tileSizeZ / 2;
            cameraWorldX = mainMap->mapCameraView.cameraTileX * mainMap->mapGrid.tileSizeX;
            cameraWorldZ = mainMap->mapCameraView.cameraTileZ * mainMap->mapGrid.tileSizeZ;

            xPosition = (coordinateX + mapWorldCenterX) - tileOffsetX;
            zPosition = (coordinateZ + mapWorldCenterZ) - tileOffsetZ;

            xPosition -= cameraWorldX;
            zPosition -= cameraWorldZ;

            xPosition += mainMap->mapCameraView.viewOffset.x;
            yPosition += mainMap->mapCameraView.viewOffset.y;
            zPosition += mainMap->mapCameraView.viewOffset.z;

            vec2.x = ((coordinateX - tileOffsetX) + mainMap->mapState.mapOriginX) / mainMap->mapGrid.tileSizeX;
            vec2.y = 0;
            vec2.z = ((coordinateZ - tileOffsetZ) + mainMap->mapState.mapOriginZ) / mainMap->mapGrid.tileSizeZ;
            vec = vec2;

            if (mainMap->visibilityGrid[(u8)vec.z][(u8)vec.x] && (bitmapCounter < HM64_CORE_MAP_BITMAP_LIMIT)) {
                texturePtr = hm64_get_texture_ptr_inline(mainMap->coreMapObjectsMetadata[i].spriteIndex,
                                                         mainMap->coreMapObjectsTextures);
                palettePtr = (u16*)hm64_get_palette_ptr_type_inline(mainMap->coreMapObjectsMetadata[i].spriteIndex,
                                                                    mainMap->coreMapObjectsPalettes);

                switch (coreMapObjects[k].flags & CORE_MAP_OBJECT_SPRITE_MODE_MASK) {
                    case 0:
                        scaleY = 1.0f;
                        scaleX = scaleY;
                        break;
                    case 4:
                        scaleY = 2.0f;
                        scaleX = scaleY;
                        break;
                    case 8:
                        scaleY = 4.0f;
                        scaleX = scaleY;
                        break;
                    case 0xC:
                        scaleY = 8.0f;
                        scaleX = scaleY;
                        break;
                    default:
                        scaleY = 1.0f;
                        scaleX = scaleY;
                        break;
                }

                if (coreMapObjects[k].flags & CORE_MAP_OBJECT_APPLY_ROTATION) {
                    rotationX = mainMap->mapGlobals.rotation.x;
                    rotationY = mainMap->mapGlobals.rotation.y;
                    rotationZ = mainMap->mapGlobals.rotation.z;

                    switch (coreMapObjects[k].flags & CORE_MAP_OBJECT_ROTATION_MODE_MASK) {
                        case 0x70:
                            rotationY = (s32)(rotationY + 45.0f) % 360;
                            break;
                        case 0x10:
                            rotationY = (s32)(rotationY + 315.0f) % 360;
                            break;
                        case 0x20:
                            rotationY = (s32)(rotationY + 270.0f) % 360;
                            break;
                        case 0x30:
                            rotationY = (s32)(rotationY + 225.0f) % 360;
                            break;
                        case 0x40:
                            rotationY = (s32)(rotationY + 180.0f) % 360;
                            break;
                        case 0x50:
                            rotationY = (s32)(rotationY + 135.0f) % 360;
                            break;
                        case 0x60:
                            rotationY = (s32)(rotationY + 90.0f) % 360;
                            break;
                        case 0:
                        default:
                            break;
                    }
                } else {
                    rotationX = 0.0f;
                    rotationY = 0.0f;
                    rotationZ = 0.0f;
                }

                bitmapIndex = setBitmap(texturePtr, palettePtr, (0x8 | SCENE_NODE_UPDATE_SCALE | SCENE_NODE_UPDATE_ROTATION));

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

                setBitmapRGBA(bitmapIndex, mainMap->mapGlobals.currentRGBA.r, mainMap->mapGlobals.currentRGBA.g,
                              mainMap->mapGlobals.currentRGBA.b, mainMap->mapGlobals.currentRGBA.a);
                setBitmapAnchor(bitmapIndex, 0, 0);
                bitmaps[bitmapIndex].flags |= BITMAP_USE_BILINEAR_FILTERING;
            }

            j++;
            total = mainMap->coreMapObjectsMetadata[i].repeatObjectCount;
            k++;
        }

        i++;
    }
}
