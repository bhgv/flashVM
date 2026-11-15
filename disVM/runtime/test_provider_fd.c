/* Copyright (c) 2026 bhgv
 * EN: Test-only provider injection at real FD publication; observe native close.
 * RU: Только тестовая инъекция provider при реальной FD publication и native close.
 */
#include "file_slots.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Each exclusive native provider owns private bytes and one cursor.
 * Каждый exclusive native provider владеет private bytes и одним cursor. */
typedef struct Memory
{
    char bytes[5]; /* Exact fixed-length payload, never allocated in guest heap. */
    size_t cursor; /* Serialized through the production FD gate. */
    unsigned zero_writes; /* Actual zero-byte callback dispatch count. */
    int directory; /* List capability, independent of underlying native fixture kind. */
    int writable; /* Factory-selected rights; no dynamic rights escalation. */
} Memory;
static pthread_mutex_t sync = PTHREAD_MUTEX_INITIALIZER; /* Test counters only. */
static unsigned created; /* Provider ownership transfers, protected by sync. */
static unsigned closed; /* Genuine consuming close count, protected by sync. */
static unsigned incorrect; /* Payload mismatches on close, protected by sync. */
void __real_dvm_file_slots_publish(int, FVM_PortFile *, FVM_PortDirectory *);
void __real_dvm_close_worker_stop(void);


/* EN/RU: Publish copied metadata for borrowed Memory; no OS path lookup.
 * Вернуть metadata заимствованного Memory; без поиска OS path. */
