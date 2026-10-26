/* Optional bridge from a global broker subscription to a real FlashVM value channel. */
#ifndef DVM_BROKER_FLASH_CHANNEL_H
#define DVM_BROKER_FLASH_CHANNEL_H
#include "broker.h"
#include "flashvm/channel.h"
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct DVM_BrokerFlashPump
    DVM_BrokerFlashPump; /* Owns its global listener and at most one pending copied frame. */


/* Create a global deviations-only subscription. broker/output are borrowed until destruction.
 * Configure a bounded output channel before other users start. out is unchanged on failure.
 * Caller serializes pump calls; broker/FlashVM channel may have other native users. */
FVM_Result dvm_broker_flash_create(DVM_Broker *broker, FVM_Channel *output, DVM_BrokerFlashPump **out);


/* Try to deliver at most one record without waiting, evaluating objects or entering a VM context.
 * NOT_FOUND means empty/busy. Busy delivery retains exactly one frame for the next step.
 * ERROR from a closed output retains the pending frame until rebind or explicit destruction. */
FVM_Result dvm_broker_flash_step(DVM_BrokerFlashPump *pump);


/* Rebind a borrowed output channel after host-controlled recovery; preserve a pending record. */
FVM_Result dvm_broker_flash_rebind(DVM_BrokerFlashPump *pump, FVM_Channel *output);


/* Copy total broker-queue overflow count observed by this pump. No normal-success notifications. */
uint64_t dvm_broker_flash_dropped(const DVM_BrokerFlashPump *pump);


/* Remove the owned listener/free native copies after pump users quiesce; never close borrowed output.
 * A pending record is explicitly discarded. Quiesce before destroying broker/output as well. */
void dvm_broker_flash_destroy(DVM_BrokerFlashPump *pump);
#ifdef __cplusplus
}
#endif
#endif
