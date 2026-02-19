#include "modding.h"

#include "PR/os.h"
#include "sys/rand.h"

RECOMP_HOOK("game_init") void init(void) {
    // Seed RNG ourselves, otherwise the seed will be consistent on boot (which is no fun)
    rand_set_seed(osGetTime());
}
