/* Copyright (c) 2026 bhgv
 * EN: Verify framed admission, pending reads and detached callback lifetime without a VM.
 * RU: Проверка приёма кадров, чтения в ожидании и срока жизни колбэка без VM.
 */
#include "styxrpc/channel_handlers.h"
#include "styxrpc/graph.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define TEST_PID UINT32_C(42) /* Checked reverse process ID carried in the request. */
#define TEST_KEY UINT32_C(7)  /* Checked current-bus channel key. */
#define TEST_FRAME 128U       /* Enough for one small framed request or reply. */

typedef struct Owner
{
    StyxRpcChannelHandlers *module; /* Queue driven by the owner test thread. */
    pthread_mutex_t lock;          /* Protects count, paused and release. */
    pthread_cond_t wake;            /* Signals entry or release of the paused callback. */
    int count;                     /* Number of completed/started callbacks. */
    int paused;                    /* First callback entered and waiting for the dispatcher. */
    int release;                   /* Dispatcher completed its in-flight clunk. */
    int released;                  /* Dropped result cleaned up by the owner. */
} Owner;


/* EN: Check the grant, block the first callback for a concurrent clunk and return void.
 * RU: Проверить полномочие, задержать первый вызов для параллельного clunk и вернуть void. */
static StyxRpcStatus handle(void *context, const StyxRpcChannelCall *call, void *result,
    size_t capacity, size_t *used)
{
    Owner *owner = context;
    (void)result;
    (void)capacity;
    pthread_mutex_lock(&owner->lock);
    ++owner->count;
    pthread_cond_signal(&owner->wake);
    if (owner->count == 1)
    {
        owner->paused = 1;
        while (!owner->release)
        {
            pthread_cond_wait(&owner->wake, &owner->lock);
        }
    }
    pthread_mutex_unlock(&owner->lock);
    *used = 0;
    return call->pid == TEST_PID && call->count == 1 && call->grants[0].key == TEST_KEY ?
        STYXRPC_OK : STYXRPC_REMOTE_ERROR;
}


/* EN: Count the abandoned output's destructor on the callback owner.
 * RU: Подсчитать освобождение отброшенного результата у владельца колбэка. */
static void release_output(void *context)
{
    ((Owner *)context)->released++;
}


/* EN: Pause the active result callback, then hand back a nil channel with cleanup.
 * RU: Задержать активный колбэк, затем выдать канал nil с освобождением. */
static StyxRpcStatus handle_result(void *context, const StyxRpcChannelCall *call, StyxRpcChannelOutput *output)
{
    size_t used;
    StyxRpcStatus status = handle(context, call, NULL, 0, &used);
    output->resource.context = context;
    output->release = release_output;
    return status;
}


/* EN: Fail if a dropped output ever reaches the token registry.
 * RU: Вернуть отказ, если отброшенный результат достиг реестра токенов. */
static int unexpected_entropy(void *context, void *bytes, size_t size)
{
    (void)context;
    (void)bytes;
    (void)size;
    return 0;
}


/* EN: Drive the single owner until the dispatcher stops it.
 * RU: Обслуживать единственного владельца до остановки диспетчером. */
static void *owner_loop(void *context)
{
    Owner *owner = context;
    while (styxrpc_channel_handlers_step(owner->module))
    {
    }
    return NULL;
}


/* EN: Open the function and write a fragmented valid request on its own fid.
 * RU: Открыть функцию и разбить правильный запрос на записи в её собственный fid. */
static int stage(void *module, size_t index, const unsigned char *frame, size_t size, void **file)
{
    const StyxRpcVfs *ops = &styxrpc_channel_handlers_tree.files;
    const StyxRpcEntry *entry = ops->enumerate(module, 2, index);
    size_t sent;
    return entry && ops->open(module, entry->id, STYXRPC_READ | STYXRPC_WRITE, file) &&
        ops->write(*file, 0, frame, 1, &sent) && sent == 1 &&
        ops->write(*file, 1, frame + 1, size - 1, &sent) && sent == size - 1 &&
        !ops->write(*file, size, frame, 1, &sent);
}


/* EN: Generate one exact graph request with a caller PID and one valid port.
 * RU: Сформировать точный запрос графа с PID вызывающего и одним допустимым каналом. */
static int request(unsigned char *frame, size_t *size)
{
    const StyxRpcChannelRoute port = {.kind = STYXRPC_ROUTE_CURRENT, .channel = {TEST_KEY, "(i)"}};
    unsigned char pid[TEST_FRAME], channel[TEST_FRAME];
    size_t pid_size, channel_size;
    return styxrpc_value_integer(STYXRPC_VALUE_BIG, TEST_PID, pid, sizeof(pid), &pid_size) == STYXRPC_OK &&
        styxrpc_channel_route_value(&port, channel, sizeof(channel), &channel_size) == STYXRPC_OK &&
        fvm_srecord_write_pair(STYXRPC_GRAPH_REQUEST_TAG, pid, pid_size, channel, channel_size,
            frame, TEST_FRAME, size) == FVM_S_OK;
}