static FVM_Result info(void *state, FVM_FileInfo *out)
{
    Memory *memory = state;
    if (!out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    memset(out, 0, sizeof(*out));
    strcpy(out->name, memory->directory ? "memory-dir" : memory->writable ? "memory-write" : "memory-read");
    out->length.low = sizeof(memory->bytes);
    out->directory = memory->directory;
    out->mode = memory->directory ? 0555 : memory->writable ? 0222 : 0444;
    return FVM_OK;
}


/* EN/RU: Copy at most size bytes from the gated cursor; received acknowledges.
 * Скопировать до size bytes от gated cursor; received сообщает результат. */
static FVM_Result read_bytes(void *state, void *bytes, size_t size, size_t *received)
{
    Memory *memory = state;
    if (!received || (!bytes && size))
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (size > sizeof(memory->bytes) - memory->cursor)
    {
        size = sizeof(memory->bytes) - memory->cursor;
    }
    if (size)
    {
        memcpy(bytes, memory->bytes + memory->cursor, size);
    }
    memory->cursor += size;
    *received = size;
    return FVM_OK;
}


/* EN/RU: Replace bounded bytes at gated cursor; refusal leaves bytes unchanged.
 * Заменить bounded bytes у gated cursor; отказ не меняет bytes. */
static FVM_Result write_bytes(void *state, const void *bytes, size_t size)
{
    Memory *memory = state;
    if ((!bytes && size) || size > sizeof(memory->bytes) - memory->cursor)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (size)
    {
        memcpy(memory->bytes + memory->cursor, bytes, size);
    }
    memory->zero_writes += size == 0;
    memory->cursor += size;
    return FVM_OK;
}


/* EN/RU: Accept absolute seeks in 0..5 only; keep cursor/output on refusal.
 * Принимать только absolute seek 0..5; сохранять cursor/output при отказе. */
static FVM_Result seek_bytes(void *state, long offset, int origin, long *position)
{
    Memory *memory = state;
    if (origin || offset < 0 || offset > (long)sizeof(memory->bytes) || !position)
    {
        return FVM_INVALID_ARGUMENT;
    }
    memory->cursor = (size_t)offset;
    *position = offset;
    return FVM_OK;
}


/* EN/RU: Consume native state on the actual close worker; verify exact payload.
 * Поглотить native state на настоящем close worker; проверить payload. */
static void close_bytes(void *state)
{
    Memory *memory = state;
    pthread_mutex_lock(&sync);
    incorrect += memcmp(memory->bytes, memory->writable ? "XBCZ!" : "ABCDE", sizeof(memory->bytes)) != 0;
    incorrect += memory->writable && memory->zero_writes != 1;
    closed++;
    pthread_mutex_unlock(&sync);
    free(memory);
}
/* EN/RU: Transfer one native metadata batch, then EOF; no guest allocations.
 * Передать один native metadata batch, затем EOF; без guest allocations. */
static FVM_Result list_entries(void *state, FVM_FileInfo **entries, size_t *count)
{
    Memory *memory = state;
    FVM_FileInfo *entry;
    if (memory->cursor)
    {
        *entries = NULL;
        *count = 0;
        return FVM_OK;
    }
    entry = calloc(1, sizeof(*entry));
    if (!entry)
    {
        return FVM_OUT_OF_MEMORY;
    }
    strcpy(entry->name, "entry");
    entry->mode = 0444;
    entry->length.low = 5;
    *entries = entry;
    *count = 1;
    memory->cursor = 1;
    return FVM_OK;
}


/* EN/RU: Mark cursor exhausted after failed managed publication; native-only.
 * Пометить cursor exhausted после failed managed publication; native-only. */
static void abort_entries(void *state)
{
    Memory *memory = state;
    memory->cursor = 1;
}

static const DVM_VfsOps read_ops = {info, read_bytes, NULL, seek_bytes, NULL, close_bytes, NULL}; /* Read capability. */
static const DVM_VfsOps write_ops = {info, NULL, write_bytes, seek_bytes, NULL, close_bytes, NULL}; /* Write capability. */
static const DVM_VfsOps list_ops = {info, NULL, NULL, NULL, list_entries, close_bytes, abort_entries}; /* List capability. */


/* EN/RU: Replace only named fixture files with owned memory providers. Ordinary
 * loader/native publications retain the real implementation. No guest pointers.
 * Подменить только named fixture files своими memory providers; обычный loader
 * использует настоящую publication. Guest pointers не используются. */
void __wrap_dvm_file_slots_publish(int descriptor, FVM_PortFile *file, FVM_PortDirectory *directory)
{
    FVM_FileInfo metadata;
    Memory *memory;
    DVM_VfsHandle handle = {0};
    if (!file || fvm_port_file_info(file, &metadata) != FVM_OK ||
        (strcmp(metadata.name, "provider-read") && strcmp(metadata.name, "provider-write") && strcmp(metadata.name, "provider-list")))
    {
        __real_dvm_file_slots_publish(descriptor, file, directory);
        return;
    }
    memory = calloc(1, sizeof(*memory));
    if (!memory)
    {
        abort();
    }
    memcpy(memory->bytes, "ABCDE", sizeof(memory->bytes));
    memory->writable = !strcmp(metadata.name, "provider-write");
    memory->directory = !strcmp(metadata.name, "provider-list");
    handle.ops = memory->directory ? &list_ops : memory->writable ? &write_ops : &read_ops;
    handle.state = memory;
    fvm_port_file_close(file);
    pthread_mutex_lock(&sync);
    created++;
    pthread_mutex_unlock(&sync);
    dvm_file_slots_publish_resource(descriptor, &handle);
    if (handle.ops || handle.state)
    {
        abort();
    }
}


/* EN/RU: Join actual native closes, then require 24 exact consuming callbacks.
 * Дождаться native closes, затем потребовать 24 точных consuming callbacks. */
void __wrap_dvm_close_worker_stop(void)
{
    __real_dvm_close_worker_stop();
    if (created != 24 || closed != created || incorrect)
    {
        fprintf(stderr, "FAIL provider lifecycle created=%u closed=%u incorrect=%u\n", created, closed, incorrect);
        exit(8);
    }
    puts("PASS provider lifecycle created=24 closed=24 payloads=24");
}
