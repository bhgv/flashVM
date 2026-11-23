/* Copyright (c) 2026 bhgv
 * EN: Issue native UniBus FIFO and rendezvous channels from a FlashVM initializer.
 * RU: Выдача нативных каналов UniBus FIFO и rendezvous из инициализатора FlashVM.
 */
#define _POSIX_C_SOURCE 200809L
#include "channel_private.h"
#include "managed_values.h"
#include "flashvm/avm1.h"
#include "flashvm/exception.h"
#include "styxrpc/channel_scheduler.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FLASH_CHANNEL_KEY 1U /* Each token scopes its own first channel. */
#define CHANNEL_OPERATION_BUFFERS 2U /* One request and one response per open. */
#define CHANNEL_SCHEMA_WRAPPER 5U /* Outer parentheses and the channel's c[] envelope. */

typedef struct FlashPort
{
    StyxRpcChannelQueue queue;          /* One registered portable queue or rendezvous. */
    StyxRpcChannelScheduler scheduler; /* Owned by the server dispatcher. */
    size_t sizes[FVM_STYXRPC_CHANNEL_QUEUE_SLOTS]; /* Encoded queue lengths. */
    unsigned char *bytes;              /* Owned disjoint queue slots, optional. */
} FlashPort;

typedef struct FlashOperation
{
    StyxRpcChannelFid fid; /* Checked state for one .unibus operation. */
    FlashPort *port;       /* Borrowed until this operation's clunk. */
    unsigned char *bytes; /* Owned request and reply. */
} FlashOperation;


/* EN: Provide full-width monotonic milliseconds for native WAIT guards.
 * RU: Получить полноразрядные монотонные миллисекунды для веток WAIT. */
static uint64_t clock_ms(void *unused)
{
    struct timespec now;
    (void)unused;
    return !clock_gettime(CLOCK_MONOTONIC, &now) ?
        UINT64_C(1000) * (uint64_t)now.tv_sec + (uint64_t)now.tv_nsec / 1000000 : 0;
}


/* EN: Retire unresolved work before freeing the buffers borrowed by the scheduler.
 * RU: Снять ожидающую работу до освобождения заимствованных планировщиком буферов. */
static void close_operation(void *cookie)
{
    FlashOperation *operation = cookie;
    styxrpc_channel_fid_close(&operation->fid);
    free(operation->bytes);
    free(operation);
}


/* EN: Own isolated staging buffers for one authenticated read/write operation.
 * RU: Владеть отдельными буферами одной проверенной операции чтения и записи. */
static int open_operation(void *context, unsigned access, void **cookie)
{
    FlashPort *port = context;
    FlashOperation *operation;
    if (access != (STYXRPC_READ | STYXRPC_WRITE))
    {
        return 0;
    }
    operation = calloc(1, sizeof(*operation));
    if (!operation)
    {
        return 0;
    }
    operation->bytes = malloc(CHANNEL_OPERATION_BUFFERS * STYXRPC_MANAGED_LIMIT);
    if (!operation->bytes)
    {
        free(operation);
        return 0;
    }
    operation->port = port;
    operation->fid.grants = &port->queue.grant;
    operation->fid.grant_count = 1;
    operation->fid.request = operation->bytes;
    operation->fid.request_capacity = STYXRPC_MANAGED_LIMIT;
    operation->fid.reply = operation->bytes + STYXRPC_MANAGED_LIMIT;
    operation->fid.reply_capacity = STYXRPC_MANAGED_LIMIT;
    operation->fid.admit = styxrpc_scheduler_admit;
    operation->fid.cancel = styxrpc_scheduler_cancel;
    operation->fid.context = &port->scheduler;
    *cookie = operation;
    return 1;
}


/* EN: Admit the final complete write on the portable scheduler exactly once.
 * RU: Передать полный запрос переносимому планировщику ровно один раз. */
static int write_operation(void *cookie, uint64_t offset, const void *bytes, size_t size, size_t *sent)
{
    FlashOperation *operation = cookie;
    if (offset > SIZE_MAX || styxrpc_channel_fid_write(&operation->fid, (size_t)offset, bytes, size) == STYXRPC_INVALID)
    {
        return 0;
    }
    *sent = size;
    return 1;
}


/* EN: Poll expiration and read the immutable completion from module-relative offset zero.
 * RU: Проверить сроки и прочитать неизменяемый ответ от нулевого смещения модуля. */
static int read_operation(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    FlashOperation *operation = cookie;
    styxrpc_scheduler_progress(&operation->port->scheduler);
    return offset <= SIZE_MAX &&
        styxrpc_channel_fid_read(&operation->fid, (size_t)offset, bytes, capacity, size) != STYXRPC_INVALID;
}


/* EN: Release storage after the module has revoked every operation on the token.
 * RU: Освободить память после того, как модуль отозвал все операции токена. */
static void release_port(void *context)
{
    FlashPort *port = context;
    free(port->bytes);
    free(port);
}


/* EN: Allocate an empty FIFO or zero-capacity rendezvous using the compiled type.
 * RU: Выделить пустой FIFO или rendezvous нулевой ёмкости с компилированным типом. */
