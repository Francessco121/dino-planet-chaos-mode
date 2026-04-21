#include "modding.h"
#include "recompconfig.h"
#include "recompevents.h"

#include "PR/ultratypes.h"
#include "sys/gfx/texture.h"
#include "sys/rand.h"

extern s32 gTexTabTextureCounts[2]; // Number of textures in TEX0/TEX1

extern Gfx *tex_setup_display_lists(Texture *texture, Gfx *gdl);

RECOMP_ON_TEX_LOAD_CALLBACK void recomp_on_tex_load_callback(s32 *idPtr) {
    s32 id = *idPtr;

    s32 tabEntry = id & 0xFFFF;
    if (tabEntry & 0x8000) {
        // TEX1
        tabEntry &= 0x7FFF;

        // Randomize TEX1 indices
        if (rand_next(0, 99) < (f32)recomp_get_config_double("random_tex1_chance")) {
            tabEntry = rand_next(0, gTexTabTextureCounts[1] - 1);
        }

        *idPtr = tabEntry | 0x8000;
    } else {
        // TEX0

        if (rand_next(0, 99) < (f32)recomp_get_config_double("random_tex0_chance")) {
            tabEntry = rand_next(0, gTexTabTextureCounts[0] - 1);
        }

        *idPtr = tabEntry;
    }
}

