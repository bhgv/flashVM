/* Transport-neutral hybrid UDP/TCP idle policy and checked hints; no implicit sockets or timers. */
#ifndef FLASHVM_RHEARTBEAT_H
#define FLASHVM_RHEARTBEAT_H /* Guard the standalone C89 policy ABI. */
#include "rdispatch.h"
#include "sbindings.h"
#define FVM_RH_UDP 1U        /* Configured UDP path/activity identifier. */
#define FVM_RH_TCP 2U        /* Configured TCP path/activity identifier. */
#define FVM_RH_TRANSPORTS 3U /* Defined transport bits. */
#define FVM_RH_SEND_UDP 1U   /* Idle UDP probe is due; host decides actual transmission. */
#define FVM_RH_SEND_TCP 2U   /* Periodic/requested TCP probe is due on the TCP connection. */
#define FVM_RH_UDP_STALE 4U  /* UDP receive-silence deadline elapsed; not proof of peer death. */
#define FVM_RH_TCP_STALE 8U  /* Independent TCP receive-silence deadline elapsed. */
#define FVM_RH_RECONNECT 16U /* Rate-limited authenticated reconnect advice is pending. */
#define FVM_RH_HINT_TCP 1U   /* Peer requests a TCP check, never a VM/debug command. */
#define FVM_RH_HINT_RECONNECT                                                                      \
    2U                  /* Peer suggests host-controlled reconnection to configured peer. */
