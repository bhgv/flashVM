/* Copyright (c) 2026 bhgv
 * EN: Exact chunk counts, failure-only metadata and nested provider wrappers.
 * RU: Точный chunk count, metadata только после отказа и provider wrappers.
 */
#include "provider.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Stack-owned test state; all callbacks run synchronously on main.
 * Stack-owned test state; callbacks синхронны на main. */
typedef struct Probe
{
    DVM_IoResult next; /* Explicit backend outcome for the next single operation. */
    unsigned operations; /* Real effects/callback count, never inferred retries. */
    unsigned stats; /* Actual metadata callbacks; success path must leave unchanged. */
    unsigned closes; /* Owned inner consuming close count. */
    unsigned destroys; /* Final retained factory release count. */
    FVM_Result stat_status; /* Independent diagnostic outcome, cannot overwrite next.status. */
} Probe;


/* EN/RU: Require condition or exit with its label; no build-mode assertions.
 * Проверить condition либо выйти с label; без build-mode assertions. */
static void require(int ok, const char *label)
{
    if (!ok)
    {
        fprintf(stderr, "chunk result: %s\n", label);
        exit(2);
    }
}


/* EN/RU: Record the diagnostic query and optionally publish a readonly mode.
 * Посчитать diagnostic query; при успехе вернуть readonly mode. */
static FVM_Result info(void *state, FVM_FileInfo *output)
{
    Probe *probe = state;
    probe->stats++;
    if (probe->stat_status == FVM_OK)
    {
        memset(output, 0, sizeof(*output));
        output->mode = 0444;
    }
    return probe->stat_status;
}


/* EN/RU: Publish one prepared count/status, writing only bounded confirmed bytes.
 * Вернуть prepared count/status, записать только bounded confirmed bytes. */
static void read_chunk(void *state, void *bytes, size_t size, DVM_IoResult *output)
{
    Probe *probe = state;
    probe->operations++;
    *output = probe->next;
    if ((output->flags & DVM_IO_PREFIX) && output->count <= size)
    {
        memset(bytes, 'P', output->count);
    }
}


/* EN/RU: Acknowledge prepared write effects once without replay or buffer mutation.
 * Подтвердить prepared write effects один раз; без replay и изменения buffer. */
static void write_chunk(void *state, const void *bytes, size_t size, DVM_IoResult *output)
{
    Probe *probe = state;
    (void)bytes;
    (void)size;
    probe->operations++;
    *output = probe->next;
}


/* EN/RU: Count logical consuming close, leaving stack storage alive.
 * Посчитать consuming close, оставив stack storage живым. */
static void close_probe(void *state)
{
    Probe *probe = state;
    probe->closes++;
}
static const DVM_VfsOps precise = {info, NULL, NULL, NULL, NULL, close_probe, NULL, read_chunk, write_chunk}; /* Precise-only provider. */


/* EN/RU: Return legacy failure with an untrusted count that must not be copied.
 * Вернуть legacy failure с недостоверным count; его нельзя копировать. */
static FVM_Result legacy_read(void *state, void *bytes, size_t size, size_t *count)
{
    (void)state;
    (void)bytes;
    (void)size;
    *count = 2;
    return FVM_ERROR;
}


/* EN/RU: Legacy write failure provides no trustworthy acknowledgement.
 * Legacy write failure не задаёт достоверного подтверждения. */
static FVM_Result legacy_write(void *state, const void *bytes, size_t size)
{
    (void)state;
    (void)bytes;
    (void)size;
    return FVM_ERROR;
}
static const DVM_VfsOps legacy = {info, legacy_read, legacy_write, NULL, NULL, close_probe, NULL}; /* Unknown legacy effects. */


/* EN/RU: Delegate factory metadata to the test; path has no host interpretation.
 * Делегировать factory metadata тесту; path не является host path. */
static FVM_Result factory_info(void *state, const char *path, FVM_FileInfo *output)
{
    (void)path;
    return info(state, output);
}


/* EN/RU: Return a precise handle for retained-wrapper testing; no effects.
 * Вернуть precise handle для wrapper test; без эффектов. */
static FVM_Result factory_open(void *state, const char *path, const char *mode, DVM_VfsHandle *output)
{
    (void)path;
    (void)mode;
    output->ops = &precise;
    output->state = state;
    return FVM_OK;
}


/* EN/RU: Count final factory cleanup without freeing main-owned state.
 * Посчитать final factory cleanup, не освобождая main-owned state. */
static void destroy(void *state)
{
    Probe *probe = state;
    probe->destroys++;
}
static const DVM_ProviderOps factory = {factory_info, factory_open, destroy, 0, NULL}; /* Wrapper owns references only. */


/* EN/RU: Prepare a fully initialized backend outcome, independent from dispatcher.
 * Подготовить initialized backend outcome независимо от dispatcher. */
