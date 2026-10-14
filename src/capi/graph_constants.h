/* FlashVM: graph transport representation IDs; do not confuse them with native resource kinds. */
#ifndef FVM_GRAPH_CONSTANTS_H
#define FVM_GRAPH_CONSTANTS_H /* Header guard. */
/* Base offset for local capability wire identities. */
#define FVM_GRAPH_LOCAL_CAPABILITY_BASE 3000UL
/* Base offset for broker-backed capability wire identities. */
#define FVM_GRAPH_REMOTE_CAPABILITY_BASE 4000UL
/* Local channel capability representation. */
#define FVM_GRAPH_LOCAL_CHANNEL 3001UL
/* Local mutex capability representation. */
#define FVM_GRAPH_LOCAL_MUTEX 3008UL
/* Remote channel capability representation. */
#define FVM_GRAPH_REMOTE_CHANNEL 4001UL
/* Remote mutex capability representation. */
#define FVM_GRAPH_REMOTE_MUTEX 4008UL
/* Current append-only graph envelope/schema version. */
#define FVM_GRAPH_WIRE_VERSION 8UL
#endif
