#include "patches.h"
#include "graphics.h"

#include "system/message.h"

#define HM64_MESSAGE_BOX_SCREEN_WIDTH 320
#define HM64_MESSAGE_BOX_SCREEN_HEIGHT 240
#define HM64_MESSAGE_BOX_SCISSOR_BASE_PAD_X 4
#define HM64_MESSAGE_BOX_SCISSOR_BASE_PAD_Y 2

typedef struct {
    s32 ulx;
    s32 uly;
    s32 lrx;
    s32 lry;
} MessageScissorRect;

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

static MessageScissorRect hm64_get_message_scissor_rect(MessageBox* messageBox) {
    MessageScissorRect rect;
    s32 padX;
    s32 padY;
    u16 temp;

    temp = (messageBox->fontContext.characterCellWidth / 2);

    rect.ulx = (s32) messageBox->viewSpacePosition.x + (HM64_MESSAGE_BOX_SCREEN_WIDTH / 2) - temp -
               ((messageBox->textBoxLineCharWidth * messageBox->fontContext.characterCellWidth) / 2) + temp -
               ((messageBox->textBoxLineCharWidth * messageBox->characterSpacing) / 2);

    temp = (messageBox->fontContext.characterCellHeight / 2);

    rect.uly = (HM64_MESSAGE_BOX_SCREEN_HEIGHT / 2) - (s32) messageBox->viewSpacePosition.y - temp -
               ((messageBox->textBoxVisibleRows * messageBox->fontContext.characterCellHeight) / 2) + temp -
               ((messageBox->textBoxVisibleRows * messageBox->lineSpacing) / 2);

    rect.lrx = rect.ulx + (messageBox->textBoxLineCharWidth * messageBox->fontContext.characterCellWidth) +
               (messageBox->textBoxLineCharWidth * messageBox->characterSpacing);

    rect.lry = rect.uly + (messageBox->textBoxVisibleRows * messageBox->fontContext.characterCellHeight) +
               (messageBox->textBoxVisibleRows * messageBox->lineSpacing);

    padX = hm64_get_message_scissor_pad_x(messageBox);
    padY = hm64_get_message_scissor_pad_y(messageBox);

    rect.ulx = hm64_clamp_s32(rect.ulx - padX, 0, HM64_MESSAGE_BOX_SCREEN_WIDTH);
    rect.uly = hm64_clamp_s32(rect.uly - padY, 0, HM64_MESSAGE_BOX_SCREEN_HEIGHT);
    rect.lrx = hm64_clamp_s32(rect.lrx + padX, 0, HM64_MESSAGE_BOX_SCREEN_WIDTH);
    rect.lry = hm64_clamp_s32(rect.lry + padY, 0, HM64_MESSAGE_BOX_SCREEN_HEIGHT);

    if (rect.lrx <= rect.ulx) {
        rect.lrx = hm64_clamp_s32(rect.ulx + 1, 1, HM64_MESSAGE_BOX_SCREEN_WIDTH);
    }

    if (rect.lry <= rect.uly) {
        rect.lry = hm64_clamp_s32(rect.uly + 1, 1, HM64_MESSAGE_BOX_SCREEN_HEIGHT);
    }

    return rect;
}

RECOMP_PATCH Gfx* setupMessageBoxScissor(Gfx* dl, MessageBox* messageBox) {

    /*
    u16 ulx, uly, lrx, lry;
    u16 temp;

    *dl++ = D_8011EE98;
    *dl++ = D_8011EEA0;

    temp = (messageBox->fontContext.characterCellWidth / 2);

    ulx = messageBox->viewSpacePosition.x + 160.0f - temp -
          ((messageBox->textBoxLineCharWidth * messageBox->fontContext.characterCellWidth) / 2) + temp -
          ((messageBox->textBoxLineCharWidth * messageBox->characterSpacing) / 2);

    temp = (messageBox->fontContext.characterCellHeight / 2);

    uly = 120.0f - messageBox->viewSpacePosition.y - temp -
          (messageBox->textBoxVisibleRows * messageBox->fontContext.characterCellHeight / 2) + temp -
          (messageBox->textBoxVisibleRows * messageBox->lineSpacing / 2);

    lrx = ulx + (messageBox->textBoxLineCharWidth * messageBox->fontContext.characterCellWidth) +
          (messageBox->textBoxLineCharWidth * messageBox->characterSpacing);

    lry = uly + (messageBox->textBoxVisibleRows * messageBox->fontContext.characterCellHeight) +
          (messageBox->textBoxVisibleRows * messageBox->lineSpacing);
          */

    // @recomp Use MessageScissorRect class instead of separate variables in the original function
    MessageScissorRect rect = hm64_get_message_scissor_rect(messageBox);
    Gfx scissorCommand;

    gDPSetCombineMode(dl++, G_CC_MODULATEIDECALA, G_CC_MODULATEIDECALA);
    gDPSetRenderMode(dl++, G_RM_TEX_EDGE, G_RM_TEX_EDGE2);

    gDPSetScissor(&scissorCommand, G_SC_NON_INTERLACE, rect.ulx, rect.uly, rect.lrx, rect.lry);

    *dl++ = scissorCommand;
    gDPSetTextureFilter(dl++, G_TF_BILERP);

    return dl;
}
