#include "patches.h"
#include "sound.h"

#include "common.h"
#include "nualstl_n.h"

#include "system/audio.h"
#include "game/gameAudio.h"

#include "assetIndices/sequences.h"
#include "assetIndices/sfxs.h"

extern void nuAuStlSeqPlayerDataSet(u8 player_no, u8* seq_addr, u32 seq_size);
extern musHandle nuAuStlSeqPlayerPlay2(u8 player_no);

extern Addresses sequenceRomAddresses[];
extern Addresses sfxRomAddresses[];
extern u8 audioTypeTable[];
extern s32 volumesTable[];

#define HM64_RECOMP_VOLUME_UNSCALED 0
#define HM64_RECOMP_VOLUME_MUSIC 1
#define HM64_RECOMP_VOLUME_AMBIENCE 2

static u8 s_recomp_sequence_volume_type[MAX_ACTIVE_SEQUENCES];

static s32 hm64_scale_volume(s32 volume, float scale) {
    s32 scaled = (s32)(volume * scale);

    if (scaled < 0) {
        return 0;
    }

    if (scaled > MAX_VOLUME) {
        return MAX_VOLUME;
    }

    return scaled;
}

static bool hm64_is_ambience_sequence_id(u16 sequence_id) {
    switch (sequence_id) {
        case RAIN_SFX:
        case TYPHOON_AMBIENCE_1:
        case TYPHOON_AMBIENCE_2:
        case NIGHT_AMBIENCE_SPRING:
        case NIGHT_AMBIENCE_SUMMER:
        case NIGHT_AMBIENCE_AUTUMN:
        case BEACH_AMBIENCE_1:
        case BEACH_AMBIENCE_2:
            return TRUE;
        default:
            return FALSE;
    }
}

static bool hm64_is_ambience_sfx_index(u16 sfx_index) {
    switch (sfx_index) {
        case BIRD_CHIRP:
        case CAT_MEOW:
        case BIRD_CHIRP_2:
        case 72:
        case 73:
        case 78:
            return TRUE;
        default:
            return FALSE;
    }
}

static s32 hm64_get_scaled_sequence_volume(u16 index, s32 volume) {
    switch (s_recomp_sequence_volume_type[index]) {
        case HM64_RECOMP_VOLUME_MUSIC:
            return hm64_scale_volume(volume, recomp_get_music_volume());
        case HM64_RECOMP_VOLUME_AMBIENCE:
            return hm64_scale_volume(volume, recomp_get_ambience_volume());
        default:
            return volume;
    }
}

static s32 hm64_get_scaled_sfx_volume(u32 sfx_index, s32 volume) {
    if (sfx_index > 0 && hm64_is_ambience_sfx_index((u16)(sfx_index - 1))) {
        return hm64_scale_volume(volume, recomp_get_ambience_volume());
    }

    return volume;
}

RECOMP_PATCH void updateAudio(void) {
    u16 i;
    u16 j;
    u8 current;

    for (i = 0; i < MAX_ACTIVE_SEQUENCES; i++) {
        if (gAudioSequences[i].flags & AUDIO_ACTIVE) {
            if (gAudioSequences[i].flags & AUDIO_START) {
                current = i;
                nuAuStlSeqPlayerDataSet(current, gAudioSequences[i].currentAudioSequenceRomAddrStart, gAudioSequences[i].currentAudioSequenceRomAddrEnd - gAudioSequences[i].currentAudioSequenceRomAddrStart);
                gAudioSequences[i].handle = nuAuStlSeqPlayerPlay2(current);
                gAudioSequences[i].flags &= ~AUDIO_START;
            }

            if (gAudioSequences[i].flags & AUDIO_STOP) {
                MusHandleStop(gAudioSequences[i].handle, gAudioSequences[i].speed);
                gAudioSequences[i].flags &= ~AUDIO_STOP;
            }

            interpolateToTarget(&gAudioSequences[i].volumes);
            MusHandleSetVolume(gAudioSequences[i].handle, hm64_get_scaled_sequence_volume(i, gAudioSequences[i].volumes.currentValue));

            gAudioSequences[i].numChannelsPlaying = MusHandleAsk(gAudioSequences[i].handle);

            if (!gAudioSequences[i].numChannelsPlaying) {
                gAudioSequences[i].flags = 0;
                MusHandleStop(gAudioSequences[i].handle, 1);
            }
        }
    }

    for (j = 0; j < MAX_ACTIVE_SFX; j++) {
        if (gSfx[j].flags & AUDIO_ACTIVE) {
            if (gSfx[j].flags & AUDIO_START) {
                gSfx[j].handle = nuAuStlSndPlayerPlay(gSfx[j].sfxIndex);
                gSfx[j].flags &= ~AUDIO_START;
            }

            if (gSfx[j].flags & AUDIO_STOP) {
                MusHandleStop(gSfx[j].handle, 0);
                gSfx[j].flags &= ~AUDIO_STOP;
            }

            MusHandleSetFreqOffset(gSfx[j].handle, gSfx[j].frequency);
            MusHandleSetPan(gSfx[j].handle, gSfx[j].pan);
            MusHandleSetVolume(gSfx[j].handle, hm64_get_scaled_sfx_volume(gSfx[j].sfxIndex, gSfx[j].volume));

            gSfx[j].numSfxPlaying = MusHandleAsk(gSfx[j].handle);

            if (!(gSfx[j].numSfxPlaying)) {
                gSfx[j].flags = 0;
            }
        }
    }
}

RECOMP_PATCH void setCurrentAudioSequence(u16 sequenceIndex) {
    if (sequenceIndex < TOTAL_SEQUENCES) {
        setAudioSequence(0, (u8*)sequenceRomAddresses[sequenceIndex].romAddrStart, (u8*)sequenceRomAddresses[sequenceIndex].romAddrEnd);
        s_recomp_sequence_volume_type[0] = hm64_is_ambience_sequence_id(sequenceIndex) ? HM64_RECOMP_VOLUME_AMBIENCE : HM64_RECOMP_VOLUME_MUSIC;
    } else {
        s_recomp_sequence_volume_type[0] = HM64_RECOMP_VOLUME_UNSCALED;
    }

    setAudioSequenceVolumes(0, 0, 0);
}

RECOMP_PATCH void playSfx(u16 index) {
    u8 audio_type;

    if (index < TOTAL_SFX) {
        audio_type = audioTypeTable[index];

        if (audio_type == SFX_TYPE) {
            setSfx(index + 1);
            setSfxVolume(index + 1, volumesTable[index]);
        } else {
            setAudioSequence(audio_type, (u8*)sfxRomAddresses[index].romAddrStart, (u8*)sfxRomAddresses[index].romAddrEnd);
            s_recomp_sequence_volume_type[audio_type] = hm64_is_ambience_sfx_index(index) ? HM64_RECOMP_VOLUME_AMBIENCE : HM64_RECOMP_VOLUME_UNSCALED;
            setAudioSequenceVolumes(audio_type, volumesTable[index], volumesTable[index]);
        }
    }
}
