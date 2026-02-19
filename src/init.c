#include "PR/os.h"
#include "modding.h"
#include "sys/rand.h"

RECOMP_HOOK("game_init") void init(void) {
    rand_set_seed(osGetTime());
}
