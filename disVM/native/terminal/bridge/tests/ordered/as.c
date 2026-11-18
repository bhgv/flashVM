/* Copyright (c) 2026 bhgv
 * EN: Real FlashVM AS wrappers over negotiated terminal; copied TX survives context destruction.
 * RU: Настоящие FlashVM AS-wrappers через согласованный терминал; TX-копия переживает удаление context.
 */
#define _POSIX_C_SOURCE 200809L
#include "bridge/ordered.h"
#include "flashvm/extensions.h"
#include "flashvm/function.h"
#include "thread_port.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER; /* Test-only ownership handoff, not a production wait. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Retained TX entered before context destruction. */
static int claimed; /* Native relay owns its independent frame copy. */
static int released; /* FlashVM owner and original wrapper heap have retired. */


/* EN/RU: Fatal explicit ownership/codec test contract, active under optimization.
 * Явная обязательная проверка владения/кодека, активная при оптимизации.
 */
static void check(int valid)
{
    if (!valid)
    {
        fprintf(stderr, "ordered terminal AS ownership mismatch\n");
        exit(2);
    }
}


/* EN/RU: Original per-client native admission, retains no VM pointers.
 * Исходный native-приём клиента, без удержания VM-указателей.
 */
extern int __real_styxorderedclientsendwait(StyxOrderedClient *, const void *, size_t, unsigned);


/* EN/RU: Hold copied relay frame until sender context is destroyed, then invoke production admission once.
 * Удерживать копию relay до удаления context отправителя, затем однократно вызвать production-приём.
 */
int __wrap_styxorderedclientsendwait(StyxOrderedClient *client, const void *bytes, size_t size, unsigned timeout)
{
    pthread_mutex_lock(&gate);
    claimed = 1;
    pthread_cond_broadcast(&changed);
    while (!released)
    {
        pthread_cond_wait(&changed, &gate);
    }
    pthread_mutex_unlock(&gate);
    return __real_styxorderedclientsendwait(client, bytes, size, timeout);
}


/* EN/RU: Test-only readiness snapshot of actual incoming FlashVM channel; no managed receive consumed here.
 * Только test-наблюдение настоящего входящего FlashVM-канала; managed-получение здесь не потребляется.
 */
static void queued(FVM_Channel *incoming)
{
    for (unsigned i = 0; i < 5000; i++)
    {
        FVM_ChannelWaitState state; /* Mutex-protected actual local channel state. */
        check(fvm_channel_wait_state(incoming, &state) == FVM_OK);
        if (state.pending)
        {
            return;
        }
        fvm_port_thread_sleep_us(1000);
    }
    check(0);
}


/* EN/RU: Actual wrapper calls stay on one FlashVM owner; native broker/relay receives copied strings only.
 * Настоящие wrapper-вызовы выполняются на одном FlashVM-owner; native-брокер/relay получает лишь копии строк.
 */
void orderedterminalas(DVM_UIBridge *local)
{
    FVM_Config config; /* Owner-local configuration. */
    FVM_Runtime *runtime = NULL; /* Owned actual FlashVM runtime. */
    FVM_Context *context = NULL; /* Sole-owner heap, not accessed by native relays. */
    FVM_Channel *outgoing; /* Borrowed actual UI producer queue. */
    FVM_Channel *incoming; /* Borrowed actual UI consumer queue. */
    FVM_Object *sender; /* [GC:] Rooted before further heap allocations. */
    FVM_Object *receiver; /* [GC:] Rooted through context global until destroy. */
    FVM_Value value; /* [GC:] Owner-only scalar/object reference. */
    FVM_Value method; /* [GC:] Owner-only wrapper method. */
    FVM_Value result; /* [GC:] Owner-only invocation result. */
    DVM_UIMessage message; /* Copied portable frame values. */
    char bytes[DVM_UI_HEADER_SIZE + DVM_UI_TEXT_MAX]; /* Private canonical frame. */
    check(dvm_ui_channels(local, DVM_UI_UI, &outgoing, &incoming) == FVM_OK);
    check(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK &&
        fvm_context_create(runtime, &context) == FVM_OK && fvm_extensions_install(context) == FVM_OK &&
        fvm_extensions_wrap_channel(context, outgoing, &sender) == FVM_OK);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = sender;
    check(fvm_set_global(context, "TerminalTx", &value) == FVM_OK &&
        fvm_extensions_wrap_channel(context, incoming, &receiver) == FVM_OK);
    value.object = receiver;
    check(fvm_set_global(context, "TerminalRx", &value) == FVM_OK);
    queued(incoming);
    check(fvm_object_get(context, receiver, "tryReceive", &method) == FVM_OK &&
        fvm_function_invoke(context, method.object, receiver, 0, NULL, &result) == FVM_OK &&
        dvm_ui_decode(&result, &message) == FVM_OK && message.id == 42 && message.size == 4 &&
        !memcmp(message.text, "A\0\320\257", 4));
    message.kind = DVM_UI_REPLY;
    message.id = 43;
    message.first = 0;
    message.size = 5;
    memcpy(message.text, "reply", 5);
    check(dvm_ui_encode(&message, bytes, sizeof(bytes), &value) == FVM_OK &&
        fvm_object_get(context, sender, "trySend", &method) == FVM_OK &&
        fvm_function_invoke(context, method.object, sender, 1, &value, &result) == FVM_OK &&
        result.type == FVM_VALUE_BOOLEAN && result.boolean);
    struct timespec deadline; /* Five-second fixture-only TX handoff budget. */
    check(!clock_gettime(CLOCK_REALTIME, &deadline));
    deadline.tv_sec += 5;
    pthread_mutex_lock(&gate);
    while (!claimed)
    {
        check(!pthread_cond_timedwait(&changed, &gate, &deadline));
    }
    pthread_mutex_unlock(&gate);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    pthread_mutex_lock(&gate);
    released = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&gate);
}
