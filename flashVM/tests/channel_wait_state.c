/* Native blocking/try operations, observable wait directions and receipt acknowledgement. */
#include "flashvm/channel.h"
#include "flashvm/extensions.h"
#include "flashvm/debug.h"
#include "../ports/thread_port.h"
#include <stdio.h>
#include <string.h>

#define CHECK(test)                                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(test))                                                                                                   \
        {                                                                                                              \
            fprintf(stderr, "failed line %d: %s\n", __LINE__, #test);                                                  \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)


/* Main observes result only after join; channel lifetime extends through both threads. */
typedef struct Call
{
    FVM_Channel *channel;
    FVM_Result result;
    FVM_Value value;
    int sending;
    int borrowed;
    int script;
} Call;


/* Use actual native OS threads; no polling enters a VM or accesses its heap. */
static void invoke(void *user)
{
    Call *call = (Call *)user;
    if (call->script)
    {
        FVM_Config config;
        FVM_Runtime *runtime = 0;
        FVM_Context *context = 0;
        FVM_Object *wrapper = 0;
        call->result = fvm_config_init(&config);
        if (call->result == FVM_OK)
        {
            call->result = fvm_runtime_create(&config, &runtime);
        }
        if (call->result == FVM_OK)
        {
            call->result = fvm_context_create(runtime, &context);
        }
        if (call->result == FVM_OK)
        {
            call->result = fvm_extensions_wrap_channel(context, call->channel, &wrapper);
        }
        if (call->result == FVM_OK)
        {
            call->result = fvm_object_call(context, wrapper, "receive", 0, 0, &call->value);
        }
        if (context)
        {
            fvm_context_destroy(context);
        }
        if (runtime)
        {
            fvm_runtime_destroy(runtime);
        }
    }
    else if (call->sending)
    {
        call->result = fvm_channel_send(call->channel, &call->value);
    }
    else if (call->borrowed)
    {
        call->result = fvm_channel_receive(call->channel, &call->value);
    }
    else
    {
        call->result = fvm_channel_receive_owned(call->channel, &call->value, 1);
    }
}


/* Bounded observation avoids assuming scheduling speed or sleeping for a fixed race. */
static int wait_for(FVM_Channel *channel, unsigned int direction, size_t pending)
{
    FVM_ChannelWaitState state;
    unsigned int attempt;
    for (attempt = 0; attempt < 5000; attempt++)
    {
        if (fvm_channel_wait_state(channel, &state) != FVM_OK)
        {
            return 0;
        }
        if ((state.directions & direction) == direction && state.pending == pending)
        {
            return 1;
        }
        fvm_port_thread_sleep_us(1000);
    }
    return 0;
}


/* Check sender-first, receiver-first, closed offers and buffered backpressure. */
int main(void)
{
    FVM_Channel *channel = 0;
    FVM_PortThread *thread;
    FVM_PortThread *receiver_thread;
    FVM_ChannelWaitState state;
    FVM_Value value;
    Call call;
    Call receiver;
    int available;
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    FVM_Object *wrapper = 0;
    memset(&call, 0, sizeof(call));
    CHECK(fvm_channel_create(&channel) == FVM_OK);
    CHECK(fvm_channel_set_rendezvous(channel, 1) == FVM_OK);
    fvm_value_number(&value, 42);
    CHECK(fvm_channel_try_send(channel, &value) == FVM_NOT_FOUND);
    CHECK(fvm_channel_try_receive(channel, &value) == FVM_NOT_FOUND);
    call.channel = channel;
    call.sending = 1;
    fvm_value_number(&call.value, 42);
    thread = fvm_port_thread_start(invoke, &call);
    CHECK(thread && wait_for(channel, FVM_CHANNEL_WAIT_SEND, 1));
    CHECK(fvm_config_init(&config) == FVM_OK);
    CHECK(fvm_runtime_create(&config, &runtime) == FVM_OK);
    CHECK(fvm_context_create(runtime, &context) == FVM_OK);
    CHECK(fvm_extensions_wrap_channel(context, channel, &wrapper) == FVM_OK);
    CHECK(fvm_object_call(context, wrapper, "waitingSenders", 0, 0, &value) == FVM_OK && value.number == 1);
    CHECK(fvm_object_call(context, wrapper, "waitingReceivers", 0, 0, &value) == FVM_OK && value.number == 0);
    CHECK(fvm_object_call(context, wrapper, "waitDirections", 0, 0, &value) == FVM_OK &&
          value.number == FVM_CHANNEL_WAIT_SEND);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    CHECK(fvm_channel_available(channel, &available) == FVM_OK && available);
    CHECK(fvm_channel_set_rendezvous(channel, 0) == FVM_INVALID_ARGUMENT);
    CHECK(fvm_channel_receive_owned(channel, &value, 0) == FVM_OK && value.number == 42);
    fvm_channel_message_release(&value);
    fvm_port_thread_join(thread);
    CHECK(call.result == FVM_OK);
    CHECK(fvm_channel_wait_state(channel, &state) == FVM_OK && !state.directions && !state.pending);

    call.sending = 0;
    call.borrowed = 1;
    thread = fvm_port_thread_start(invoke, &call);
    CHECK(thread && wait_for(channel, FVM_CHANNEL_WAIT_RECEIVE, 0));
    fvm_value_number(&value, 99);
    CHECK(fvm_channel_try_send(channel, &value) == FVM_OK);
    fvm_port_thread_join(thread);
    CHECK(call.result == FVM_OK && call.value.number == 99);

    call.script = 1;
    thread = fvm_port_thread_start(invoke, &call);
    CHECK(thread && wait_for(channel, FVM_CHANNEL_WAIT_RECEIVE, 0));
    CHECK(fvm_channel_try_send(channel, &value) == FVM_OK);
    fvm_port_thread_join(thread);
    CHECK(call.result == FVM_OK && call.value.number == 99);
    CHECK(fvm_channel_wait_state(channel, &state) == FVM_OK && !state.directions);
    call.script = 0;

    CHECK(fvm_debug_channel_freeze(channel, 1) == FVM_OK);
    call.sending = 1;
    thread = fvm_port_thread_start(invoke, &call);
    CHECK(thread && wait_for(channel, FVM_CHANNEL_WAIT_SEND, 1));
    receiver = call;
    receiver.sending = 0;
    receiver.borrowed = 0;
    receiver_thread = fvm_port_thread_start(invoke, &receiver);
    CHECK(receiver_thread && wait_for(channel, FVM_CHANNEL_WAIT_SEND | FVM_CHANNEL_WAIT_RECEIVE, 1));
    CHECK(fvm_debug_channel_freeze(channel, 0) == FVM_OK);
    fvm_port_thread_join(thread);
    fvm_port_thread_join(receiver_thread);
    CHECK(call.result == FVM_OK && receiver.result == FVM_OK);

    call.sending = 1;
    fvm_value_number(&call.value, 7);
    thread = fvm_port_thread_start(invoke, &call);
    CHECK(thread && wait_for(channel, FVM_CHANNEL_WAIT_SEND, 1));
    fvm_channel_close(channel);
    fvm_port_thread_join(thread);
    CHECK(call.result == FVM_ERROR);
    CHECK(fvm_channel_wait_state(channel, &state) == FVM_OK && state.closed && !state.pending && !state.directions);
    fvm_channel_destroy(channel);

    CHECK(fvm_channel_create(&channel) == FVM_OK);
    CHECK(fvm_channel_set_capacity(channel, 1) == FVM_OK);
    CHECK(fvm_channel_send(channel, &value) == FVM_OK);
    call.channel = channel;
    call.sending = 1;
    thread = fvm_port_thread_start(invoke, &call);
    CHECK(thread && wait_for(channel, FVM_CHANNEL_WAIT_SEND, 1));
    CHECK(fvm_channel_receive_owned(channel, &value, 0) == FVM_OK);
    fvm_channel_message_release(&value);
    fvm_port_thread_join(thread);
    CHECK(call.result == FVM_OK);
    CHECK(fvm_channel_receive_owned(channel, &value, 0) == FVM_OK);
    fvm_channel_message_release(&value);
    call.sending = 0;
    call.borrowed = 0;
    thread = fvm_port_thread_start(invoke, &call);
    CHECK(thread && wait_for(channel, FVM_CHANNEL_WAIT_RECEIVE, 0));
    fvm_channel_close(channel);
    fvm_port_thread_join(thread);
    CHECK(call.result == FVM_ERROR);
    CHECK(fvm_channel_wait_state(channel, &state) == FVM_OK && !state.directions);
    fvm_channel_destroy(channel);
    puts("PASS native blocking channels, wait directions, rendezvous and close cleanup");
    return 0;
}
