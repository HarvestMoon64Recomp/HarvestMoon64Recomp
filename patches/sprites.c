#include "patches.h"

#include "system/graphic.h"
#include "system/globalSprites.h"
#include "system/map.h"
#include "system/sceneGraph.h"
#include "system/sprite.h"

// @recomp increase sprite display list and vertex buffer sizes to prevent overflowing
#define HM64_BIG_SPRITE_DL_SIZE                    0x2000
#define HM64_BIG_BITMAP_VERTEX_SLOTS               0x200
#define HM64_MAX_BITMAPS                           320
#define HM64_RESERVED_DYNAMIC_BITMAPS              48
#define HM64_CORE_MAP_BITMAP_LIMIT                 (HM64_MAX_BITMAPS - HM64_RESERVED_DYNAMIC_BITMAPS)
#define HM64_SCREEN_WIDTH                          320.0f
#define HM64_SCREEN_HEIGHT                         240
#define HM64_CHECKERBOARD_BACKGROUND_SPRITE        0x80
#define HM64_CHECKERBOARD_WIDESCREEN_OVERLAP_X     14.0f
#define HM64_CORE_MAP_OBJECT_MATRIX_GROUP_ID_BASE  0x484D6F00

static BitmapObject hm64_bitmaps[HM64_MAX_BITMAPS];
static Gfx hm64_bigSpriteDisplayList[2][HM64_BIG_SPRITE_DL_SIZE];
static Vtx hm64_bigBitmapVertices[2][HM64_BIG_BITMAP_VERTEX_SLOTS][4];
static f32 hm64_bitmapWidescreenOverlapX[HM64_MAX_BITMAPS];
static u32 hm64_bitmapMatrixGroupId[HM64_MAX_BITMAPS];
static u8 hm64_bitmapNoInterpolate[HM64_MAX_BITMAPS];

extern volatile u32 gGraphicsBufferIndex;
extern u16 bitmapCounter;
extern CoreMapObject coreMapObjects[0x100];
extern void setBitmapMetadata(BitmapMetadata* ptr, u16* data);
extern u16* advanceBitmapMetadataPtr(u16 numFrames, u16* bitmapMetadataPtr);
extern u8* getTexturePtr(u16 spriteIndex, u32* spritesheetIndex);

RECOMP_PATCH void initializeBitmaps(void) {
    u16 i;

    for (i = 0; i < HM64_MAX_BITMAPS; i++) {
        hm64_bitmaps[i].flags = 0;
        hm64_bitmaps[i].renderingFlags = 0;
        hm64_bitmaps[i].spriteNumber = 0;
        hm64_bitmaps[i].vtxIndex = 0;
        hm64_bitmaps[i].viewSpacePosition.x = 0.0f;
        hm64_bitmaps[i].viewSpacePosition.y = 0.0f;
        hm64_bitmaps[i].viewSpacePosition.z = 0.0f;
        hm64_bitmaps[i].scaling.x = 1.0f;
        hm64_bitmaps[i].scaling.y = 1.0f;
        hm64_bitmaps[i].scaling.z = 1.0f;
        hm64_bitmaps[i].rotation.x = 0.0f;
        hm64_bitmaps[i].rotation.y = 0.0f;
        hm64_bitmaps[i].rotation.z = 0.0f;
        hm64_bitmaps[i].rgba.r = 255.0f;
        hm64_bitmaps[i].rgba.g = 255.0f;
        hm64_bitmaps[i].rgba.b = 255.0f;
        hm64_bitmaps[i].rgba.a = 255.0f;
        hm64_bitmapWidescreenOverlapX[i] = 0.0f;
        hm64_bitmapMatrixGroupId[i] = 0;
        hm64_bitmapNoInterpolate[i] = FALSE;
    }

    bitmapCounter = 0;
}

