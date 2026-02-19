#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "dlls/engine/18_objfsa.h"
#include "dlls/objects/210_player.h"
#include "dlls/objects/214_animobj.h"
#include "game/gamebits.h"
#include "sys/joypad.h"
#include "sys/objanim.h"
#include "sys/objtype.h"
#include "sys/newshadows.h"
#include "dll.h"

#include "recomp/dlls/objects/210_player_recomp.h"

extern f32 _data_4[];
extern f32 _data_8[];
extern u8 _data_14[4];
extern s16 _data_18[2];
extern f32 _data_6FC[];

extern u8 _bss_14[0x4];
extern f32 _bss_1B0[0x4];
extern f32 _bss_1F8[2];
extern s16 _bss_200;
extern s16 _bss_202;
extern f32 _bss_204;
extern f32 _bss_208;
extern s16 _data_564[];

extern s32 dll_210_func_7E6C(Object* player, Player_Data* arg1, ObjFSA_Data* fsa, Player_Data3B4* arg3, f32 arg4, s32 arg5);
extern s32 dll_210_func_7BC4(Object* player, Player_Data* arg1, u32* arg2, UnkArg4* arg3);
extern void dll_210_func_1DAB0(Object* player);
extern s32 dll_210_func_18E80(Object* player, ObjFSA_Data* fsa, f32 arg2);
extern void dll_210_func_D510(ObjFSA_Data* fsa, f32 arg1);
extern s32 dll_210_func_EFB4(Object* player, ObjFSA_Data* fsa, f32 arg2);
extern void dll_210_func_7260(Object* player, Player_Data* arg1);

static u8 cutsceneActive = FALSE;
static u8 sizeReverted = FALSE;

static void becomeSmall(Object *player) {
    f32 custom_scale;

    if (!player || !player->def) {
        return;
    }
    
    cutsceneActive = (player->unkB0 & 0x1000) ? TRUE : FALSE;

    if (cutsceneActive && (recomp_get_config_u32("affect_cutscenes") == FALSE)) {
        if (sizeReverted == FALSE) {
            player->srt.scale = player->def->scale;
            sizeReverted = TRUE;
        }
        return;   
    }

    custom_scale = recomp_get_config_u32("custom_size");
    player->srt.scale = 0.01f*custom_scale*player->def->scale;
    sizeReverted = FALSE;
}

RECOMP_HOOK_DLL(dll_210_control) void becomeSmallControl(Object* player) {
    becomeSmall(player);
}

RECOMP_HOOK_DLL(dll_210_func_4910) int becomeSmallAnimCallback(Object* player, Object* arg1, AnimObj_Data* arg2, s8 arg3) {
    becomeSmall(player);
    return 1;
}

