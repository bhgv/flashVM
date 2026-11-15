/* Copyright (c) 2026 bhgv
 * EN: Real host-installed precise providers; verify no duplicate effects/probes.
 * RU: Настоящие host-installed precise providers; без duplicate effects/probes.
 */
#include "vfs/boot.h"
#include "vfs/resource.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: One guest owns each handle; FIFO serializes callbacks before close.
 * Один guest владеет handle; FIFO сериализует callbacks до close. */
typedef struct Stream
{
    unsigned calls; /* One per actual I/O attempt; no retry allowed by this fixture. */
    unsigned cursor; /* Confirmed prefix position only. */
    int writing; /* Factory-selected direction; independent of native file kind. */
    char bytes[5]; /* Actual written bytes or original ABCDE read payload. */
} Stream;
static unsigned probes; /* Guest-native callbacks only, observed after join. */
static unsigned created; /* Guest-native factory opens, before ownership publication. */
static unsigned closed; /* One close-worker thread, observed after join. */
static unsigned destroyed; /* Final factory callback count. */
int __real_dvm_namespace_boot_initialize(const char *directory);
void __real_dvm_close_worker_stop(void);


/* EN/RU: Fail fixture with a native diagnostic, never expose pointers to guest.
 * Завершить fixture с diagnostic; guest не видит pointers. */
static void require(int ok, const char *label)
{
    if (!ok)
    {
        fprintf(stderr, "chunk guest: %s\n", label);
        abort();
    }
}


/* EN/RU: Handle metadata is diagnostic only and must run exactly after failure.
 * Handle metadata — только diagnostic; вызывать ровно после отказа. */
static FVM_Result stream_info(void *state, FVM_FileInfo *output)
{
    (void)state;
    probes++;
    memset(output, 0, sizeof(*output));
    output->mode = 0444;
    return FVM_OK;
}


/* EN/RU: Return partial denial, denial without progress, short success then EOF.
 * Вернуть partial denial, отказ без progress, short success, затем EOF. */
static void read_chunk(void *state, void *bytes, size_t size, DVM_IoResult *result)
{
    Stream *stream = state;
    require(!stream->writing && size == 5 && stream->calls < 4, "read replay or invalid direction");
    memset(result, 0, sizeof(*result));
    result->flags = DVM_IO_PREFIX;
    result->status = stream->calls < 2 ? FVM_ERROR : FVM_OK;
    result->stop = stream->calls < 2 ? DVM_IO_DENIED : stream->calls == 2 ? DVM_IO_EOF : DVM_IO_NONE;
    result->count = stream->calls == 0 || stream->calls == 2 ? 2 : 0;
    memcpy(bytes, stream->bytes + stream->cursor, result->count);
    stream->cursor += result->count;
    stream->calls++;
}


/* EN/RU: Write confirmed AB, no-progress, C, then D with uncertain remainder.
 * Записать confirmed AB, no-progress, C, затем D с uncertain remainder. */
static void write_chunk(void *state, const void *bytes, size_t size, DVM_IoResult *result)
{
    Stream *stream = state;
    require(stream->writing && stream->calls < 5, "write replay or invalid direction");
    memset(result, 0, sizeof(*result));
    result->flags = DVM_IO_PREFIX;
    result->status = FVM_OK;
    if (stream->calls == 0)
    {
        require(!size, "expected zero-size write");
    }
    else if (stream->calls == 1)
    {
        require(size == 5, "expected first write");
        result->count = 2;
        result->status = FVM_ERROR;
        result->stop = DVM_IO_DENIED;
    }
    else if (stream->calls == 2)
    {
        require(size == 3, "expected no-progress attempt");
    }
    else
    {
        result->count = 1;
        if (stream->calls == 4)
        {
            result->flags |= DVM_IO_UNCERTAIN;
            result->status = FVM_ERROR;
            result->stop = DVM_IO_FAILURE;
        }
    }
    if (result->count)
    {
        memcpy(stream->bytes + stream->cursor, bytes, result->count);
    }
    stream->cursor += result->count;
    stream->calls++;
}


