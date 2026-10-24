/* Real FlashVM channel, AS wrapper and input-state integration; no channel test doubles. */
#include "disvm/ui_bridge.h"
#include "disvm/flashvm_host.h"
#include "flashvm/extensions.h"
#include "flashvm/function.h"
#include <stdio.h>
#include <string.h>


/* Worker state is owned by main until join; result is read only after join. */
typedef struct Worker
{
    DVM_UIBridge *bridge; /* Shared queue pair, alive through thread join. */
    FVM_Result result;    /* Last transport result, published by thread completion. */
    int reply;            /* Whether to echo an admitted request as a reply. */
} Worker;


/* Receive on a real OS worker, then optionally reply through the opposite queue.
 * opaque borrows Worker until return; returns nothing. No FlashVM context crosses threads. */
static void service(void *opaque)
{
    Worker *worker = (Worker *)opaque;
    DVM_UIMessage message;
    worker->result = dvm_ui_receive(worker->bridge, DVM_UI_DIS, &message, 1);
    if (worker->result == FVM_OK && worker->reply)
    {
        message.kind = DVM_UI_REPLY;
        worker->result = dvm_ui_send(worker->bridge, DVM_UI_DIS, &message, 1);
    }
}


/* Wait for an actual receiver registration, with a bounded failure path.
 * channel is borrowed. Returns one when waiting, zero after roughly two seconds/errors. */
static int await_receiver(FVM_Channel *channel)
{
    unsigned int attempt;
    FVM_ChannelWaitState state;
    for (attempt = 0; attempt < 2000; attempt++)
    {
        if (fvm_channel_wait_state(channel, &state) != FVM_OK)
        {
            return 0;
        }
        if (state.waiting_receivers)
        {
            return 1;
        }
        fvm_port_thread_sleep_us(1000);
    }
    return 0;
}


/* Exercise codec atomicity, AS interoperability, actual input and blocking lifecycle.
 * No parameters. Returns zero on success, one with a named diagnostic on failure. */
