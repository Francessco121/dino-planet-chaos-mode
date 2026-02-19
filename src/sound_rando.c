#include "modding.h"

#include "libnaudio/n_libaudio.h"
#include "libnaudio/n_sndplayer.h"
#include "libnaudio/n_sndp.h"
#include "dlls/engine/5_amseq.h"
#include "dlls/engine/6_amsfx.h"
#include "game/objects/object.h"
#include "recompconfig.h"
#include "recompevents.h"
#include "recomputils.h"
#include "sys/fs.h"
#include "sys/asset_thread.h"
#include "sys/main.h"
#include "sys/rand.h"

extern sndstate *g_SndpFreeStatesHead;
extern void func_800668A4(void);

/** 
 * Fade out all currently playing sounds if we run out of state slots.
 * The SFX randomizer often starts bugged(?) sounds that never end which will eventually
 * stop all sounds from playing.
 */
RECOMP_HOOK("func_80066064") void func_80066064_hook(void) {
    // TODO: this causes crashes occasionally! figure out why
    if (g_SndpFreeStatesHead == NULL) {
        recomp_printf("too many sounds!!!\n");
        func_800668A4();
    }
}

#include "recomp/dlls/engine/6_AMSFX_recomp.h"

#define IS_MP3 0x8000
#define PITCH_DEFAULT 100

extern ALBankFile *_bss_0;

static u16 soundID;
static SoundDef* soundDef;

RECOMP_HOOK_DLL(dll_6_func_DE8) void dll_6_func_DE8_hook(u16 soundID_, SoundDef* soundDef_) {
    soundID = soundID_;
    soundDef = soundDef_;
}

RECOMP_HOOK_RETURN_DLL(dll_6_func_DE8) void dll_6_func_DE8_hook_ret(void) {
    if (soundDef == NULL) return;

    if (soundDef->bankAndClipID & IS_MP3) {
        // Randomize MPEG IDs
        if (rand_next(0, 99) >= (f32)recomp_get_config_double("random_dialog_chance")) return;

        s32 numMpegEntries = (get_file_size(MPEG_TAB) / 4) - 1;
        soundDef->bankAndClipID = rand_next(0, numMpegEntries - 1) | IS_MP3;
    } else {
        // Randomize SFX sound IDs
        if (rand_next(0, 99) < (f32)recomp_get_config_double("random_sfx_chance")) {
            soundDef->bankAndClipID = rand_next(0, _bss_0->bankArray[0]->instArray[0]->soundCount - 1);
        }

        // Randomize SFX pitch
        if (rand_next(0, 99) < (f32)recomp_get_config_double("random_sfx_pitch_chance")) {
            s32 randomRange = (s32)recomp_get_config_double("random_sfx_pitch_amount");
            soundDef->pitch += soundDef->pitch * ((f32)rand_next(-randomRange, randomRange) / 100.0f);
        }
    }
}

RECOMP_PATCH void _bnkfPatchWaveTable(ALWaveTable *w, s32 offset, s32 table) {
	if (w->flags) {
		return;
	}

	w->flags = 1;

	w->base += table;

    // @recomp: Disable looping on sounds if enabled. For the SFX randomizer, looping sounds
    //          will often play and never end otherwise which can be a bit overwhelming.
    // TODO: this currently affects music/ambient banks! we don't want that!
	if (w->type == AL_ADPCM_WAVE) {
		w->waveInfo.adpcmWave.book  = (ALADPCMBook *)((u8 *)w->waveInfo.adpcmWave.book + offset);

        if (recomp_get_config_u32("disable_sfx_looping")) {
            w->waveInfo.adpcmWave.loop = 0;
        } else {
            if (w->waveInfo.adpcmWave.loop) {
                w->waveInfo.adpcmWave.loop = (ALADPCMloop *)((u8 *)w->waveInfo.adpcmWave.loop + offset);
            }
        }
	} else if (w->type == AL_RAW16_WAVE) {
        if (recomp_get_config_u32("disable_sfx_looping")) {
            w->waveInfo.rawWave.loop = 0;
        } else {
            if (w->waveInfo.rawWave.loop) {
                w->waveInfo.rawWave.loop = (ALRawLoop *)((u8 *)w->waveInfo.rawWave.loop + offset);
            }
        }
	}
}

#include "recomp/dlls/engine/5_AMSEQ_recomp.h"

typedef struct {
/*00*/ Object *obj;
/*04*/ MusicAction action;
/*24*/ s16 actionNo;
/*26*/ s16 unk26;
/*28*/ s16 unk28;
} AMSEQHandle;

// size:0x24C
typedef struct {
/*000*/ u8 unk0;
/*004*/ N_ALCSPlayer seqp;
/*090*/ ALCSeq seq;
/*118*/ void *midiData;
/*18C*/ u8 currentSeqID;
/*18E*/ s16 bpm;
/*190*/ s16 volume;
/*192*/ u16 unk192;
/*194*/ u16 unk194; // enabled channels?
/*196*/ u16 unk196; // dirty channels?
/*198*/ u16 unk198; // ignore channels?
/*19A*/ u8 volUpRate; // volume per tick
/*19B*/ u8 volDownRate; // volume per tick
/*19C*/ s16 channelVolumes[16];
/*1BC*/ u8 _unk1BC[0x1FC - 0x1BC];
/*1FC*/ u8 nextSeqID; // current music/ambient id?
/*1FE*/ s16 nextBPM;
/*200*/ u16 unk200;
/*202*/ s16 targetVolume;
/*204*/ u16 unk204;
/*206*/ u16 unk206;
/*208*/ u16 unk208;
/*20A*/ u8 nextVolUpRate; // volume per tick
/*20B*/ u8 nextVolDownRate; // volume per tick
/*20C*/ u8 _unk20C[0x24C - 0x20C];
} AMSEQPlayer;

