#include "modding.h"

#include "recompconfig.h"
#include "recompevents.h"

#include "sys/gfx/model.h"
#include "sys/rand.h"

extern s32 gNumModelsTabEntries;

extern void model_destroy(Model* model);

RECOMP_ON_MODEL_LOAD_CALLBACK void recomp_on_model_load_callback(s32 *idPtr) {
    if (rand_next(0, 99) < (f32)recomp_get_config_double("random_model_chance")) {
        // Don't randomize ID 0 (causes crashes?)
        if (*idPtr != 0) {
            *idPtr = rand_next(1, gNumModelsTabEntries - 1);
        }
    }
}