int main(void)
{
    DVM_UIBridge *bridge = 0;
    DVM_UIMessage message;
    DVM_UIMessage received;
    DVM_UIMessage before;
    DVM_UIMessage captured;
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    FVM_Channel *outgoing;
    FVM_Channel *incoming;
    FVM_Object *sender;
    FVM_Object *receiver;
    FVM_Value method;
    FVM_Value value;
    FVM_Value result;
    FVM_InputEvent input;
    FVM_KeyboardState keyboard;
    char storage[DVM_UI_HEADER_SIZE + DVM_UI_TEXT_MAX];
    unsigned int index;
    Worker worker;
    FVM_PortThread *thread;


    memset(&message, 0, sizeof(message));
    message.kind = DVM_UI_REQUEST;
    message.id = 17;
    message.code = 2;
    message.first = -2147483647L - 1L;
    message.second = 2147483647L;
    message.size = 5;
    memcpy(message.text, "A\0\xd0\xafZ", 5);
    if (dvm_ui_encode(&message, storage, sizeof(storage), &value) != FVM_OK ||
        dvm_ui_decode(&value, &received) != FVM_OK || received.first != message.first ||
        received.second != message.second || received.size != 5 || memcmp(received.text, message.text, 5))
    {
        fprintf(stderr, "codec lost signed bounds, Unicode or embedded NUL\n");
        return 1;
    }
    before = received;
    storage[6] = 'g';
    if (dvm_ui_decode(&value, &received) != FVM_INVALID_ARGUMENT || memcmp(&before, &received, sizeof(received)))
    {
        fprintf(stderr, "malformed header changed output\n");
        return 1;
    }
    message.id = 0;
    if (dvm_ui_encode(&message, storage, sizeof(storage), &value) != FVM_INVALID_ARGUMENT)
    {
        fprintf(stderr, "uncorrelated request admitted\n");
        return 1;
    }
    message.id = 17;
    message.size = 3;
    memcpy(message.text, "\xed\xa0\x80", 3);
    if (dvm_ui_encode(&message, storage, sizeof(storage), &value) != FVM_INVALID_ARGUMENT)
    {
        fprintf(stderr, "isolated surrogate admitted into the shared Unicode profile\n");
        return 1;
    }
    message.size = 5;
    memcpy(message.text, "A\0\xd0\xafZ", 5);
    if (dvm_ui_create(0, &bridge) != FVM_OK || dvm_ui_channels(bridge, DVM_UI_UI, &outgoing, &incoming) != FVM_OK ||
        outgoing == incoming || dvm_ui_receive(bridge, DVM_UI_DIS, &received, 0) != FVM_NOT_FOUND)
    {
        fprintf(stderr, "directional queue construction/empty observation failed\n");
        return 1;
    }
    if (fvm_config_init(&config) != FVM_OK || fvm_runtime_create(&config, &runtime) != FVM_OK ||
        fvm_context_create(runtime, &context) != FVM_OK || fvm_extensions_install(context) != FVM_OK ||
        fvm_extensions_wrap_channel(context, outgoing, &sender) != FVM_OK)
    {
        fprintf(stderr, "real FlashVM/AS sender initialization failed\n");
        return 1;
    }
    /* [GC:] Publish the first wrapper before allocations can collect it. */
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = sender;
    if (fvm_set_global(context, "BridgeTx", &value) != FVM_OK ||
        fvm_extensions_wrap_channel(context, incoming, &receiver) != FVM_OK)
    {
        fprintf(stderr, "AS sender rooting/receiver initialization failed\n");
        return 1;
    }
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = receiver;
    if (fvm_set_global(context, "BridgeRx", &value) != FVM_OK ||
        dvm_ui_encode(&message, storage, sizeof(storage), &value) != FVM_OK ||
        fvm_object_get(context, sender, "trySend", &method) != FVM_OK ||
        fvm_function_invoke(context, method.object, sender, 1, &value, &result) != FVM_OK ||
        result.type != FVM_VALUE_BOOLEAN || !result.boolean ||
        dvm_ui_receive(bridge, DVM_UI_DIS, &received, 0) != FVM_OK || received.id != 17)
    {
        fprintf(stderr, "AS wrapper to native adapter transport failed\n");
        return 1;
    }
    received.kind = DVM_UI_REPLY;
    if (dvm_ui_send(bridge, DVM_UI_DIS, &received, 0) != FVM_OK ||
        fvm_object_get(context, receiver, "tryReceive", &method) != FVM_OK ||
        fvm_function_invoke(context, method.object, receiver, 0, 0, &result) != FVM_OK ||
        dvm_ui_decode(&result, &received) != FVM_OK || received.kind != DVM_UI_REPLY)
    {
        fprintf(stderr, "native adapter to AS wrapper transport failed\n");
        return 1;
    }
    memset(&input, 0, sizeof(input));
    input.gesture = FVM_GESTURE_TAP;
    input.fingers = 1;
    input.x = -12.25;
    input.y = 40.5;
    if (fvm_input_dispatch(context, &input, FVM_INPUT_BEGIN) != FVM_OK ||
        dvm_ui_pointer(bridge, context, FVM_MOUSE_LEFT, FVM_INPUT_BEGIN, &captured) != FVM_OK ||
        dvm_ui_receive(bridge, DVM_UI_DIS, &received, 0) != FVM_OK || received.kind != DVM_UI_POINTER ||
        received.first != -245 || received.second != 810)
    {
        fprintf(stderr, "FlashVM pointer dispatch/capture failed\n");
        return 1;
    }
    memset(&keyboard, 0, sizeof(keyboard));
    keyboard.struct_size = sizeof(keyboard);
    keyboard.last_code = 65;
    keyboard.last_ascii = 65;
    keyboard.down[65] = 1;
    if (fvm_input_dispatch_keyboard(context, &keyboard, 1) != FVM_OK ||
        dvm_ui_keyboard(bridge, context, 65, 1, &captured) != FVM_OK ||
        dvm_ui_receive(bridge, DVM_UI_DIS, &received, 0) != FVM_OK || received.kind != DVM_UI_KEY ||
        received.code != 65 || received.first != 65 ||
        dvm_ui_keyboard(bridge, context, 65, 0, &captured) != FVM_INVALID_ARGUMENT)
    {
        fprintf(stderr, "FlashVM keyboard dispatch/capture validation failed\n");
        return 1;
    }
    fvm_value_number(&value, 1);
    if (fvm_channel_send(outgoing, &value) != FVM_OK ||
        dvm_ui_receive(bridge, DVM_UI_DIS, &received, 0) != FVM_INVALID_ARGUMENT ||
        dvm_ui_send(bridge, DVM_UI_UI, &message, 0) != FVM_OK ||
        dvm_ui_receive(bridge, DVM_UI_DIS, &received, 0) != FVM_OK)
    {
        fprintf(stderr, "invalid envelope poisoned subsequent delivery\n");
        return 1;
    }
    for (index = 0; index < DVM_UI_PENDING_MAX; index++)
    {
        message.id = index + 1;
        if (dvm_ui_send(bridge, DVM_UI_UI, &message, 0) != FVM_OK)
        {
            fprintf(stderr, "bounded queue rejected an available slot\n");
            return 1;
        }
    }
    if (dvm_ui_send(bridge, DVM_UI_UI, &message, 0) != FVM_NOT_FOUND)
    {
        fprintf(stderr, "full queue failed to apply backpressure\n");
        return 1;
    }
    if (dvm_ui_pointer(bridge, context, FVM_MOUSE_LEFT, FVM_INPUT_END, &captured) != FVM_NOT_FOUND ||
        captured.kind != DVM_UI_POINTER || captured.first != -245 || captured.second != 810)
    {
        fprintf(stderr, "backpressure failed to preserve the exact input sample\n");
        return 1;
    }
    dvm_ui_close(bridge);
    for (index = 0; index < DVM_UI_PENDING_MAX; index++)
    {
        if (dvm_ui_receive(bridge, DVM_UI_DIS, &received, 0) != FVM_OK || received.id != index + 1)
        {
            fprintf(stderr, "close lost or reordered buffered requests\n");
            return 1;
        }
    }
    if (dvm_ui_receive(bridge, DVM_UI_DIS, &received, 0) != FVM_ERROR)
    {
        fprintf(stderr, "closed empty queue did not terminate reception\n");
        return 1;
    }
    /* [GC:] Wrappers release their retained queue handles before bridge destruction. */
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    dvm_ui_destroy(bridge);

    if (dvm_ui_create(1, &bridge) != FVM_OK || dvm_ui_channels(bridge, DVM_UI_UI, &outgoing, &incoming) != FVM_OK ||
        dvm_ui_send(bridge, DVM_UI_UI, &message, 0) != FVM_NOT_FOUND)
    {
        fprintf(stderr, "rendezvous send succeeded without receiver\n");
        return 1;
    }
    worker.bridge = bridge;
    worker.result = FVM_ERROR;
    worker.reply = 1;
    thread = dvm_host_thread_start(service, &worker);
    if (!thread)
    {
        fprintf(stderr, "native service thread unavailable\n");
        return 1;
    }
    if (!await_receiver(outgoing) || dvm_ui_send(bridge, DVM_UI_UI, &message, 0) != FVM_OK ||
        dvm_ui_receive(bridge, DVM_UI_UI, &received, 1) != FVM_OK || received.id != message.id ||
        received.kind != DVM_UI_REPLY)
    {
        dvm_ui_close(bridge);
        dvm_host_thread_join(thread);
        fprintf(stderr, "native rendezvous request/reply failed\n");
        return 1;
    }
    dvm_host_thread_join(thread);
    if (worker.result != FVM_OK)
    {
        fprintf(stderr, "service reply failed\n");
        return 1;
    }
    worker.reply = 0;
    thread = dvm_host_thread_start(service, &worker);
    if (!thread)
    {
        fprintf(stderr, "close test thread unavailable\n");
        return 1;
    }
    if (!await_receiver(outgoing))
    {
        dvm_ui_close(bridge);
        dvm_host_thread_join(thread);
        fprintf(stderr, "close test receiver did not register\n");
        return 1;
    }
    dvm_ui_close(bridge);
    dvm_host_thread_join(thread);
    if (worker.result != FVM_ERROR)
    {
        fprintf(stderr, "close failed to wake blocked receiver\n");
        return 1;
    }
    dvm_ui_destroy(bridge);
    puts("PASS codec, AS wrappers, FlashVM input, native rendezvous and close wakeup");
    return 0;
}
