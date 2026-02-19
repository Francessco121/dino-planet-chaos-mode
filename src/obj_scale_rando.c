#include "modding.h"

#include "game/objects/object.h"
#include "recompconfig.h"
#include "sys/rand.h"

#include "recomp/dlls/_asm/215_recomp.h"

static Object *obj = NULL;

/** This runs after an object's setup function is called. */
RECOMP_HOOK("copy_obj_position_mirrors") void objsetup_hook(Object *_obj) {
    obj = _obj;
}
RECOMP_HOOK_RETURN("copy_obj_position_mirrors") void objsetup_hook_ret(void) {
    if (obj == NULL) return;
    if (obj->def == NULL) return;

    if (rand_next(0, 99) >= (f32)recomp_get_config_double("random_size_chance")) return;
    
    // Don't randomize some objects
    switch (obj->def->dllID) {
        case 0x8074: // trigger
            return;
    }

    // Randomize object scale relative to their current scale
    s32 randomRange = (s32)recomp_get_config_double("random_size_amount");
    f32 multiplier = (f32)rand_next(-randomRange, randomRange) / 100.0f;

    obj->srt.scale += obj->srt.scale * multiplier;

    // Also adjust their shadow size to match
    ObjectShadow *shadow = obj->shadow;
    if (shadow != NULL) {
        shadow->scale += shadow->scale * multiplier;
        shadow->maxDistScale += shadow->maxDistScale * multiplier;
    }

    obj = NULL;
}