extern AMSEQPlayer **sSeqPlayers;
extern MusicAction* sMusicAction;
extern s32 sNumSeqHandles;
extern AMSEQHandle *sSeqHandles;
extern ALSeqFile *sSeqFiles[2];
extern u16 sCurrGlobalVolume;

extern s32 amseq_func_1E8C(MusicAction *action, s8 a1, s16 actionNo);

RECOMP_PATCH s32 amseq_set(Object *obj, u16 actionNo, const char *filename, s32 lineNo, const char *debugStr) {
    s32 i;
    s32 temp_v1;

    if (!actionNo) {
        return -1;
    }
    // "music %08x,%d\n"
    queue_load_file_region_to_ptr((void** ) sMusicAction, MUSICACTIONS_BIN, (actionNo - 1) * sizeof(MusicAction), sizeof(MusicAction));
    
    // @recomp: Randomize ambience/music seq IDs
    if (sMusicAction->playerNo < 2) {
        // ambience
        if (rand_next(0, 99) < (f32)recomp_get_config_double("random_ambience_chance")) {
            sMusicAction->seqID = rand_next(0, sSeqFiles[0]->seqCount - 1);
        }
    } else {
        // music
        if (rand_next(0, 99) < (f32)recomp_get_config_double("random_music_chance")) {
            sMusicAction->seqID = rand_next(0, sSeqFiles[1]->seqCount - 1);
        }
    }
    
    if (obj != NULL && (sMusicAction->unk18 & sMusicAction->unk1A) != 0) {
        // "object+fade\n"
        temp_v1 = sMusicAction->unk18 & sMusicAction->unk1A;
        if (temp_v1 != 0) { // lolwat
            for (i = 0; i < sNumSeqHandles; i++) {
                if (sSeqHandles[i].obj == obj) {
                    break;
                }
            }
            if (i == sNumSeqHandles) {
                sNumSeqHandles++;
                if (sNumSeqHandles == 16) {
                    // "AUDIO: Maximum sequence handles reached.\n"
                }
                sSeqHandles[i].actionNo = actionNo;
                sSeqHandles[i].obj = obj;
                sSeqHandles[i].unk28 = -1;
                sSeqHandles[i].unk26 = sSeqHandles[i].unk28;
                bcopy(sMusicAction, &sSeqHandles[i].action, sizeof(MusicAction));
                // "registered %d,%08x\n"
            } else {
                // "already registered %d,%08x\n";
            }
            if (temp_v1 != 0xFFFF) {
                // "starting non-fade channels\n"
                return amseq_func_1E8C(sMusicAction, 0, actionNo);
            }
        }
        goto bail;
    }
    // "starting static sequence\n"
    return amseq_func_1E8C(sMusicAction, 0, actionNo);

    bail:
    return -1;
}

RECOMP_PATCH void amseq_play_ex(u8 playerNo, u8 seqID, s16 bpm, s16 volume, u16 arg4) {
    if (seqID == 0) {
        return;
    }
    if (playerNo >= 4) {
        // "amSeqPlayEx: Warning, player value '%d' out of range.\n"
        return;
    }
    // @recomp: Randomize ambience/music seq IDs
    if (playerNo < 2) {
        // ambience
        if (rand_next(0, 99) < (f32)recomp_get_config_double("random_ambience_chance")) {
            seqID = rand_next(0, sSeqFiles[0]->seqCount - 1);
        }
    } else {
        // music
        if (rand_next(0, 99) < (f32)recomp_get_config_double("random_music_chance")) {
            seqID = rand_next(0, sSeqFiles[1]->seqCount - 1);
        }
    }
    if (sSeqFiles[playerNo >> 1]->seqCount < seqID) {
        // "amSeqPlayEx: Warning, sequence value '%d' out of range.\n"
        *((volatile u8*)NULL) = 0;
        return;
    }
    if (!(sSeqPlayers[playerNo]->unk0 & 2) && (sCurrGlobalVolume != 0)) {
        sSeqPlayers[playerNo]->nextSeqID = seqID;
        sSeqPlayers[playerNo]->nextBPM = bpm;
        sSeqPlayers[playerNo]->targetVolume = volume << 4;
        sSeqPlayers[playerNo]->unk200 = arg4;
        sSeqPlayers[playerNo]->unk204 = 0;
        sSeqPlayers[playerNo]->unk206 = 0;
        sSeqPlayers[playerNo]->unk208 = 0;
        sSeqPlayers[playerNo]->nextVolUpRate = 0;
        sSeqPlayers[playerNo]->nextVolDownRate = 0;
    }
}
