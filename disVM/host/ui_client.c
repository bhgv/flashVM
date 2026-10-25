/* Preserve exact input samples across multiple frames of transport backpressure. */
#include "disvm/ui_client.h"
#include <stdlib.h>


/* The UI owner alone accesses this native bounded ring. */
struct DVM_UIClient
{
    DVM_UIBridge *bridge;                           /* Borrowed queue pair, alive until destruction. */
    DVM_UIMessage messages[DVM_UI_CLIENT_CAPACITY]; /* Copied records in FIFO order. */
    size_t head;                                    /* Oldest pending record index. */
    size_t count;                                   /* Number of accepted local records. */
    FVM_Result failure;                             /* First terminal send error; never silently retried. */
};


/* Allocate the local FIFO without touching the bridge.
 * bridge/out and return ownership follow ui_client.h. */
FVM_Result dvm_ui_client_create(DVM_UIBridge *bridge, DVM_UIClient **out)
{
    DVM_UIClient *client;
    if (!bridge || !out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    client = (DVM_UIClient *)calloc(1, sizeof(*client));
    if (!client)
    {
        return FVM_OUT_OF_MEMORY;
    }
    client->bridge = bridge;
    client->failure = FVM_OK;
    *out = client;
    return FVM_OK;
}


/* Validate before acceptance; reject a full ring without changing any earlier record.
 * client/message and local-versus-service acceptance semantics follow ui_client.h. */
FVM_Result dvm_ui_client_post(DVM_UIClient *client, const DVM_UIMessage *message)
{
    FVM_Value value;
    FVM_Result result;
    char storage[DVM_UI_HEADER_SIZE + DVM_UI_TEXT_MAX];
    size_t tail;
    if (!client)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (client->failure != FVM_OK)
    {
        return client->failure;
    }
    result = dvm_ui_encode(message, storage, sizeof(storage), &value);
    if (result != FVM_OK)
    {
        return result;
    }
    if (client->count == DVM_UI_CLIENT_CAPACITY)
    {
        return FVM_NOT_FOUND;
    }
    tail = (client->head + client->count) % DVM_UI_CLIENT_CAPACITY;
    client->messages[tail] = *message;
    client->count++;
    return FVM_OK;
}


/* Try oldest-first transport sends, retaining the unsent head on pressure/error.
 * client/budget/sent and partial-progress guarantees follow ui_client.h. */
FVM_Result dvm_ui_client_flush(DVM_UIClient *client, unsigned int budget, size_t *sent)
{
    size_t completed = 0;
    FVM_Result result = FVM_NOT_FOUND;
    if (!client || !sent || !budget || budget > DVM_UI_CLIENT_CAPACITY)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *sent = 0;
    if (client->failure != FVM_OK)
    {
        return client->failure;
    }
    while (client->count && completed < budget)
    {
        result = dvm_ui_send(client->bridge, DVM_UI_UI, &client->messages[client->head], 0);
        if (result != FVM_OK)
        {
            if (result != FVM_NOT_FOUND)
            {
                client->failure = result;
            }
            break;
        }
        client->head = (client->head + 1) % DVM_UI_CLIENT_CAPACITY;
        client->count--;
        completed++;
    }
    *sent = completed;
    if (result != FVM_OK && result != FVM_NOT_FOUND)
    {
        return result;
    }
    return completed ? FVM_OK : FVM_NOT_FOUND;
}


/* Copy the oldest pending value without removing it.
 * client/out/pending and empty behavior follow ui_client.h. */
FVM_Result dvm_ui_client_peek(const DVM_UIClient *client, DVM_UIMessage *out, size_t *pending)
{
    if (!client || !out || !pending)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *pending = client->count;
    if (!client->count)
    {
        return FVM_NOT_FOUND;
    }
    *out = client->messages[client->head];
    return FVM_OK;
}


/* Release local storage only; client is optional and already quiescent.
 * Returns nothing. Closing/joining the service/bridge remains the host's decision. */
void dvm_ui_client_destroy(DVM_UIClient *client)
{
    free(client);
}