static void outcome(Probe *probe, size_t count, FVM_Result status, DVM_IoStop stop, unsigned flags)
{
    memset(&probe->next, 0, sizeof(probe->next));
    probe->next.count = count;
    probe->next.status = status;
    probe->next.stop = stop;
    probe->next.flags = flags;
}


/* EN/RU: Require no success-path stats, preserved partial refusals, unknown legacy
 * effects, zero-progress distinction, malformed-result refusal and one nested stat.
 * Проверить success без stat, partial refusal, unknown effects, zero progress,
 * malformed result и ровно один nested stat. Native-only, zero on success. */
int main(void)
{
    Probe probe = {0};
    DVM_VfsHandle raw = {&precise, &probe}, old = {&legacy, &probe}, wrapped = {0};
    DVM_IoResult result, saved;
    DVM_Provider *provider;
    char bytes[4] = {'X', 'X', 'X', 'X'};
    unsigned before;
    outcome(&probe, 4, FVM_OK, DVM_IO_NONE, DVM_IO_PREFIX | DVM_IO_CHANGED);
    probe.next.generation = 9;
    result = dvm_vfs_read_chunk(&raw, bytes, sizeof(bytes));
    require(result.count == 4 && result.generation == 9 && !probe.stats, "success/change hint caused stat");
    outcome(&probe, 1, FVM_OK, DVM_IO_NONE, DVM_IO_PREFIX);
    result = dvm_vfs_write_chunk(&raw, bytes, sizeof(bytes));
    require(result.count == 1 && !probe.stats, "short success caused stat");
    outcome(&probe, 2, FVM_ERROR, DVM_IO_DENIED, DVM_IO_PREFIX);
    result = dvm_vfs_read_chunk(&raw, bytes, sizeof(bytes));
    require(result.count == 2 && result.status == FVM_ERROR && result.stop == DVM_IO_DENIED &&
            (result.flags & DVM_IO_MODE) && result.observed_mode == 0444 && probe.stats == 1, "partial refusal lost");
    require(dvm_vfs_last_io(&raw, &saved) && saved.count == 2 && saved.stop == DVM_IO_DENIED, "last cause lost");
    probe.stat_status = FVM_NOT_FOUND;
    result = dvm_vfs_write_chunk(&raw, bytes, sizeof(bytes));
    require(result.count == 2 && result.status == FVM_ERROR && result.probe_status == FVM_NOT_FOUND &&
            !(result.flags & DVM_IO_MODE) && probe.stats == 2, "stat failure overwrote operation");
    outcome(&probe, 0, FVM_OK, DVM_IO_NONE, DVM_IO_PREFIX);
    result = dvm_vfs_read_chunk(&raw, bytes, sizeof(bytes));
    require(result.stop == DVM_IO_EOF && probe.stats == 2, "read EOF probed");
    result = dvm_vfs_write_chunk(&raw, bytes, sizeof(bytes));
    require(result.stop == DVM_IO_NO_PROGRESS && result.status != FVM_OK && probe.stats == 3, "write zero became EOF");
    result = dvm_vfs_write_chunk(&raw, NULL, 0);
    require(result.status == FVM_OK && probe.stats == 3, "zero-size success probed");
    result = dvm_vfs_read_chunk(&old, bytes, sizeof(bytes));
    require(!result.count && (result.flags & DVM_IO_UNCERTAIN) && !(result.flags & DVM_IO_PREFIX), "legacy read invented certainty");
    result = dvm_vfs_write_chunk(&old, bytes, sizeof(bytes));
    require(!result.count && (result.flags & DVM_IO_UNCERTAIN), "legacy write invented certainty");
    outcome(&probe, 5, FVM_OK, DVM_IO_NONE, DVM_IO_PREFIX);
    result = dvm_vfs_read_chunk(&raw, bytes, sizeof(bytes));
    require(result.stop == DVM_IO_INVALID && !result.count && (result.flags & DVM_IO_UNCERTAIN), "oversized count accepted");
    provider = dvm_provider_create(&factory, &probe);
    require(provider && dvm_provider_open(provider, ".", "r", &wrapped) == FVM_OK, "retained wrapper");
    dvm_provider_release(provider);
    before = probe.stats;
    outcome(&probe, 1, FVM_ERROR, DVM_IO_AGAIN, DVM_IO_PREFIX | DVM_IO_UNCERTAIN);
    result = dvm_vfs_read_chunk(&wrapped, bytes, sizeof(bytes));
    require(result.count == 1 && result.stop == DVM_IO_AGAIN && (result.flags & DVM_IO_UNCERTAIN) &&
            probe.stats == before + 1, "wrapper repeated stat/effect or lost acknowledgement");
    dvm_vfs_close(&wrapped);
    require(probe.closes == 1 && probe.destroys == 1, "wrapper lifetime");
    puts("PASS precise chunk count/cause, failure-only stat, unknown effects, zero/EOF and retained wrapper");
    return 0;
}