/* EN/RU: Verify cursor/attempts and payload after all native borrowers retire.
 * Проверить cursor/attempts и payload после завершения всех native borrowers. */
static void close_stream(void *state)
{
    Stream *stream = state;
    require(stream->cursor == 4 && stream->calls == (stream->writing ? 5U : 4U), "lost/replayed attempts");
    require(!memcmp(stream->bytes, "ABCD", 4), "lost/replayed bytes");
    closed++;
    free(stream);
}
static const DVM_VfsOps read_ops = {stream_info, NULL, NULL, NULL, NULL, close_stream, NULL, read_chunk, NULL}; /* Precise-only read. */
static const DVM_VfsOps write_ops = {stream_info, NULL, NULL, NULL, NULL, close_stream, NULL, NULL, write_chunk}; /* Precise-only write. */


/* EN/RU: Path metadata precedes open, counted separately from diagnostic probes.
 * Path metadata до open, отдельно от diagnostic probes. */
static FVM_Result factory_info(void *state, const char *path, FVM_FileInfo *output)
{
    (void)state;
    memset(output, 0, sizeof(*output));
    output->directory = !strcmp(path, ".");
    output->mode = 0777;
    return FVM_OK;
}


/* EN/RU: Transfer independent Stream only for declared paths/modes.
 * Передать independent Stream только для заявленных paths/modes. */
static FVM_Result factory_open(void *state, const char *path, const char *mode, DVM_VfsHandle *output)
{
    Stream *stream;
    int writing = !strcmp(path, "write");
    (void)state;
    if ((!writing && strcmp(path, "read")) || strcmp(mode, writing ? "wo" : "r"))
    {
        return FVM_UNSUPPORTED;
    }
    stream = calloc(1, sizeof(*stream));
    if (!stream)
    {
        return FVM_OUT_OF_MEMORY;
    }
    stream->writing = writing;
    if (!writing)
    {
        memcpy(stream->bytes, "ABCDE", 5);
    }
    output->ops = writing ? &write_ops : &read_ops;
    output->state = stream;
    created++;
    return FVM_OK;
}


/* EN/RU: Last factory reference follows both handle closes.
 * Последняя factory reference следует обоим handle close. */
static void destroy(void *state)
{
    (void)state;
    require(created == 2 && closed == 2, "factory destroyed before handles");
    destroyed++;
}
static const DVM_ProviderOps factory = {factory_info, factory_open, destroy, 0, NULL}; /* Private host-installed factory. */


/* EN/RU: Install actual namespace provider at startup, no per-I/O substitution.
 * Установить namespace provider при startup, без подмены per-I/O. */
int __wrap_dvm_namespace_boot_initialize(const char *directory)
{
    DVM_NamespacePlan target;
    DVM_Provider *provider;
    if (!__real_dvm_namespace_boot_initialize(directory))
    {
        return 0;
    }
    provider = dvm_provider_create(&factory, NULL);
    require(provider && dvm_namespace_plan(dvm_namespace_boot_group(), "view", &target) &&
            dvm_namespace_mount_provider(dvm_namespace_boot_group(), &target, provider, "/chunk-provider") == 1, "mount provider");
    dvm_provider_release(provider);
    return 1;
}


/* EN/RU: Join close worker, then require exact failure-only probes and lifetimes.
 * Дождаться close worker, проверить failure-only probes и lifetime. */
void __wrap_dvm_close_worker_stop(void)
{
    __real_dvm_close_worker_stop();
    fprintf(stderr, "CHUNK_TOTALS probes=%u created=%u closed=%u destroyed=%u\n", probes, created, closed, destroyed);
    require(probes == 5 && created == 2 && closed == 2 && destroyed == 1, "probe/lifetime totals");
    puts("PASS precise provider effects once, five failure probes, zero success/EOF probes");
}