RECOMP_PATCH void resetBitmaps(void) {
    u16 i;

    bitmapCounter = 0;

    for (i = 0; i < HM64_MAX_BITMAPS; i++) {
        hm64_bitmaps[i].flags = 0;
        hm64_bitmapWidescreenOverlapX[i] = 0.0f;
        hm64_bitmapMatrixGroupId[i] = 0;
        hm64_bitmapNoInterpolate[i] = FALSE;
    }
}

RECOMP_PATCH u16 setBitmap(u8* timg, u16* pal, u16 flags) {
    u16 bitmapIndex = bitmapCounter;

    if (bitmapCounter < HM64_MAX_BITMAPS) {
        hm64_bitmaps[bitmapCounter].flags = flags | BITMAP_ACTIVE;
        hm64_bitmaps[bitmapCounter].renderingFlags = 0;
        hm64_bitmaps[bitmapCounter].timg = timg;
        hm64_bitmaps[bitmapCounter].pal = pal;
        bitmapCounter++;
    } else {
        bitmapIndex = 0xFFFF;
    }

    return bitmapIndex;
}

RECOMP_PATCH bool setBitmapAnchorAlignment(u16 index, u16 arg1, u16 arg2) {
    if ((index < HM64_MAX_BITMAPS) && (hm64_bitmaps[index].flags & BITMAP_ACTIVE)) {
        hm64_bitmaps[index].renderingFlags &= ~BITMAP_ANCHOR_MASK;
        hm64_bitmaps[index].renderingFlags |= arg1 << BITMAP_ANCHOR_H_SHIFT;
        hm64_bitmaps[index].renderingFlags |= arg2 << BITMAP_ANCHOR_V_SHIFT;
        return TRUE;
    }

    return FALSE;
}

RECOMP_PATCH bool setBitmapAxisMapping(u16 index, u16 mode) {
    if ((index < HM64_MAX_BITMAPS) && (hm64_bitmaps[index].flags & BITMAP_ACTIVE)) {
        hm64_bitmaps[index].renderingFlags &= ~BITMAP_AXIS_MAPPING_MASK;
        hm64_bitmaps[index].renderingFlags |= mode << BITMAP_AXIS_SHIFT;
        return TRUE;
    }

    return FALSE;
}

RECOMP_PATCH bool setBitmapTriangleWinding(u16 index, u16 arg1) {
    if ((index < HM64_MAX_BITMAPS) && (hm64_bitmaps[index].flags & BITMAP_ACTIVE)) {
        if (arg1) {
            hm64_bitmaps[index].renderingFlags |= SPRITE_RENDERING_REVERSE_WINDING;
        } else {
            hm64_bitmaps[index].renderingFlags &= ~SPRITE_RENDERING_REVERSE_WINDING;
        }

        return TRUE;
    }

    return FALSE;
}

RECOMP_PATCH bool setBitmapFlip(u16 index, bool flipHorizontal, bool flipVertical) {
    if ((index < HM64_MAX_BITMAPS) && (hm64_bitmaps[index].flags & BITMAP_ACTIVE)) {
        if (flipHorizontal) {
            hm64_bitmaps[index].renderingFlags |= BITMAP_RENDERING_FLIP_HORIZONTAL;
        }

        if (flipVertical) {
            hm64_bitmaps[index].renderingFlags |= BITMAP_RENDERING_FLIP_VERTICAL;
        }

        return TRUE;
    }

    return FALSE;
}

RECOMP_PATCH bool setBitmapBlendMode(u16 index, u16 flag) {
    if ((index < HM64_MAX_BITMAPS) && (hm64_bitmaps[index].flags & BITMAP_ACTIVE)) {
        hm64_bitmaps[index].renderingFlags &= ~BITMAP_BLEND_MODE_MASK;
        hm64_bitmaps[index].renderingFlags |= flag << BITMAP_BLEND_SHIFT;
        return TRUE;
    }

    return FALSE;
}

