/* This adapter uses the genuine thread-safe FlashVM channel, never a VM object pointer. */
#include "flash_channel.h"
#include "codec.h"
#include <stdlib.h>


/* One owner pumps this native record; no field contains a Dis/AS managed reference. */
struct DVM_BrokerFlashPump
{
    DVM_Broker *broker;               /* Borrowed metadata broker. */
    FVM_Channel *output;              /* Borrowed actual copied-value channel. */
    uint32_t listener;                /* Owned global deviations-only subscription. */
    uint64_t dropped;                 /* Saturating count of upstream diagnostic overflow. */
    size_t size;                      /* Pending counted frame extent; zero means no pending delivery. */
    char frame[DVM_BROKER_FRAME_MAX]; /* Owned canonical ASCII bytes retained across backpressure. */
};


/* Allocate a global subscription and private pending storage; publish no partial pump on error. */
FVM_Result dvm_broker_flash_create(DVM_Broker *broker, FVM_Channel *output, DVM_BrokerFlashPump **out)
{
    DVM_BrokerFlashPump *pump;
    if (!broker || !output || !out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    pump = calloc(1, sizeof(*pump));
    if (!pump)
    {
        return FVM_OUT_OF_MEMORY;
    }
    if (dvm_broker_watch(broker, 0, 0, &pump->listener) != DVM_BROKER_OK)
    {
        free(pump);
        return FVM_ERROR;
    }
    pump->broker = broker;
    pump->output = output;
    *out = pump;
    return FVM_OK;
}


/* Keep an unaccepted frame across full/closed output; no receipt is mistaken for RPC completion. */
FVM_Result dvm_broker_flash_step(DVM_BrokerFlashPump *pump)
{
    FVM_Value value;
    FVM_Result result;
    if (!pump)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!pump->size)
    {
        DVM_BrokerEvent event;
        uint64_t dropped;
        DVM_BrokerResult status = dvm_broker_poll(pump->broker, pump->listener, &event, &dropped);
        if (status != DVM_BROKER_OK && status != DVM_BROKER_EMPTY)
        {
            return FVM_ERROR;
        }
        pump->dropped = dropped > UINT64_MAX - pump->dropped ? UINT64_MAX : pump->dropped + dropped;
        if (status == DVM_BROKER_EMPTY)
        {
            return FVM_NOT_FOUND;
        }
        if (dvm_broker_encode(&event, pump->frame, sizeof(pump->frame), &pump->size) != DVM_BROKER_OK)
        {
            return FVM_INVALID_ARGUMENT;
        }
    }
    result = fvm_value_string_bytes(&value, pump->frame, pump->size);
    if (result == FVM_OK)
    {
        result = fvm_channel_try_send(pump->output, &value);
    }
    if (result == FVM_OK)
    {
        pump->size = 0;
    }
    return result;
}


/* Change only the borrowed transport endpoint; the copied pending frame remains owned. */
FVM_Result dvm_broker_flash_rebind(DVM_BrokerFlashPump *pump, FVM_Channel *output)
{
    if (!pump || !output)
    {
        return FVM_INVALID_ARGUMENT;
    }
    pump->output = output;
    return FVM_OK;
}


/* Owner-only metric query, suitable for a log/monitoring path independent of the output channel. */
uint64_t dvm_broker_flash_dropped(const DVM_BrokerFlashPump *pump)
{
    return pump ? pump->dropped : 0;
}


/* Retire native metadata after quiescence; both broker and output are still caller-owned. */
void dvm_broker_flash_destroy(DVM_BrokerFlashPump *pump)
{
    if (pump)
    {
        dvm_broker_unwatch(pump->broker, pump->listener);
        free(pump);
    }
}
