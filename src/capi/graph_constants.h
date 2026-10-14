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
/* Newest accepted graph envelope/schema version; emit older compatible layouts when possible. */
#define FVM_GRAPH_WIRE_VERSION 9UL
/* Graphs without interface snapshots keep the previous byte-compatible schema. */
#define FVM_GRAPH_BASE_WIRE_VERSION 8UL
/* First schema carrying a per-object interface-edge representation flag. */
#define FVM_GRAPH_SNAPSHOT_INTERFACES_VERSION 9UL
#define FVM_GRAPH_INTERFACES_CONSTRUCTORS 0U /* Legacy interface class references. */
#define FVM_GRAPH_INTERFACES_PROTOTYPES 1U   /* Frozen interface prototype references. */
#define FVM_GRAPH_VERSION_OFFSET 7U          /* Version byte in the graph envelope. */
#define FVM_GRAPH_MIN_WIRE_VERSION 1U        /* Oldest accepted graph envelope/schema. */
#define FVM_GRAPH_MAGIC_PREFIX_BYTES 7U      /* Signature bytes preceding the envelope version. */
#endif