RECOMP_PATCH bool setBitmapViewSpacePosition(u16 index, f32 x, f32 y, f32 z) {
    if ((index < HM64_MAX_BITMAPS) && (hm64_bitmaps[index].flags & BITMAP_ACTIVE)) {
        hm64_bitmaps[index].viewSpacePosition.x = x;
        hm64_bitmaps[index].viewSpacePosition.y = y;
        hm64_bitmaps[index].viewSpacePosition.z = z;
        return TRUE;
    }

    return FALSE;
}

RECOMP_PATCH bool setBitmapScale(u16 index, f32 x, f32 y, f32 z) {
    if ((index < HM64_MAX_BITMAPS) && (hm64_bitmaps[index].flags & BITMAP_ACTIVE)) {
        hm64_bitmaps[index].scaling.x = x;
        hm64_bitmaps[index].scaling.y = y;
        hm64_bitmaps[index].scaling.z = z;
        return TRUE;
    }

    return FALSE;
}

RECOMP_PATCH bool setBitmapRotation(u16 index, f32 x, f32 y, f32 z) {
    if ((index < HM64_MAX_BITMAPS) && (hm64_bitmaps[index].flags & BITMAP_ACTIVE)) {
        hm64_bitmaps[index].rotation.x = x;
        hm64_bitmaps[index].rotation.y = y;
        hm64_bitmaps[index].rotation.z = z;
        return TRUE;
    }

    return FALSE;
}

RECOMP_PATCH bool setBitmapRGBA(u16 index, u8 r, u8 g, u8 b, u8 a) {
    if ((index < HM64_MAX_BITMAPS) && (hm64_bitmaps[index].flags & BITMAP_ACTIVE)) {
        hm64_bitmaps[index].rgba.r = r;
        hm64_bitmaps[index].rgba.g = g;
        hm64_bitmaps[index].rgba.b = b;
        hm64_bitmaps[index].rgba.a = a;
        return TRUE;
    }

    return FALSE;
}

RECOMP_PATCH bool setBitmapAnchor(u16 index, s16 anchorX, s16 anchorY) {
    if ((index < HM64_MAX_BITMAPS) && (hm64_bitmaps[index].flags & BITMAP_ACTIVE)) {
        hm64_bitmaps[index].anchorX = anchorX;
        hm64_bitmaps[index].anchorY = anchorY;
        return TRUE;
    }

    return FALSE;
}

static Gfx* hm64_draw_bitmap_quad(Gfx* dl, BitmapObject* bitmap, u16 vertexSlot, u16 textureSize, u16 textureOffset, s16 xOffset, bool mirrorHorizontal) {
    Vtx* vtxs = &hm64_bigBitmapVertices[gGraphicsBufferIndex][vertexSlot][0];
    u16 flipHorizontal = bitmap->renderingFlags & BITMAP_RENDERING_FLIP_HORIZONTAL;

    if (mirrorHorizontal) {
        flipHorizontal = !flipHorizontal;
    }

    setupBitmapVertices(vtxs, bitmap->width, bitmap->height, textureSize, textureOffset, flipHorizontal, bitmap->renderingFlags & BITMAP_RENDERING_FLIP_VERTICAL, bitmap->anchorX, bitmap->anchorY, bitmap->renderingFlags, bitmap->rgba.r, bitmap->rgba.g, bitmap->rgba.b, bitmap->rgba.a);

    if (xOffset != 0) {
        u16 i;

        for (i = 0; i < 4; i++) {
            vtxs[i].v.ob[0] += xOffset;
        }
    }

    gSPVertex(dl++, vtxs, 4, 0);

    if (bitmap->renderingFlags & SPRITE_RENDERING_REVERSE_WINDING) {
        gSP2Triangles(dl++, 0, 2, 1, 0, 0, 3, 2, 0);
    } else {
        gSP2Triangles(dl++, 0, 1, 2, 0, 0, 2, 3, 0);
    }

    return dl;
}

