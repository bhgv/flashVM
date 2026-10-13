/* FlashVM: Public embedding API declarations: capability.
 * File: include/flashvm/capability.h.
 */

#ifndef FLASHVM_CAPABILITY_H
/* Define FLASHVM CAPABILITY H constant or compilation switch. */
#define FLASHVM_CAPABILITY_H
#include "extensions.h"
#ifdef __cplusplus
extern "C"
{
#endif
    typedef struct FVM_ResourceBroker FVM_ResourceBroker;
    /* Start a TCP resource authority. Port 0 chooses an available port. Advertised
       host must be reachable by recipients; URI format supports DNS names/IPv4.
       Exported resources remain retained until broker stop. Stop before releasing.
   Do not race stop/release calls; bound contexts retain their own references. */
    /* Implement broker start for this subsystem. */
    FVM_API FVM_Result fvm_broker_start(const char *bind_host, const char *advertised_host,
                                        unsigned short port, FVM_ResourceBroker **);
    /* Implement broker port for this subsystem. */
    FVM_API unsigned short fvm_broker_port(FVM_ResourceBroker *);
    /* Copied broker lifetime counters; these are handle/context counts, not memory byte totals. */
    typedef struct FVM_BrokerStats
    {
        size_t peer_contexts;      /* Stable client ownership contexts retained by the broker. */
        size_t exported_resources; /* Retained canonical mutex/channel capability entries. */
        size_t adapter_contexts;   /* [GC:] Graph-conversion contexts currently alive. */
        unsigned long
            adapter_requests; /* [GC:] Successful adapter creations, saturating at ULONG_MAX. */
    } FVM_BrokerStats;
    /* [GC:] Snapshot active broker counters without RPC or allocation; stopped/error leaves output
     * unchanged. */
    FVM_API FVM_Result fvm_broker_stats(FVM_ResourceBroker *, FVM_BrokerStats *);
    /* Retain broker. */
    FVM_API FVM_Result fvm_broker_retain(FVM_ResourceBroker *);
    /* Implement broker stop for this subsystem. */
    FVM_API void fvm_broker_stop(FVM_ResourceBroker *);
    /* Release broker. */
    FVM_API void fvm_broker_release(FVM_ResourceBroker *);
    /* Update context broker. */
    FVM_API FVM_Result fvm_context_set_broker(FVM_Context *, FVM_ResourceBroker *);
    /* Returned URI is malloc-owned; free with fvm_serialized_free. */
    /* Implement broker export for this subsystem. */
    FVM_API FVM_Result fvm_broker_export(FVM_Context *, FVM_Object *, char **);
    /* Implement broker import for this subsystem. */
    FVM_API FVM_Result fvm_broker_import(FVM_Context *, const char *, FVM_Object **);
    /* Open channel network. */
    FVM_API FVM_Result fvm_channel_open_network(FVM_Context *, const char *, FVM_Channel **);
#ifdef __cplusplus
}
#endif
#endif
