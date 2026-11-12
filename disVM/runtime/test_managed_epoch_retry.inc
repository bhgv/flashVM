/* Copyright (c) 2026 bhgv
 * [GC:] EN: Observe actual shared leaf epoch refusal without relying on fallback.
 * RU: Наблюдать настоящий отказ эпохи shared-листа независимо от fallback.
 */
static _Thread_local int leaf_scan_active;    /* Only the current real shared leaf call. */
static _Thread_local int leaf_retry_observed; /* That call saw an actual incomplete epoch. */


int __real_dvm_world_epoch_elapsed(DVM_NativeWorld *, unsigned long);


/* [GC:] EN: Forward world/epoch unchanged and record a genuine epoch wait in
 * this owner's leaf scan. No heap payload or collector response is substituted.
 * RU: Передать world/epoch неизменными и отметить настоящий отказ эпохи внутри
 * обхода листьев владельца. Данные кучи и ответ сборщика не подменяются. */
int __wrap_dvm_world_epoch_elapsed(DVM_NativeWorld *world, unsigned long epoch)
{
    int result = __real_dvm_world_epoch_elapsed(world, epoch);
    if (leaf_scan_active && !result)
    {
        leaf_retry_observed = 1;
    }
    return result;
}