RECOMP_PATCH s32 dll_210_func_C3D0(Object* player, ObjFSA_Data* fsa, f32 arg2) {
    static f32 _bss_4;
    static f32 _bss_8;
    Player_Data* spAC;
    f32 temp_ft2;
    f32 spA4;
    f32 spA0;
    f32 sp9C;
    f32 temp_fv0;
    f32 temp_fv1;
    f32 var_fa0;
    f32 var_fv0;
    s32 temp_v1;
    s32 temp_v0;
    s32 pad;
    Player_Data3B4 sp44;

    spAC = player->data;
    if (fsa->enteredAnimState != 0) {
        fsa->unk270 = PLAYER_ASTATE_Falling;
    }
    temp_v0 = dll_210_func_7E6C(player, spAC, fsa, &sp44, arg2, 0x14);
    if (temp_v0 == 0x11) {
        return 0x11;
    }
    if (temp_v0 == 0xC) {
        var_fv0 = spAC->unk430.unk8 + 30.0f;
        var_fa0 = spAC->unk430.unk4 - 5.0f;
        temp_fv1 = player->srt.transl.f[1] + 26.0f;
        if (var_fv0 <= temp_fv1 && temp_fv1 <= var_fa0) {
            return 0x1B;
        }
    }
    spAC->unk8B8 = 1;
    fsa->flags |= 0x200000;
    dll_210_func_D510(fsa, arg2);
    switch (player->curModAnimId) {
        case 0x417:
            fsa->animTickDelta = 0.1f;
            gDLL_18_objfsa->vtbl->func7(player, fsa, 1.0f, 1);
            if (player->animProgress > 0.99f) {
                func_80023D30(player, 0x12, 0.0f, 0U);
                // @recomp: Scale jump speed/length with player size
                f32 speedScale = (player->def->scale / player->srt.scale);
                fsa->unk278 = 2.0f * speedScale;
                fsa->animTickDelta = 0.0f;
                _bss_8 = 0.5f;
                _bss_8 = 38.4f * _bss_8 * speedScale;
                _bss_4 = 0.0f;
                player->speed.f[1] = 0.1f * _bss_8;
                _bss_8 = _bss_8 + _bss_8;
                gDLL_6_AMSFX->vtbl->play_sound(player, spAC->unk3B8[0x8], MAX_VOLUME, NULL, NULL, 0, NULL);
                if (*_bss_14 != 0) {
                    player->speed.f[1] *= *_data_8;
                    fsa->unk278 *= *_data_8;
                }
            }
            break;
        case 0x12:
            player->speed.f[1] += -0.1f * arg2;
            _bss_4 += arg2;
            func_800240BC(player, _bss_4 / _bss_8);
            if ((_bss_4 > 10.0f) && (fsa->unk4.unk25C & 0x10)) {
                func_80023D30(player, 0x11, 0.0f, 0U);
                fsa->unk278 = 0.0f;
                fsa->unk27C = 0.0f;
                fsa->animTickDelta = 0.045f;
                gDLL_6_AMSFX->vtbl->play_sound(player, spAC->unk898[func_80025CD4(fsa->unk4.unk68.unk50[0])], MAX_VOLUME, NULL, NULL, 0, NULL);
                gDLL_6_AMSFX->vtbl->play_sound(player, spAC->unk3B8[0x16], MAX_VOLUME, NULL, NULL, 0, NULL);
            } else {
                if ((_bss_8 + 2.0f) < _bss_4) {
                    return 0xE;
                }
                if ((player->speed.f[1] < 0.0f) && (fsa->unk4.underwaterDist > 5.0f)) {
                    return 0x20;
                }
            }
            spA4 = fsin16_precise(player->srt.yaw);
            spA0 = fcos16_precise(player->srt.yaw);
            temp_v1 = arctan2_f(fsa->xAnalogInput, -fsa->yAnalogInput) - fsa->unk324;
            sp9C = fsin16_precise(temp_v1);
            var_fv0 = fcos16_precise(temp_v1);
            temp_fv0 = (spA4 * sp9C) + (spA0 * var_fv0);
            var_fa0 = temp_fv0;
            if (temp_fv0 < 0.0f) {
                var_fa0 = -temp_fv0;
            }
            temp_fv0 = var_fa0;
            var_fv0 = fsa->analogInputPower;
            if (var_fv0 < 0.4f) {
                var_fv0 = 0;
            }
            temp_fv0 = (1.0f - temp_fv0) * var_fv0;
            if (fsa->unk32A > 0) {
                temp_fv0 = -temp_fv0;
            }
            fsa->unk27C += (temp_fv0 - fsa->unk27C) * 0.05f * arg2;
            fsa->unk27C *= 0.98f;
            fsa->unk27C += (temp_fv0 - fsa->unk27C) * 0.015f * arg2;
            if (fsa->unk27C > 0.3f) {
                fsa->unk27C = 0.3f;
            } else if (fsa->unk27C < -0.3f) {
                fsa->unk27C = -0.3f;
            }
            fsa->unk27C *= 0.99f;
            break;
        case 0x11:
            gDLL_18_objfsa->vtbl->func7(player, fsa, 1.0f, 1);
            player->speed.f[1] += -0.1f * arg2;
            if (player->animProgress > 0.99f) {
                player->speed.f[1] = 0.0f;
                return 2;
            }
            break;
        default:
            if (fsa->analogInputPower < *_data_4) {
                *_bss_14 = 1;
            } else {
                *_bss_14 = 0;
            }
            func_80023D30(player, 0x417, 0.0f, 0U);
            break;
    }
    return 0;
}