RECOMP_PATCH void setBitmapFromSpriteObject(u16 spriteIndex, AnimationFrameMetadata* animationFrameMetadataPtr, u8 animationType) {
    BitmapMetadata bitmapMetadata;
    u32 length;
    u16 bitmapIndex;
    u32 texturePtr;
    u16* palettePtr;
    u16 objectCount;
    u16 temp;
    u16 i;

    objectCount = animationFrameMetadataPtr->objectCount;
    length = 0;
    texturePtr = (u32)globalSprites[spriteIndex].texturePtr[gGraphicsBufferIndex];

    for (i = 0; i < objectCount; i++) {
        if (animationType) {
            setBitmapMetadata(&bitmapMetadata, advanceBitmapMetadataPtr((objectCount - i) - 1, (u16*)globalSprites[spriteIndex].bitmapMetadataPtr));

            texturePtr += length;

            if (animationType == 2) {
                setSpriteDMAInfo(bitmapMetadata.spritesheetIndex, globalSprites[spriteIndex].spritesheetIndexPtr, (u8*)texturePtr, globalSprites[spriteIndex].romTexturePtr);
            }

            length = getTextureLength(bitmapMetadata.spritesheetIndex, globalSprites[spriteIndex].spritesheetIndexPtr);
        } else {
            temp = (objectCount - i) - 1;
            setBitmapMetadata(&bitmapMetadata, advanceBitmapMetadataPtr(temp, (u16*)globalSprites[spriteIndex].bitmapMetadataPtr));
            texturePtr = (u32)getTexturePtr(bitmapMetadata.spritesheetIndex, globalSprites[spriteIndex].spritesheetIndexPtr);
        }

        if (globalSprites[spriteIndex].stateFlags & SPRITE_DIRECT_PALETTE_MODE) {
            palettePtr = (u16*)getPalettePtrType1(globalSprites[spriteIndex].paletteIndex, globalSprites[spriteIndex].paletteIndexPtr);
        } else {
            palettePtr = (u16*)getPalettePtrType2(bitmapMetadata.spritesheetIndex, globalSprites[spriteIndex].paletteIndexPtr, globalSprites[spriteIndex].spriteToPaletteMappingPtr);
        }

        if (globalSprites[spriteIndex].stateFlags & SPRITE_NO_TRANSFORM) {
            bitmapIndex = setBitmap((u8*)texturePtr, palettePtr, 0x8 | SCENE_NODE_UPDATE_SCALE | SCENE_NODE_UPDATE_ROTATION | SCENE_NODE_TRANSFORM_EXEMPT);
        } else {
            bitmapIndex = setBitmap((u8*)texturePtr, palettePtr, 0x8 | SCENE_NODE_UPDATE_SCALE | SCENE_NODE_UPDATE_ROTATION);
        }

        if (bitmapIndex == 0xFFFF) {
            break;
        }

        // @recomp don't interpolate bitmaps.
        hm64_bitmaps[bitmapIndex].flags |= (globalSprites[spriteIndex].stateFlags & HM64_WIDESCREEN_FLAGS);
        hm64_bitmapWidescreenOverlapX[bitmapIndex] = (spriteIndex == HM64_CHECKERBOARD_BACKGROUND_SPRITE) ? HM64_CHECKERBOARD_WIDESCREEN_OVERLAP_X : 0.0f;
        hm64_bitmapNoInterpolate[bitmapIndex] = (spriteIndex == HM64_CHECKERBOARD_BACKGROUND_SPRITE);
        setBitmapViewSpacePosition(bitmapIndex, globalSprites[spriteIndex].viewSpacePosition.x, globalSprites[spriteIndex].viewSpacePosition.y, globalSprites[spriteIndex].viewSpacePosition.z);
        setBitmapScale(bitmapIndex, globalSprites[spriteIndex].scale.x, globalSprites[spriteIndex].scale.y, globalSprites[spriteIndex].scale.z);
        setBitmapRotation(bitmapIndex, globalSprites[spriteIndex].rotation.x, globalSprites[spriteIndex].rotation.y, globalSprites[spriteIndex].rotation.z);
        setBitmapRGBA(bitmapIndex, globalSprites[spriteIndex].currentRGBA.r, globalSprites[spriteIndex].currentRGBA.g, globalSprites[spriteIndex].currentRGBA.b, globalSprites[spriteIndex].currentRGBA.a);
        setBitmapAnchor(bitmapIndex, bitmapMetadata.anchorX, bitmapMetadata.anchorY);
        setBitmapFlip(bitmapIndex, globalSprites[spriteIndex].renderingFlags & SPRITE_RENDERING_FLIP_HORIZONTAL, globalSprites[spriteIndex].renderingFlags & SPRITE_RENDERING_FLIP_VERTICAL);
        setBitmapAnchorAlignment(bitmapIndex, (globalSprites[spriteIndex].renderingFlags >> 3) & 3, (globalSprites[spriteIndex].renderingFlags >> 5) & 3);
        setBitmapAxisMapping(bitmapIndex, (globalSprites[spriteIndex].renderingFlags >> 7) & 3);
        setBitmapTriangleWinding(bitmapIndex, globalSprites[spriteIndex].renderingFlags & SPRITE_RENDERING_REVERSE_WINDING);
        setBitmapBlendMode(bitmapIndex, (globalSprites[spriteIndex].renderingFlags >> 10) & 7);

        if (globalSprites[spriteIndex].stateFlags & SPRITE_ENABLE_BILINEAR_FILTERING) {
            hm64_bitmaps[bitmapIndex].flags |= BITMAP_USE_BILINEAR_FILTERING;
        } else {
            hm64_bitmaps[bitmapIndex].flags &= ~BITMAP_USE_BILINEAR_FILTERING;
        }
    }
}

