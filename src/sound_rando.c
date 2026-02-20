#include "modding.h"
#include "dbgui.h"

#include "PR/libaudio.h"
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

#define SNDSTATEFLAG_01 0x01
#define SNDSTATEFLAG_02 0x02
#define SNDSTATEFLAG_04 0x04
#define SNDSTATEFLAG_10 0x10
#define SNDSTATEFLAG_20 0x20

extern sndstate *g_SndpAllocStatesHead;
extern sndstate *g_SndpAllocStatesTail;
extern sndstate *g_SndpFreeStatesHead;
extern N_ALSndPlayer *g_SndPlayer;
extern sndstate *func_80066064(ALBank *bank, ALSound *sound);

#include "recomp/dlls/engine/6_AMSFX_recomp.h"

#define IS_MP3 0x8000
#define PITCH_DEFAULT 100

extern ALBankFile *_bss_0;

static u16 soundID;
static SoundDef* soundDef;

static s32 forceSFX = FALSE;
static s32 forceSFXID = 0;
static s32 soundRandoDebugMenuOpen = FALSE;

RECOMP_CALLBACK("*", recomp_on_dbgui) void sound_rando_dbgui(void) {
    if (dbgui_begin_main_menu_bar()) {
        if (dbgui_begin_menu("Chaos")) {
            dbgui_menu_item("Sound Rando", &soundRandoDebugMenuOpen);
            dbgui_end_menu();
        }
        dbgui_end_main_menu_bar();
    }

    if (!soundRandoDebugMenuOpen) return;

    if (dbgui_begin("Sound Rando", &soundRandoDebugMenuOpen)) {
        dbgui_checkbox("Force SFX ID", &forceSFX);

        if (forceSFX) {
            if (dbgui_input_int("SFX ID", &forceSFXID)) {
                if (forceSFXID < 0) forceSFXID = 0;
                s32 max = _bss_0->bankArray[0]->instArray[0]->soundCount - 1;
                if (forceSFXID > max) forceSFXID = max;
            }
        }

        dbgui_separator();

        if (dbgui_tree_node("Sound States")) {
	        sndstate *state;

            dbgui_text(" == ALLOC LIST ==");
	        state = g_SndpAllocStatesHead;
	        for (s32 i = 0; state; i++, state = (sndstate *)state->node.next) {
                ALMicroTime timeLeft = state->endtime - g_SndPlayer->curTime;
                dbgui_textf("[%d] soundnum: %d | timeleft: %d | state: %d", 
                    i, state->soundnum, timeLeft, state->state);
            }
            dbgui_separator();
            dbgui_text(" == FREE LIST ==");
            state = g_SndpFreeStatesHead;
	        for (s32 i = 0; state; i++, state = (sndstate *)state->node.next) {
                dbgui_textf("[%d] soundnum: %d | state: %d", 
                    i, state->soundnum, state->state);
            }

            dbgui_end_child();
        }
    }
    dbgui_end();
}

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
            if (forceSFX) {
                soundDef->bankAndClipID = forceSFXID;
            }
        }

        // Randomize SFX pitch
        if (rand_next(0, 99) < (f32)recomp_get_config_double("random_sfx_pitch_chance")) {
            s32 randomRange = (s32)recomp_get_config_double("random_sfx_pitch_amount");
            soundDef->pitch += soundDef->pitch * ((f32)rand_next(-randomRange, randomRange) / 100.0f);
        }
    }
}

RECOMP_PATCH sndstate *some_sound_func(ALBank *bank, s16 soundnum, u16 vol, ALPan pan, f32 pitch, u8 fxmix, u8 fxbus, sndstate **handleptr) {
	sndstate *state;
	sndstate *state2 = NULL;
	ALKeyMap *keymap;
	ALSound *sound;
	s16 sp4e = 0;
	s32 sp48;
	s32 sp44;
	s32 sp40 = 0;
	s32 abspan;
	N_ALEvent evt;
	N_ALEvent evt2;

	if (soundnum != 0) {
		do {
			sound = bank->instArray[0]->soundArray[soundnum - 1];
			state = func_80066064(bank, sound);

			if (state != NULL) {
				g_SndPlayer->target = (s32)state;
				evt.type = AL_SNDP_PLAY_EVT;
				evt.msg.generic.sndstate = state;
				abspan = pan + state->pan - AL_PAN_CENTER;

				if (abspan > 127) {
					abspan = 127;
				} else if (abspan < 0) {
					abspan = 0;
				}

				state->pan = abspan;
				state->vol = (u32)(vol * state->vol) >> 15;
				state->pitch *= pitch;
				state->fxmix = fxmix;
				state->fxbus = fxbus;
                // @recomp: Use unused field for debugging the soundnum
                state->soundnum = soundnum;

				sp44 = sound->keyMap->velocityMax * 33333;

				if (state->flags & SNDSTATEFLAG_10) {
					state->flags &= ~SNDSTATEFLAG_10;
					n_alEvtqPostEvent(&g_SndPlayer->evtq, &evt, sp40 + 1, 0);
					sp48 = sp44 + 1;
					sp4e = soundnum;
				} else {
					n_alEvtqPostEvent(&g_SndPlayer->evtq, &evt, sp44 + 1, 0);
				}

                // @recomp: Force SFX to not loop if SFX rando is enabled. For the SFX randomizer, looping sounds
                //          will often play and never end which can be a bit overwhelming.
                if (bank == _bss_0->bankArray[0] && recomp_get_config_double("random_sfx_chance") > 0) {
                    state->flags &= ~SNDSTATEFLAG_02;
                }

				state2 = state;
				if (1) {}
			} else {
                osSyncPrintf("Sound state allocate failed - sndId %d\n", soundnum);
            }

			sp40 += sp44;
			keymap = sound->keyMap;
			soundnum = keymap->velocityMin + (keymap->keyMin & 0xc0) * 4;
		} while (soundnum && state);

		if (state2 != NULL) {
			state2->flags |= 0x1;
			state2->unk30 = handleptr;

			if (sp4e != 0) {
				state2->flags |= SNDSTATEFLAG_10;

				evt2.type = AL_SNDP_0200_EVT;
				evt2.msg.generic.sndstate = state2;
				evt2.msg.generic.data = sp4e;
				evt2.msg.generic.data2 = bank;

				n_alEvtqPostEvent(&g_SndPlayer->evtq, &evt2, sp48, 0);
			}
		}
	}

	if (handleptr != NULL) {
		*handleptr = state2;
	}

	return state2;
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
            // Note: Don't play ambient seq 3, it will crash
            if (sMusicAction->seqID == 3) {
                sMusicAction->seqID = 0;
            }
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
            // Note: Don't play ambient seq 3, it will crash
            if (seqID == 3) {
                seqID = 0;
            }
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