RECOMP_PATCH s32 dll_210_func_75B0(Object* player, Func_80059C40_Struct* arg1, Player_Data490* arg2, Vec3f* arg3, f32 arg4, f32 arg5) {
    Player_Data *objdata = player->data;
    f32 sp4[4];
    f32 temp_fv1;
    f32 temp;
    f32 f0;

    arg2->unk1C.x = arg1->unk1C.x;
    arg2->unk1C.y = arg1->unk1C.y;
    arg2->unk1C.z = arg1->unk1C.z;

    arg2->unk1C.x = -arg2->unk1C.x;
    arg2->unk1C.y = -arg2->unk1C.y;
    arg2->unk1C.z = -arg2->unk1C.z;
    arg2->unk1C.w = -arg1->unk1C.w;

    arg2->unk2C.x = arg3->x;
    arg2->unk2C.y = arg3->y;
    arg2->unk2C.z = arg3->z;

    arg2->unk4 = ((arg1->unk10 - arg1->unkC) * arg1->unk48) + arg1->unkC;
    arg2->unk8 = arg1->unkC;
    arg2->unk0 = arg2->unk4 - arg2->unk8;

    arg2->unk46 = arg1->unk50;

    sp4[0] = -arg2->unk1C.z;
    sp4[1] = 0.0f;
    sp4[2] = arg2->unk1C.x;
    sp4[3] = -((sp4[0] * arg1->unk4) + (sp4[2] * (0, arg1)->unk14));
    temp_fv1 = ((arg3->z * sp4[2]) + ((sp4[0] * arg3->x) + (sp4[1] * arg3->y))) + sp4[3];
    // @fake
    temp = temp_fv1;
    sp4[0] = -sp4[0];
    sp4[1] = 0.0f;
    sp4[2] = -sp4[2];
    sp4[3] = -((sp4[0] * arg1->unk8) + (sp4[2] * (0, arg1)->unk18));
    f0 = ((arg3->z * sp4[2]) + ((sp4[0] * arg3->x) + (sp4[1] * arg3->y))) + sp4[3];
    if (temp_fv1 < f0) {
        arg2->unk47 = 0;
    } else {
        arg2->unk47 = 1;
    }
    if (arg4 <= (objdata->unk0.unk278 * arg5) || arg4 <= 3.5f) {
        if (arg1->unk50 == 2 || arg1->unk50 == 0x11) {
            return 4;
        }
        // @recomp: Always do a jump when crossing a hit line that is either a jump or ledge grab depending
        //          on walkspeed. This check doesn't work right when the player is scaled down.
        //if (objdata->unk0.unk278 >= 0.94f) {
            return 5;
        //}
        if (arg1->unk50 != 4) {
            return 4;
        }
        return 5;
    }
    return 0;
}

