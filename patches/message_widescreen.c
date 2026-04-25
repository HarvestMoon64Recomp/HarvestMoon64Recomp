#include "patches.h"
#include "graphics.h"

#include "system/message.h"

#define HM64_MESSAGE_BOX_SCREEN_WIDTH 320
#define HM64_MESSAGE_BOX_SCREEN_HEIGHT 240
#define HM64_MESSAGE_BOX_SCISSOR_BASE_PAD_X 4
#define HM64_MESSAGE_BOX_SCISSOR_BASE_PAD_Y 2

static s32 hm64_clamp_s32(s32 value, s32 minimum, s32 maximum) {
    if (value < minimum) {
        return minimum;
    }

    if (value > maximum) {
        return maximum;
    }

    return value;
}

static s32 hm64_get_message_scissor_pad_x(const MessageBox* messageBox) {
    u32 width = 0;
    u32 height = 0;
    s32 pad = HM64_MESSAGE_BOX_SCISSOR_BASE_PAD_X + messageBox->fontContext.characterCellWidth +
              messageBox->characterSpacing;

    recomp_get_window_resolution(&width, &height);

    if ((width > 0) && (height > 0)) {
        s32 aspect_milli = (s32) ((width * 3000U) / (height * 4U));

        if (aspect_milli > 1000) {
            pad = (pad * aspect_milli) / 1000;
        }
    }

    return pad;
}

static s32 hm64_get_message_scissor_pad_y(const MessageBox* messageBox) {
    return HM64_MESSAGE_BOX_SCISSOR_BASE_PAD_Y + (messageBox->fontContext.characterCellHeight / 2) +
           messageBox->lineSpacing;
}

RECOMP_PATCH Gfx* setupMessageBoxScissor(Gfx* dl, MessageBox* messageBox) {
    s32 ulx;
    s32 uly;
    s32 lrx;
    s32 lry;
    s32 padX;
    s32 padY;
    u16 temp;
    Gfx tempDl;

    gDPSetCombineMode(dl++, G_CC_MODULATEIDECALA, G_CC_MODULATEIDECALA);
    gDPSetRenderMode(dl++, G_RM_TEX_EDGE, G_RM_TEX_EDGE2);

    temp = (messageBox->fontContext.characterCellWidth / 2);

    ulx = (s32) messageBox->viewSpacePosition.x + (HM64_MESSAGE_BOX_SCREEN_WIDTH / 2) - temp -
          ((messageBox->textBoxLineCharWidth * messageBox->fontContext.characterCellWidth) / 2) + temp -
          ((messageBox->textBoxLineCharWidth * messageBox->characterSpacing) / 2);

    temp = (messageBox->fontContext.characterCellHeight / 2);

    uly = (HM64_MESSAGE_BOX_SCREEN_HEIGHT / 2) - (s32) messageBox->viewSpacePosition.y - temp -
          ((messageBox->textBoxVisibleRows * messageBox->fontContext.characterCellHeight) / 2) + temp -
          ((messageBox->textBoxVisibleRows * messageBox->lineSpacing) / 2);

    lrx = ulx + (messageBox->textBoxLineCharWidth * messageBox->fontContext.characterCellWidth) +
          (messageBox->textBoxLineCharWidth * messageBox->characterSpacing);

    lry = uly + (messageBox->textBoxVisibleRows * messageBox->fontContext.characterCellHeight) +
          (messageBox->textBoxVisibleRows * messageBox->lineSpacing);

    padX = hm64_get_message_scissor_pad_x(messageBox);
    padY = hm64_get_message_scissor_pad_y(messageBox);

    ulx = hm64_clamp_s32(ulx - padX, 0, HM64_MESSAGE_BOX_SCREEN_WIDTH);
    uly = hm64_clamp_s32(uly - padY, 0, HM64_MESSAGE_BOX_SCREEN_HEIGHT);
    lrx = hm64_clamp_s32(lrx + padX, 0, HM64_MESSAGE_BOX_SCREEN_WIDTH);
    lry = hm64_clamp_s32(lry + padY, 0, HM64_MESSAGE_BOX_SCREEN_HEIGHT);

    if (lrx <= ulx) {
        lrx = hm64_clamp_s32(ulx + 1, 1, HM64_MESSAGE_BOX_SCREEN_WIDTH);
    }

    if (lry <= uly) {
        lry = hm64_clamp_s32(uly + 1, 1, HM64_MESSAGE_BOX_SCREEN_HEIGHT);
    }

    gDPSetScissor(&tempDl, G_SC_NON_INTERLACE, ulx, uly, lrx, lry);

    *dl++ = tempDl;
    gDPSetTextureFilter(dl++, G_TF_BILERP);

    return dl++;
}