/* EN: Verify one canceled queued call, a separate detached active call and a completed call.
 * RU: Проверить отмену очереди, закрытие отдельного активного вызова и готовый ответ. */
static int exercise(Owner *owner, const StyxRpcSchema *schema)
{
    const StyxRpcVfs *ops = &styxrpc_channel_handlers_tree.files;
    unsigned char frame[TEST_FRAME], reply[TEST_FRAME], expected[TEST_FRAME];
    void *canceled = NULL, *active = NULL, *finished = NULL;
    unsigned char result[TEST_FRAME], updates[TEST_FRAME];
    StyxRpcChannelCall call;
    size_t size, used, expected_size, result_size, updates_size;
    int valid = request(frame, &size) && styxrpc_channel_call(schema, frame, size, &call) == STYXRPC_OK &&
        fvm_srecord_write(STYXRPC_VALUE_NIL, NULL, 0, result, sizeof(result), &result_size) == FVM_S_OK &&
        fvm_srecord_write(STYXRPC_GRAPH_UPDATES_TAG, NULL, 0, updates, sizeof(updates), &updates_size) == FVM_S_OK &&
        styxrpc_channel_call_reply(schema, &call, result, result_size, updates, updates_size,
            expected, sizeof(expected), &expected_size) == STYXRPC_OK &&
        stage(owner->module, 1, frame, size, &canceled);
    if (canceled)
    {
        ops->close(canceled);
    }
    valid = valid && stage(owner->module, 2, frame, size, &active) &&
        ops->read(active, 0, reply, sizeof(reply), &used) && !used &&
        stage(owner->module, 1, frame, size, &finished);
    pthread_t thread;
    int started = valid && !pthread_create(&thread, NULL, owner_loop, owner);
    if (started)
    {
        pthread_mutex_lock(&owner->lock);
        while (!owner->paused)
        {
            pthread_cond_wait(&owner->wake, &owner->lock);
        }
        valid = ops->read(active, 0, reply, sizeof(reply), &used) && !used;
        ops->close(active);
        active = NULL;
        owner->release = 1;
        pthread_cond_signal(&owner->wake);
        pthread_mutex_unlock(&owner->lock);

        /* EN: Wait until the remaining callback starts before stopping admission.
         * RU: Дождаться запуска оставшегося колбэка до остановки приёма. */
        pthread_mutex_lock(&owner->lock);
        while (owner->count < 2)
        {
            pthread_cond_wait(&owner->wake, &owner->lock);
        }
        pthread_mutex_unlock(&owner->lock);
    }
    styxrpc_channel_handlers_stop(owner->module);
    if (started)
    {
        pthread_join(thread, NULL);
        valid = valid && ops->read(finished, 0, reply, sizeof(reply), &used) &&
            used == expected_size && !memcmp(reply, expected, used) && owner->count == 2 && owner->released == 1;
    }
    if (active)
    {
        ops->close(active);
    }
    if (finished)
    {
        ops->close(finished);
    }
    return started && valid;
}


/* EN: Host and verify the generic C module with no FlashVM or DisVM dependencies.
 * RU: Запустить и проверить универсальный модуль C без зависимостей FlashVM и DisVM. */
int main(void)
{
    Owner owner = {.lock = PTHREAD_MUTEX_INITIALIZER, .wake = PTHREAD_COND_INITIALIZER};
    const StyxRpcChannelFunction function = {"accept", UINT32_C(0xe4cc716f), {"(c[i])", "()"}, handle, &owner};
    const StyxRpcChannelExport exported = {"relay", UINT32_C(0x9c30df25), {"(c[i])", "(c[i])"}, handle_result, &owner};
    StyxRpcModuleChannels registry;
    StyxRpcModuleChannel slot;
    if (styxrpc_module_channels_init(&registry, &slot, 1, unexpected_entropy, NULL))
    {
        owner.module = styxrpc_channel_handlers_create_with_results("incoming.dis", &function, 1, &exported, 1, &registry);
    }
    int ok = owner.module && exercise(&owner, &function.schema);
    styxrpc_channel_handlers_destroy(owner.module);
    pthread_cond_destroy(&owner.wake);
    pthread_mutex_destroy(&owner.lock);
    if (!ok)
    {
        fputs("channel handler dispatch or cancellation failed\n", stderr);
    }
    return ok ? 0 : 1;
}