RECOMP_PATCH s32 dll_210_func_E14C(Object* player, ObjFSA_Data* fsa, f32 arg2) {
    f32 temp_fa0;
    f32 temp_ft1;
    f32 temp_fv0;
    Vec3f sp88;
    Vec3f sp7C;
    Vec3f sp70;
    f32 sp6C;
    f32 sp68;
    f32 temp_fv1;
    s16 pad;
    s16 sp60;
    s16 sp5E;
    ModelInstance* sp58;
    Player_Data* objdata;
    Vec3f sp48;
    u8 sp47;

    if (fsa->enteredAnimState != 0) {
        fsa->unk270 = PLAYER_ASTATE_Ledge_Grab_Holding;
        player->speed.f[1] = 0.0f;
    }
    objdata = player->data;
    objdata->unk7FC = 0.0f;
    sp58 = player->modelInsts[player->modelInstIdx];
    {
        s32 temp_v0 = dll_210_func_EFB4(player, fsa, arg2);
        if (temp_v0) { return temp_v0; }
    }
    // @fake
    if ((_bss_200 && _bss_200) && _bss_200) {}
    _bss_202 = _bss_200;
    sp5E = 0;
    sp47 = 0;
    switch (_bss_200) {
    case 0:
        if (fsa->unk33A != 0) {
            player->positionMirror.f[0] = objdata->unk7EC.x;
            player->positionMirror.f[1] = objdata->unk7EC.y;
            player->positionMirror.f[2] = objdata->unk7EC.z;
            inverse_transform_point_by_object(player->positionMirror.f[0], player->positionMirror.f[1], player->positionMirror.f[2], player->srt.transl.f, &player->srt.transl.f[1], &player->srt.transl.f[2], player->parent);
            func_80023D30(player, (s32) _data_564[2], 0.0f, 1U);
            fsa->animTickDelta = 0.01f;
            _bss_202 = _bss_200 = 2;
            func_8001A3FC(sp58, 0U, 0, 0.0f, player->srt.scale, &sp7C, &sp60);
            player->srt.transl.f[1] -= sp7C.f[1];
            temp_fa0 = _bss_1B0[2] + (objdata->unk490.unk4 - objdata->unk7EC.y);
            temp_fa0 = -temp_fa0 * -0.3f;
            if (temp_fa0 >= 0.0f) {
                player->speed.f[1] = sqrtf(temp_fa0);
            } else {
                player->speed.f[1] = 0.0f;
            }
            sp48.f[0] = *_bss_1F8 * objdata->unk490.unk1C.x;
            sp48.f[1] = *_bss_1F8 * objdata->unk490.unk1C.y;
            sp48.f[2] = *_bss_1F8 * objdata->unk490.unk1C.z;
            sp70.f[0] = sp48.f[0] + objdata->unk490.unk2C.f[0];
            sp70.f[1] = sp48.f[1] + objdata->unk490.unk2C.f[1];
            sp70.f[2] = sp48.f[2] + objdata->unk490.unk2C.f[2];
            fsa->unk2EC.y = 0.0f;
            fsa->unk2EC.x = objdata->unk490.unk2C.x - player->srt.transl.f[0];
            fsa->unk2EC.z = objdata->unk490.unk2C.z - player->srt.transl.f[2];
            _bss_204 = player->srt.transl.f[0];
            _bss_208 = player->srt.transl.f[2];
            gDLL_6_AMSFX->vtbl->play_sound(player, objdata->unk3B8[8], MAX_VOLUME, NULL, NULL, 0, NULL);
        } else {
            gDLL_18_objfsa->vtbl->func10(player, fsa, arg2, 0.1f);
        }
        gDLL_2_Camera->vtbl->func10(objdata->unk490.unkC, objdata->unk490.unk10, objdata->unk490.unk14);
        break;
    case 2:
        temp_fa0 = _bss_1B0[2] + objdata->unk490.unk4;
        // @recomp: Remove gravity applied when jumping up to grab a ledge.
        //          Makes it impossible to grab ledges when the player is scaled down.
        //player->speed.f[1] += -0.15f * arg2;
        sp68 = (objdata->unk7EC.y - objdata->unk490.unk8) / (temp_fa0 - objdata->unk490.unk8);
        if (sp68 < 0.0f) {
            sp68 = 0.0f;
        } else if (sp68 > 1.0f) {
            sp68 = 1.0f;
        }
        player->srt.transl.f[0] = (fsa->unk2EC.x * sp68) + _bss_204;
        player->srt.transl.f[2] = (fsa->unk2EC.z * sp68) + _bss_208;
        // @recomp: Cancel ledge grab jump if we start falling. Otherwise, the player will just
        //          fall through the world.
        if (player->speed.f[1] < 0) {
            return 0x15;
        }
        if (temp_fa0 <= objdata->unk7EC.y) {
            _bss_200 = 3;
            sp47 = 1;
            sp6C = 0.035f;
            player->srt.transl.f[1] = objdata->unk490.unk4;
            player->speed.f[1] = 0.0f;
        }
        objdata->unk490.unk10 += (player->srt.transl.f[1] - objdata->unk490.unk10) * 0.02f * arg2;
        gDLL_2_Camera->vtbl->func10(objdata->unk490.unkC, objdata->unk490.unk10, objdata->unk490.unk14);
        shadows_set_custom_obj_pos(player, objdata->unk490.unkC, objdata->unk490.unk10, objdata->unk490.unk14);
        break;
    case 3:
        if (fsa->unk33A != 0) {
            temp_fv0 = fsa->yAnalogInput;
            if (temp_fv0 > 5.0f) {
                _bss_200 = 5;
                sp6C = 0.014f;
                gDLL_6_AMSFX->vtbl->play_sound(player, objdata->unk3B8[rand_next(10, 11)], MAX_VOLUME, NULL, NULL, 0, NULL);
            } else {
                if ((temp_fv0 < -5.0f) && (objdata->unk490.unk46 != 0x11)) {
                    return 0x15;
                }
                _bss_200 = 6;
                sp6C = 0.008f;
            }
        }
        gDLL_2_Camera->vtbl->func10(objdata->unk490.unkC, objdata->unk490.unk10, objdata->unk490.unk14);
        shadows_set_custom_obj_pos(player, objdata->unk490.unkC, objdata->unk490.unk10, objdata->unk490.unk14);
        break;
    case 6:
        if (fsa->yAnalogInput > 5.0f) {
            _bss_200 = 5;
            sp6C = 0.014f;
            gDLL_6_AMSFX->vtbl->play_sound(player, objdata->unk3B8[rand_next(10, 11)], MAX_VOLUME, NULL, NULL, 0, NULL);
        } else if ((fsa->yAnalogInput < -5.0f) && (objdata->unk490.unk46 != 0x11)) {
            return 0x15;
        }
        gDLL_2_Camera->vtbl->func10(objdata->unk490.unkC, objdata->unk490.unk10, objdata->unk490.unk14);
        shadows_set_custom_obj_pos(player, objdata->unk490.unkC, objdata->unk490.unk10, objdata->unk490.unk14);
        break;
    case 5:
        if (fsa->unk33A != 0) {
            player->positionMirror.f[0] = objdata->unk7EC.x;
            player->positionMirror.f[2] = objdata->unk7EC.z;
            inverse_transform_point_by_object(player->positionMirror.f[0], 0.0f, player->positionMirror.f[2], player->srt.transl.f, &sp68, &player->srt.transl.f[2], player->parent);
            dll_210_func_7260(player, objdata);
            func_80023D30(player, (s32) *objdata->modAnims, 0.0f, 1U);
            return 2;
        }
        sp88.z = objdata->unk490.unkC + ((player->srt.transl.x - objdata->unk490.unkC) * player->animProgress);
        sp88.y = objdata->unk490.unk10 + ((objdata->unk490.unk4 - objdata->unk490.unk10) * player->animProgress);
        sp88.x = objdata->unk490.unk14 + ((player->srt.transl.z - objdata->unk490.unk14) * player->animProgress);
        gDLL_2_Camera->vtbl->func10(sp88.z, sp88.y, sp88.x);
        shadows_set_custom_obj_pos(player, sp88.z, sp88.y, sp88.x);
        break;
    default:
        player->speed.f[1] = 0.0f;
        temp_fv0 = (1.0f - ((objdata->unk490.unk0 - 32.0f) / 32));
        sp5E = temp_fv0 * 1023.0f;
        _bss_200 = 0;
        sp6C = 0.029f;
        fsa->unk2F8 = arctan2_f(objdata->unk490.unk1C.x, objdata->unk490.unk1C.z) - player->srt.yaw;
        if (fsa->unk2F8 > 32768.0f) {
            fsa->unk2F8 += -65535.0f;
        }
        if (fsa->unk2F8 < -32768.0f) {
            fsa->unk2F8 += 65535.0f;
        }
        fsa->unk2A0 = 0.0f;
        objdata->unk490.unkC = player->srt.transl.f[0];
        objdata->unk490.unk10 = player->srt.transl.f[1];
        objdata->unk490.unk14 = player->srt.transl.f[2];
        break;
    }
    if (_bss_202 != _bss_200) {
        func_80023D30(player, _data_564[_bss_200], 0.0f, sp47);
        if (sp5E != 0) {
            func_80025540(player, _data_564[_bss_200 + 1], sp5E);
        }
        fsa->animTickDelta = sp6C;
    }
    dll_210_func_7260(player, objdata);
    return 0;
}
