#include "modding.h"

#include "game/objects/object.h"
#include "recompconfig.h"
#include "sys/rand.h"

#include "recomp/dlls/_asm/215_recomp.h"

static Object *obj = NULL;

RECOMP_HOOK("copy_obj_position_mirrors") void objsetup_hook(Object *_obj) {
    obj = _obj;
}
RECOMP_HOOK_RETURN("copy_obj_position_mirrors") void objsetup_hook_ret(void) {
    if (obj == NULL) return;
    if (obj->def == NULL) return;

    if (rand_next(0, 99) >= (f32)recomp_get_config_double("random_size_chance")) return;
    
    switch (obj->def->dllID) {
        case 0x8074: // trigger
            return;
    }

    s32 randomRange = (s32)recomp_get_config_double("random_size_amount");

    f32 multiplier = (f32)rand_next(-randomRange, randomRange) / 100.0f;

    obj->srt.scale += obj->srt.scale * multiplier;

    ObjectShadow *shadow = obj->shadow;
    if (shadow != NULL) {
        shadow->scale += shadow->scale * multiplier;
        shadow->maxDistScale += shadow->maxDistScale * multiplier;
    }

    obj = NULL;
}

// RECOMP_HOOK_DLL(dll_215_setup) void sharpclaw_setup_hook(Object *self) {
//     f32 multiplier = (f32)rand_next(-75, 75) / 100.0f;

//     self->srt.scale += self->def->scale * multiplier;

//     ObjectShadow *shadow = self->shadow;
//     if (shadow != NULL) {
//         shadow->scale += shadow->scale * multiplier;
//         shadow->maxDistScale += shadow->maxDistScale * multiplier;
//     }
// }