RECOMP_PATCH Gfx* generateBitmapDisplayList(Gfx* dl, BitmapObject* bitmap, u16 spriteNumber) {
    u16 vtxIndex;
    u32 textureDimensions;
    u16 textureOffset;
    u16 textureHeight;
    u16 textureWidth;
    u16 textureSize;
    u16 remainingSize;
    u16 slotsNeeded;
    s16 copyOffset;
    s16 overlapOffset;
    u32 bitmapIndex;
    u32 matrixGroupId;
    bool noInterpolate;
    f32 copyOffsetFloat;
    f32 widescreenOverlapX;
    bool drawLeftCopy;
    bool drawRightCopy;
    bool mirrorCopies;

    bitmap->spriteNumber = spriteNumber;

    switch ((bitmap->renderingFlags >> 10) & 7) {
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

    bitmapIndex = (u32)(bitmap - hm64_bitmaps);
    matrixGroupId = (bitmapIndex < HM64_MAX_BITMAPS) ? hm64_bitmapMatrixGroupId[bitmapIndex] : 0;
    noInterpolate = (bitmapIndex < HM64_MAX_BITMAPS) && hm64_bitmapNoInterpolate[bitmapIndex];
    if (noInterpolate) {
        // @recomp avoid RT64 blending new checkerboard menu backgrounds.
        gEXMatrixGroupNoInterpolate(dl, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
        dl += 2;
    } else if (matrixGroupId) {
        // @recomp group map decorations for RT64 interpolation.
        gEXMatrixGroupDecomposedVertsOrderAuto(dl, matrixGroupId, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
        dl += 2;
    }

    remainingSize = bitmap->height;
    textureOffset = 0;
    textureDimensions = 0;
    vtxIndex = 0;

    do {
        textureSize = remainingSize;

        if (textureSize > textureHeight) {
            textureSize = textureHeight;
        }

        drawLeftCopy = FALSE;
        drawRightCopy = FALSE;

        if ((bitmap->flags & HM64_WIDESCREEN_RENDER_FLAGS) &&
            ((bitmap->renderingFlags >> BITMAP_AXIS_SHIFT) & 3) == SPRITE_BILLBOARD_XY &&
            bitmap->scaling.x != 0.0f) {
            drawLeftCopy = (bitmap->flags & (HM64_WIDESCREEN_EXTEND_LEFT | HM64_WIDESCREEN_FULLSCREEN)) != 0;
            drawRightCopy = (bitmap->flags & (HM64_WIDESCREEN_EXTEND_RIGHT | HM64_WIDESCREEN_FULLSCREEN)) != 0;
        }

        slotsNeeded = 1 + drawLeftCopy + drawRightCopy;

        if (((u32)bitmap->spriteNumber + vtxIndex + slotsNeeded) > HM64_BIG_BITMAP_VERTEX_SLOTS) {
            break;
        }

        dl = loadBitmapTexture(dl, bitmap, textureDimensions, textureSize);
        dl = hm64_draw_bitmap_quad(dl, bitmap, bitmap->spriteNumber + vtxIndex, textureSize, textureOffset, 0, FALSE);
        vtxIndex++;

        if (slotsNeeded > 1) {
            // @recomp draw mirrored or repeated sprite copies to make it fill the entire screen in widescreen
            copyOffsetFloat = HM64_SCREEN_WIDTH / bitmap->scaling.x;
            widescreenOverlapX = 0.0f;

            if (bitmapIndex < HM64_MAX_BITMAPS) {
                widescreenOverlapX = hm64_bitmapWidescreenOverlapX[bitmapIndex];
            }

            copyOffsetFloat = (copyOffsetFloat < -32768.0f) ? -32768.0f : (copyOffsetFloat > 32767.0f) ? 32767.0f : copyOffsetFloat;
            copyOffset = (s16)copyOffsetFloat;

            copyOffsetFloat = widescreenOverlapX / bitmap->scaling.x;
            copyOffsetFloat = (copyOffsetFloat < -32768.0f) ? -32768.0f : (copyOffsetFloat > 32767.0f) ? 32767.0f : copyOffsetFloat;
            overlapOffset = (s16)copyOffsetFloat;
            mirrorCopies = (bitmap->flags & HM64_WIDESCREEN_MIRROR_COPIES) != 0;

            gEXPushScissor(dl++);
            gEXSetScissor(dl, G_SC_NON_INTERLACE, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_RIGHT, 0, 0, 0, HM64_SCREEN_HEIGHT);
            dl += 2;

            if (drawLeftCopy) {
                dl = hm64_draw_bitmap_quad(dl, bitmap, bitmap->spriteNumber + vtxIndex, textureSize, textureOffset, -(copyOffset - overlapOffset), mirrorCopies);
                vtxIndex++;
            }

            if (drawRightCopy) {
                dl = hm64_draw_bitmap_quad(dl, bitmap, bitmap->spriteNumber + vtxIndex, textureSize, textureOffset, copyOffset - overlapOffset, mirrorCopies);
                vtxIndex++;
            }

            gEXPopScissor(dl++);
        }

        remainingSize -= textureSize;
        textureOffset += textureSize;
        textureDimensions += textureSize * textureWidth;
        gDPPipeSync(dl++);
    } while (remainingSize);

    if (noInterpolate || matrixGroupId) {
        gEXPopMatrixGroup(dl++, G_MTX_MODELVIEW);
    }

    gSPEndDisplayList(dl++);
    bitmap->vtxIndex = vtxIndex;

    return dl;
}

RECOMP_PATCH void updateBitmaps(void) {
    u16 i;
    s32 scaledWidth;
    s32 scaledHeight;
    u16 sceneNodeIndex;
    u16 spriteNumber;
    Gfx* dl;
    Gfx* dlStartPosition;
    Vec3f position;

    dl = hm64_bigSpriteDisplayList[gGraphicsBufferIndex];
    spriteNumber = 0;

    for (i = 0; i < HM64_MAX_BITMAPS; i++) {
        if (hm64_bitmaps[i].flags & BITMAP_ACTIVE) {
            setBitmapFormat(&hm64_bitmaps[i], (Texture*)hm64_bitmaps[i].timg, hm64_bitmaps[i].pal);

            dlStartPosition = dl;
            dl = generateBitmapDisplayList(dl, &hm64_bitmaps[i], spriteNumber);

            position.x = hm64_bitmaps[i].viewSpacePosition.x;
            position.y = hm64_bitmaps[i].viewSpacePosition.y;
            position.z = hm64_bitmaps[i].viewSpacePosition.z;
            scaledWidth = (hm64_bitmaps[i].width / 2) * hm64_bitmaps[i].scaling.x;
            scaledHeight = (hm64_bitmaps[i].height / 2) * hm64_bitmaps[i].scaling.y;

            if (((hm64_bitmaps[i].renderingFlags >> 3) & 3) == SPRITE_ANCHOR_POSITIVE) {
                position.x += scaledWidth;
            } else if (((hm64_bitmaps[i].renderingFlags >> 3) & 3) == SPRITE_ANCHOR_NEGATIVE) {
                position.x -= scaledWidth;
            }

            if (((hm64_bitmaps[i].renderingFlags >> 5) & 3) == SPRITE_ANCHOR_POSITIVE) {
                position.y += scaledHeight;
            } else if (((hm64_bitmaps[i].renderingFlags >> 5) & 3) == SPRITE_ANCHOR_NEGATIVE) {
                position.y -= scaledHeight;
            }

            sceneNodeIndex = addSceneNode(dlStartPosition, (hm64_bitmaps[i].flags & (0x8 | SCENE_NODE_UPDATE_SCALE | SCENE_NODE_UPDATE_ROTATION | SCENE_NODE_TRANSFORM_EXEMPT)) | SCENE_NODE_Z_OFFSET);
            addSceneNodePosition(sceneNodeIndex, position.x, position.y, position.z);
            addSceneNodeScaling(sceneNodeIndex, hm64_bitmaps[i].scaling.x, hm64_bitmaps[i].scaling.y, hm64_bitmaps[i].scaling.z);
            addSceneNodeRotation(sceneNodeIndex, hm64_bitmaps[i].rotation.x, hm64_bitmaps[i].rotation.y, hm64_bitmaps[i].rotation.z);

            spriteNumber += hm64_bitmaps[i].vtxIndex;
            hm64_bitmaps[i].flags &= ~BITMAP_ACTIVE;
            hm64_bitmapMatrixGroupId[i] = 0;
            hm64_bitmapNoInterpolate[i] = FALSE;

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
    u32 matrixGroupMapBase;
    u8 total;
    u16 i;
    u16 j;
    u16 k;

    i = 0;
    matrixGroupMapBase = HM64_CORE_MAP_OBJECT_MATRIX_GROUP_ID_BASE ^ ((u32)mainMap->coreMapObjectsTextures & 0x00FFFF00);

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
                texturePtr = (u8*)mainMap->coreMapObjectsTextures +
                             ((u32*)mainMap->coreMapObjectsTextures)[mainMap->coreMapObjectsMetadata[i].spriteIndex];
                palettePtr = (u16*)((u8*)mainMap->coreMapObjectsPalettes +
                                    ((u32*)mainMap->coreMapObjectsPalettes)[mainMap->coreMapObjectsMetadata[i].spriteIndex]);

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
                if (bitmapIndex == 0xFFFF) {
                    return;
                }
                hm64_bitmapMatrixGroupId[bitmapIndex] = matrixGroupMapBase + (((u32)i) << 8) + j;

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
                hm64_bitmaps[bitmapIndex].flags |= BITMAP_USE_BILINEAR_FILTERING;
            }

            j++;
            total = mainMap->coreMapObjectsMetadata[i].repeatObjectCount;
            k++;
        }

        i++;
    }
}