static FlashPort *create_port(const FVM_StyxRpcChannelExport *binding, const StyxRpcSchema *schema)
{
    FlashPort *port = calloc(1, sizeof(*port));
    size_t length = strlen(schema->result) - CHANNEL_SCHEMA_WRAPPER;
    if (!port)
    {
        return NULL;
    }
    port->queue.grant.key = FLASH_CHANNEL_KEY;
    port->queue.grant.shape[0] = '(';
    memcpy(port->queue.grant.shape + 1, schema->result + 3, length);
    port->queue.grant.shape[length + 1] = ')';
    port->queue.capacity = binding->capacity;
    if (binding->capacity)
    {
        port->bytes = malloc(binding->capacity * (size_t)STYXRPC_MANAGED_LIMIT);
        if (!port->bytes)
        {
            release_port(port);
            return NULL;
        }
        port->queue.bytes = port->bytes;
        port->queue.sizes = port->sizes;
        port->queue.stride = STYXRPC_MANAGED_LIMIT;
    }
    if (!styxrpc_scheduler_init(&port->scheduler, &port->queue, 1, clock_ms, NULL))
    {
        release_port(port);
        return NULL;
    }
    return port;
}


/* [GC:] EN: Seed only buffered channels, using the same encoding as the client methods.
 * RU: Заполнить только буферизованный канал тем же форматом, что и методы клиента. */
static int seed_port(FVM_Context *context, FVM_GCRootScope *roots, FlashPort *port, const FVM_Value *value)
{
    unsigned char *scratch;
    StyxRpcGuard guard = {0};
    int ok;
    if (value->type == FVM_VALUE_UNDEFINED)
    {
        return 1;
    }
    if (!port->queue.capacity)
    {
        return 0;
    }
    scratch = malloc(STYXRPC_MANAGED_LIMIT);
    ok = scratch && fvm_styxrpc_channel_pack(context, roots, port->queue.grant.shape, value,
        scratch, port->bytes, &guard);
    if (ok)
    {
        if (guard.value == scratch)
        {
            memcpy(port->bytes, scratch, guard.size);
        }
        port->sizes[0] = guard.size;
        port->queue.count = 1;
    }
    free(scratch);
    return ok;
}


/* [GC:] EN: Run the initializer under GC roots and transfer the exact resource on success.
 * RU: Выполнить инициализатор под корнями GC и передать ресурс при успехе. */
static StyxRpcStatus begin(void *state, const StyxRpcChannelCall *call, StyxRpcChannelOutput *output)
{
    const FVM_StyxRpcChannelExport *binding = state;
    FVM_Context *context = binding->context;
    FVM_GCRootScope *roots;
    FVM_Value args[FVM_STYXRPC_LIST_ITEMS], seed;
    unsigned count;
    FlashPort *port = NULL;
    int ok;
    if (fvm_exception_get(context, &seed) != FVM_NOT_FOUND ||
        fvm_gc_roots_begin(context, &roots) != FVM_OK)
    {
        return STYXRPC_REMOTE_ERROR;
    }
    ok = fvm_styxrpc_values_decode(context, roots, NULL, binding->schema.arguments, &call->arguments,
            args, FVM_STYXRPC_LIST_ITEMS, &count) &&
        fvm_avm1_call(context, binding->initializer, NULL, count, args, &seed) == FVM_OK &&
        fvm_gc_roots_add(roots, &seed) == FVM_OK &&
        (port = create_port(binding, &binding->schema)) != NULL && seed_port(context, roots, port, &seed);
    fvm_gc_roots_end(roots);
    if (!ok)
    {
        if (port)
        {
            release_port(port);
        }
        fvm_exception_clear(context);
        return STYXRPC_REMOTE_ERROR;
    }
    output->key = port->queue.grant.key;
    output->unbuffered = !port->queue.capacity;
    output->resource.context = port;
    output->resource.open = open_operation;
    output->resource.read = read_operation;
    output->resource.write = write_operation;
    output->resource.close = close_operation;
    output->release = release_port;
    return STYXRPC_OK;
}


/* [GC:] EN/RU: See flashvm/styxrpc_channel.h. / См. flashvm/styxrpc_channel.h. */
FVM_Result fvm_styxrpc_channel_export(FVM_StyxRpcChannelExport *binding,
    const char *name, uint32_t signature, StyxRpcChannelExport *exported)
{
    const char *end;
    size_t size;
    unsigned char offer[STYXRPC_MANAGED_PROFILE_BYTES];
    if (!binding || !binding->context || !binding->initializer || !*binding->initializer || !exported ||
        binding->capacity > FVM_STYXRPC_CHANNEL_QUEUE_SLOTS || !fvm_styxrpc_channel_supported(&binding->schema) ||
        binding->schema.result[1] != 'c' ||
        !(end = fvm_styxrpc_type_end(binding->schema.result + 1)) || strcmp(end, ")") ||
        styxrpc_channel_profile(name, signature, &binding->schema, offer, sizeof(offer), &size) != STYXRPC_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    exported->name = name;
    exported->signature = signature;
    exported->schema = binding->schema;
    exported->begin = begin;
    exported->context = binding;
    return FVM_OK;
}
