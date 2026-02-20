#include "modding.h"
#include "recompconfig.h"
#include "recompevents.h"
#include "recomputils.h"

#include "sys/main.h"
#include "sys/rand.h"

static s32 speedup = 0;
static s32 speedupTimer = 0;

RECOMP_ON_GAME_TICK_START_CALLBACK void speedup_tick(void) {
    s32 minTime = (s32)recomp_get_config_double("speedup_min_time");
    s32 maxTime = (s32)recomp_get_config_double("speedup_max_time");
    s32 maxAmount = (s32)recomp_get_config_double("speedup_max_amount");

    if (minTime == 0 && maxTime == 0) return;
    if (maxAmount == 0) return;

    speedupTimer -= gUpdateRate;

    if (speedupTimer <= 0) {
        speedupTimer = rand_next(minTime, MAX(maxTime, minTime)) * 60;
        speedup = rand_next(-1, maxAmount);
        recomp_printf("new speed: %d\n", speedup);
    }

    gUpdateRate = MAX(1, ((s32)gUpdateRate + speedup));
    gUpdateRateF = (f32) gUpdateRate;
    gUpdateRateInverseF = 1.0f / gUpdateRateF;
    gUpdateRateMirror = gUpdateRate;
    gUpdateRateMirrorF = gUpdateRateF;
    gUpdateRateInverseMirrorF = 1.0f / gUpdateRateMirrorF;
}