#define FVM_RH_HINTS 3U /* Defined hint bits; permissions are configured independently. */
#define FVM_RH_MAX_INTERVAL 0x7fffffffUL /* Half-range monotonic Uint32 clock/age bound. */
#define FVM_RH_HINT_VERSION 1UL /* Version of the four-field canonical S-integer hint payload. */
#define FVM_RH_HINT_FIELDS 4U   /* Version, negotiated epoch, sequence and hint bits. */
#define FVM_RH_HINT_TYPE "flashvm.transport.hint" /* Direction-local binding identity. */
#define FVM_RH_BODY_MAX 32U /* Maximum hint record body, including encoded tag. */
#ifdef __cplusplus
extern "C"
{
#endif
    /* Zero timer disables that periodic/deadline policy; no universal NAT timeout is assumed. */
    typedef struct FVM_RHeartbeatPolicy
    {
        size_t struct_size; /* Exact sizeof this policy; initialize all fields before init. */
        unsigned int
            transports; /* Nonempty subset of UDP/TCP; not socket availability detection. */
        unsigned int allowed_hints; /* Explicit incoming hint permissions; zero denies all. */
        unsigned long
            udp_idle_ms; /* UDP inactivity before suggesting a UDP probe; zero disables. */
        unsigned long
            tcp_idle_ms; /* Independently selected TCP inactivity before suggesting a probe. */
        unsigned long
            request_gap_ms; /* Positive minimum gap for peer-requested actions when enabled. */
        unsigned long
            udp_silence_ms; /* Independent received-UDP silence deadline; zero disables. */
        unsigned long
            tcp_silence_ms; /* Independent received-TCP silence deadline; zero disables. */
    } FVM_RHeartbeatPolicy;
    /* Native hint identity belongs to an already authenticated connection epoch, not a credential.
     */
    typedef struct FVM_RHeartbeatHint
    {
        unsigned long
            epoch; /* Negotiated nonzero Uint32 epoch; fresh on authenticated reconnect. */
        unsigned long
            sequence;       /* Strictly increasing nonzero Uint32; reordering/replay is rejected. */
        unsigned int flags; /* Nonempty subset of HINT_TCP/HINT_RECONNECT. */
    } FVM_RHeartbeatHint;
    /* Caller-owned policy state: use APIs, never transmit native struct bytes or edit live fields.
     */
    typedef struct FVM_RHeartbeat
    {
        size_t struct_size; /* Exact native size, published by successful initialization. */
        FVM_RHeartbeatPolicy policy; /* Immutable copied configuration. */
        unsigned long epoch;         /* Authenticated connection epoch shared by this peer pair. */
        unsigned long
            last_now; /* Last observed monotonic Uint32 milliseconds, wrapping normally. */
        unsigned long udp_idle, tcp_idle; /* Saturating elapsed time since actual path activity. */
        unsigned long udp_silence,
            tcp_silence; /* Saturating elapsed time since received path traffic. */
        unsigned long
            request_age; /* Elapsed time since a requested action was handled/TCP active. */
        unsigned long received_sequence; /* Last accepted authenticated incoming hint. */
        unsigned long sent_sequence;     /* Last generated outgoing hint; never wraps. */
        unsigned int pending; /* Coalesced incoming hint bits; host explicitly handles advice. */
    } FVM_RHeartbeat;
    /* Single-owner/external-serialization API; all error returns leave state/outputs unchanged.
     * Timers are 0..MAX_INTERVAL; hints require UDP+TCP and a positive request gap. No allocation,
     * OS clock lookup, socket I/O, authentication, threads, NAT guarantee or VM call occurs. */
    FVM_API FVM_RResult fvm_rheartbeat_init(FVM_RHeartbeat *, const FVM_RHeartbeatPolicy *,
                                            unsigned long epoch, unsigned long now_ms);
    /* Advance the monotonic clock and suggest action bits. Polling never acknowledges a send or
     * receive. Rollback/ambiguous gap greater than MAX_INTERVAL is INVALID. Stale means suspicion.
     */
    FVM_API FVM_RResult fvm_rheartbeat_due(FVM_RHeartbeat *, unsigned long now_ms,
                                           unsigned int *actions);
    /* Report successful actual send/receive on exactly UDP or TCP; received must be 0/1. Sending
     * resets only path idle; receiving also resets that path silence. UDP never resets TCP state.
     * TCP activity serves a pending TCP-probe hint, but does not clear reconnect advice. */
    FVM_API FVM_RResult fvm_rheartbeat_activity(FVM_RHeartbeat *, unsigned int transport,
                                                int received, unsigned long now_ms);
    /* Invoke only AFTER authenticating and authorizing the received unicast datagram. Checks epoch,
     * sequence and configured hint permissions, coalesces hints, and records actual UDP receipt.
     * Wrong epoch/permission DENIED, replay/out-of-order CONFLICT. CRC alone is not authentication.
     */
    FVM_API FVM_RResult fvm_rheartbeat_accept(FVM_RHeartbeat *, const FVM_RHeartbeatHint *,
                                              unsigned long now_ms);
    /* Host acknowledges/declines coalesced advice. Resets its request gap without claiming actual
     * TCP traffic or peer liveness. Advice never reconnects or executes a VM command automatically.
     */
    FVM_API FVM_RResult fvm_rheartbeat_clear(FVM_RHeartbeat *, unsigned int hints,
                                             unsigned long now_ms);
    /* Generate an outgoing hint for this epoch. Sequence exhaustion LIMIT requires a fresh
     * authenticated epoch; no wrap. I/O retries reuse the returned hint without generating another.
     */
    FVM_API FVM_RResult fvm_rheartbeat_next(FVM_RHeartbeat *, unsigned int hints,
                                            FVM_RHeartbeatHint *);
    /* Encode version,epoch,sequence,hints as canonical unsigned S integers in a checked record.
     * Outputs unchanged on error; no authentication tag/MAC is supplied by this codec. */
    FVM_API FVM_SResult fvm_rheartbeat_hint_write(unsigned long tag, const FVM_RHeartbeatHint *,
                                                  void *, size_t capacity, size_t *used);
    /* Verify framing/CRC, incoming direction binding/type/version and exact four-field payload.
     * Output/consumed change only on OK. Authenticate the raw datagram before accepting its hint.
     */
    FVM_API FVM_SResult fvm_rheartbeat_hint_read(const FVM_SBindings *, const void *, size_t,
                                                 FVM_RHeartbeatHint *, size_t *consumed);
#ifdef __cplusplus
}
#endif
#endif
