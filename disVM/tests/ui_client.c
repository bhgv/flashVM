/* Exercise bounded local acceptance, ring wrap, partial flush, input snapshots and closure. */
#include "disvm/ui_client.h"
#include <stdio.h>
#include <string.h>


/* Run real FlashVM queue and input-state cases without a service simulator.
 * No parameters. Returns zero on success or one with a concrete error diagnostic. */
int main(void)
{
    DVM_UIBridge *bridge;
    DVM_UIClient *client;
    DVM_UIMessage message;
    DVM_UIMessage received;
    FVM_Config config;
    FVM_Runtime *runtime;
    FVM_Context *context;
    unsigned int index;
    size_t sent;
    size_t pending;
    if (dvm_ui_create(0, &bridge) != FVM_OK || dvm_ui_client_create(bridge, &client) != FVM_OK)
    {
        fprintf(stderr, "UI client initialization failed\n");
        return 1;
    }
    memset(&message, 0, sizeof(message));
    message.kind = DVM_UI_REQUEST;
    message.size = 1;
    message.text[0] = 'A';
    for (index = 1; index <= DVM_UI_CLIENT_CAPACITY; index++)
    {
        message.id = index;
        if (dvm_ui_client_post(client, &message) != FVM_OK)
        {
            fprintf(stderr, "available local FIFO slot rejected\n");
            return 1;
        }
    }
    message.id = 65;
    message.text[0] = 'B';
    if (dvm_ui_client_post(client, &message) != FVM_NOT_FOUND ||
        dvm_ui_client_flush(client, DVM_UI_CLIENT_CAPACITY, &sent) != FVM_OK || sent != 32 ||
        dvm_ui_client_peek(client, &received, &pending) != FVM_OK || pending != 32 || received.id != 33)
    {
        fprintf(stderr, "full/partial client acceptance or retained head incorrect\n");
        return 1;
    }
    for (index = 1; index <= 32; index++)
    {
        if (dvm_ui_receive(bridge, DVM_UI_DIS, &received, 0) != FVM_OK || received.id != index ||
            received.text[0] != 'A')
        {
            fprintf(stderr, "client FIFO lost order or borrowed mutable source bytes\n");
            return 1;
        }
    }
    if (dvm_ui_client_post(client, &message) != FVM_OK ||
        dvm_ui_client_flush(client, DVM_UI_CLIENT_CAPACITY, &sent) != FVM_OK || sent != 32)
    {
        fprintf(stderr, "wrapped local FIFO could not accept/flush\n");
        return 1;
    }
    for (index = 33; index <= 64; index++)
    {
        if (dvm_ui_receive(bridge, DVM_UI_DIS, &received, 0) != FVM_OK || received.id != index)
        {
            fprintf(stderr, "wrapped FIFO reordered an earlier accepted request\n");
            return 1;
        }
    }
    if (dvm_ui_client_flush(client, 1, &sent) != FVM_OK || sent != 1 ||
        dvm_ui_receive(bridge, DVM_UI_DIS, &received, 0) != FVM_OK || received.id != 65 || received.text[0] != 'B' ||
        dvm_ui_client_flush(client, 1, &sent) != FVM_NOT_FOUND || sent != 0)
    {
        fprintf(stderr, "wrapped tail or idle result incorrect\n");
        return 1;
    }
    if (fvm_config_init(&config) != FVM_OK || fvm_runtime_create(&config, &runtime) != FVM_OK ||
        fvm_context_create(runtime, &context) != FVM_OK || fvm_input_set_pointer(context, 1.25, -2.5) != FVM_OK ||
        dvm_ui_capture_pointer(context, FVM_MOUSE_LEFT, FVM_INPUT_BEGIN, &message) != FVM_OK ||
        dvm_ui_client_post(client, &message) != FVM_OK || fvm_input_set_pointer(context, 99, 99) != FVM_OK ||
        dvm_ui_client_flush(client, 1, &sent) != FVM_OK || dvm_ui_receive(bridge, DVM_UI_DIS, &received, 0) != FVM_OK ||
        received.kind != DVM_UI_POINTER || received.first != 25 || received.second != -50)
    {
        fprintf(stderr, "queued input was recaptured from changed FlashVM state\n");
        return 1;
    }
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    message.kind = DVM_UI_REQUEST;
    message.id = 66;
    if (dvm_ui_client_post(client, &message) != FVM_OK)
    {
        fprintf(stderr, "close recovery record not accepted\n");
        return 1;
    }
    dvm_ui_close(bridge);
    if (dvm_ui_client_flush(client, 1, &sent) != FVM_ERROR || sent != 0 ||
        dvm_ui_client_peek(client, &received, &pending) != FVM_OK || received.id != 66 || pending != 1 ||
        dvm_ui_client_post(client, &message) != FVM_ERROR)
    {
        fprintf(stderr, "close discarded pending input or admitted later requests\n");
        return 1;
    }
    dvm_ui_client_destroy(client);
    dvm_ui_destroy(bridge);
    puts("PASS client ring wrap, copied FIFO, partial flush, exact FlashVM input and close recovery");
    return 0;
}
