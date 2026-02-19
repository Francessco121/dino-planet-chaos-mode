#include "modding.h"
#include "recompconfig.h"

#include "PR/ultratypes.h"
#include "dlls/engine/21_gametext.h"
#include "sys/dll.h"
#include "sys/gfx/texture.h"
#include "sys/rand.h"
#include "dll.h"

static void *hijack_dll_export(DLLFile *dll, s32 exportIdx, void *hijack) {
    u32 *vtbl = DLL_FILE_TO_EXPORTS(dll);

    // +1 to get skip the initial null pointer. Export indices do not include the null at the start of the vtable.
    void **exportPtr = (void**)&vtbl[exportIdx + 1];
    void *original = *exportPtr;
    *exportPtr = hijack;

    return original;
}

#include "recomp/dlls/engine/21_gametext_recomp.h"

/* Note: Globally randomizing text chunks/groups causes crashes. Randomizing strings within chunks is OK. */

extern u16 sBankEntryCount;
extern u8 *sCurrentBank_StrCounts;

typedef char* (*gametext_get_text)(u16 chunk, u16 strIndex);
static gametext_get_text gametext_get_text_original; 
static char* gametext_get_text_hijack(u16 chunk, u16 strIndex);

RECOMP_HOOK_DLL(gametext_ctor) void gametext_ctor_hook(DLLFile *dll) {
    gametext_get_text_original = hijack_dll_export(dll, 5, gametext_get_text_hijack);
}

static char* gametext_get_text_hijack(u16 chunk, u16 strIndex) {
    // @recomp: Randomize string index
    if (rand_next(0, 99) < (f32)recomp_get_config_double("random_text_chance")) {
        strIndex = rand_next(0, sCurrentBank_StrCounts[chunk] - 1);
    }

    return gametext_get_text_original(chunk, strIndex);
}

#include "recomp/dlls/engine/22_subtitles_recomp.h"

// Size: 0x18
typedef struct InnerBss38 {
    Texture *unk0;
    s32 unk4;
    s16 unk8;
    s16 unkA;
    s32 unkC;
    s32 pad10;
    s32 pad14;
} InnerBss38;

// Size: 0x26C
typedef struct StructBss38 {
/*0x00*/ char *unk0[2][8];
/*0x40*/ s32 unk40[2][8];
/*0x80*/ u16 unk80[2][8];
/*0xA0*/ u16 unkA0[2][8];
/*0xC0*/ s16 unkC0[4];
/*0xC8*/ InnerBss38 unkC8[2][8];
/*0x248*/ u16 unk248;
/*0x24C*/ u8 pad24C[0x268 - 0x24A];
/*0x268*/ u16 unk268;
/*0x26A*/ u16 unk26A;
} StructBss38;

extern u32 _data_34;
extern f32 _data_48;
extern u16 _bss_7B0;
extern StructBss38 *_bss_780[3];
extern s32 currentColourCommand;
extern GameTextChunk *_bss_7AC;

extern void dll_22_func_448(void);

RECOMP_PATCH void dll_22_func_368(u16 arg0) {
    // @recomp: Randomize text group that the subtitle pulls from
    if (rand_next(0, 99) < (f32)recomp_get_config_double("random_text_chance")) {
        arg0 = rand_next(0, sBankEntryCount - 1);
    }

    if (_data_34 != 0) {
        if (arg0 == _bss_7B0) {
            _data_48 = _bss_780[0]->unk268;
            return;
        }
        dll_22_func_448();
    }
    _data_34 = 1;
    currentColourCommand = -255;
    _bss_7AC = gDLL_21_Gametext->vtbl->get_chunk(arg0);
    _bss_7B0 = arg0;
}
