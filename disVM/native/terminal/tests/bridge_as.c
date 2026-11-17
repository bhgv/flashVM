/* Copyright (c) 2026 bhgv
 * EN: Actual FlashVM AS channel wrappers connected to original 9P through native relays.
 * RU: Настоящие AS-channel wrappers FlashVM через native-мост и исходный 9P.
 */
#include "bridge/bridge.h"
#include "flashvm/extensions.h"
#include "flashvm/function.h"
#include "thread_port.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER; /* Test-only native TX lifetime barrier. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Test-only barrier readiness. */
static int claimed; /* TX relay owns a copied frame, under gate. */
static int released; /* FlashVM context has been destroyed, under gate. */


/* EN/RU: The original route sender, reached only after the test lifetime barrier releases.
 * Настоящий sender route, вызывается после открытия тестового барьера.
 */
extern int __real_styxbidirrouteuisend(StyxBidirRoute *, unsigned long, const DVM_UIMessage *);


/* EN: Hold a real native TX copy before remote acceptance until its original context retires.
 * Borrow route/tag/message throughout the barrier, then call the actual sender unchanged.
 * RU: Удержать native TX-копию до уничтожения исходного context, затем вызвать настоящий sender.
 */
int __wrap_styxbidirrouteuisend(StyxBidirRoute *route, unsigned long tag, const DVM_UIMessage *message)
{
    pthread_mutex_lock(&gate);
    claimed = 1;
    pthread_cond_broadcast(&changed);
    while (!released)
    {
        pthread_cond_wait(&changed, &gate);
    }
    pthread_mutex_unlock(&gate);
    return __real_styxbidirrouteuisend(route, tag, message);
}


/* EN/RU: Fail explicitly on any actual host/wrapper/transport contract failure.
 * Явно завершить проверку при нарушении контракта host/wrapper/transport.
 */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "terminal AS bridge: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Observe queued input under the actual mutex, at most five seconds of native test time.
 * Borrow channel; this observer is not a production or guest polling mechanism.
 * Наблюдать очередь под настоящим mutex до пяти секунд, только в native-тесте.
 */
static void queued(FVM_Channel *channel)
{
    FVM_ChannelWaitState state; /* Actual mutex-protected queue readiness snapshot. */
    for (unsigned i = 0; i < 5000; i++)
    {
        check(fvm_channel_wait_state(channel, &state) == FVM_OK, "actual channel observation");
        if (state.pending)
        {
            return;
        }
        fvm_port_thread_sleep_us(1000);
    }
    check(0, "native relay did not deliver an AS input frame");
}


/* EN: Borrow local/route through return; create a FlashVM context only on this native owner.
 * Invoke actual wrapper methods, then destroy their context before the remote delivery receipt.
 * This proves copied queue ownership, not execution of a SWF program or its presentation logic.
 * RU: Заимствовать local/route; создать FlashVM context только на текущем native owner.
 * Вызвать настоящие методы wrappers, уничтожить context до удалённой квитанции. Это проверка
 * копирования и владения, а не выполнения SWF или его логики отображения.
 */
void terminalbridgeas(DVM_UIBridge *local, StyxBidirRoute *route)
{
    FVM_Config config; /* Owner-local runtime configuration. */
    FVM_Runtime *runtime = NULL; /* Owned FlashVM runtime, retired before peer receipt. */
    FVM_Context *context = NULL; /* Sole-owner heap, never accessed by relay workers. */
    FVM_Channel *outgoing; /* Borrowed UI producer queue. */
    FVM_Channel *incoming; /* Borrowed UI receive queue. */
    FVM_Object *sender; /* [GC:] Rooted through the context's global object. */
    FVM_Object *receiver; /* [GC:] Rooted through the context's global object. */
    FVM_Value value; /* Copied scalar/object reference used only on this owner. */
    FVM_Value method; /* Owner-local actual channel method. */
    FVM_Value result; /* Owner-local native invocation result. */
    DVM_UIMessage message; /* Copied fvd1 values, independent of the context heap. */
    char storage[DVM_UI_HEADER_SIZE + DVM_UI_TEXT_MAX]; /* Counted outgoing frame, copied on send. */
    check(dvm_ui_channels(local, DVM_UI_UI, &outgoing, &incoming) == FVM_OK, "actual UI endpoints");
    check(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK &&
        fvm_context_create(runtime, &context) == FVM_OK && fvm_extensions_install(context) == FVM_OK &&
        fvm_extensions_wrap_channel(context, outgoing, &sender) == FVM_OK, "actual AS sender wrapper");
    /* [GC:] Root sender before allocating receiver; native relays retain no object addresses. */
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = sender;
    check(fvm_set_global(context, "BridgeTx", &value) == FVM_OK &&
        fvm_extensions_wrap_channel(context, incoming, &receiver) == FVM_OK, "root sender before allocation");
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = receiver;
    check(fvm_set_global(context, "BridgeRx", &value) == FVM_OK, "root receiver");
    queued(incoming);
    check(fvm_object_get(context, receiver, "tryReceive", &method) == FVM_OK &&
        fvm_function_invoke(context, method.object, receiver, 0, NULL, &result) == FVM_OK &&
        dvm_ui_decode(&result, &message) == FVM_OK && message.id == 42 && message.size == 4 &&
        !memcmp(message.text, "A\0\320\257", 4), "real 9P payload arrives through AS wrapper");
    unsigned char declaration[] = {0x81, 0x80}; /* Independent outgoing fixture control. */
    unsigned char payload[240]; /* Required binary record before the UI reply. */
    for (unsigned i = 0; i < sizeof(payload); i++)
    {
        payload[i] = (unsigned char)i;
    }
    check(styxbidirroutesend(route, 0, declaration, sizeof(declaration)) &&
        styxbidirrouteflush(route, 5000), "independent control output");
    check(styxbidirroutesend(route, 101, payload, sizeof(payload)) &&
        styxbidirrouteflush(route, 5000), "independent binary output");
    message.kind = DVM_UI_REPLY;
    message.id = 43;
    message.first = 0;
    message.second = 0;
    message.size = 5;
    memcpy(message.text, "reply", 5);
    check(dvm_ui_encode(&message, storage, sizeof(storage), &value) == FVM_OK &&
        fvm_object_get(context, sender, "trySend", &method) == FVM_OK &&
        fvm_function_invoke(context, method.object, sender, 1, &value, &result) == FVM_OK &&
        result.type == FVM_VALUE_BOOLEAN && result.boolean, "AS wrapper copies the outgoing frame");
    /* [GC:] Retire the original owner while the independent copied TX frame is still held. */
    struct timespec deadline; /* Five-second test-only realtime condition deadline. */
    check(!clock_gettime(CLOCK_REALTIME, &deadline), "native lifetime barrier clock");
    deadline.tv_sec += 5;
    pthread_mutex_lock(&gate);
    while (!claimed)
    {
        check(!pthread_cond_timedwait(&changed, &gate, &deadline) || claimed, "TX copy reached lifetime barrier");
    }
    pthread_mutex_unlock(&gate);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    pthread_mutex_lock(&gate);
    released = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&gate);
    unsigned char bytes[32]; /* Server receipt, observed after the original context is gone. */
    unsigned long tag; /* Independent general route. */
    size_t size; /* Receipt payload bytes. */
    check(styxbidirroutewait(route, 0, 10000) &&
        styxbidirroutenext(route, &tag, bytes, sizeof(bytes), &size) == STYX_CONTROL_RECORD &&
        tag == 9 && size == 4 && !memcmp(bytes, "done", 4), "copied frame survives wrapper/context retirement");
}
