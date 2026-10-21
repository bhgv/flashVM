/* Hybrid timer policy demonstration, with virtual time and no network I/O; see README.md. */
#include "flashvm/rheartbeat.h" /* Native C89 policy and compact S-record hint encoder. */
#include <stdio.h>              /* Print decisions; the library itself has no console dependency. */
#include <string.h>             /* Explicitly initialize every configuration field. */
#define EXAMPLE_UDP_IDLE 30000UL   /* Illustrative 30 seconds, not a universal NAT timeout. */
#define EXAMPLE_TCP_IDLE 300000UL  /* Illustrative TCP probes are ten times less frequent. */
#define EXAMPLE_REQUEST_GAP 5000UL /* Bound peer-triggered TCP/reconnect activity. */
#define EXAMPLE_HINT_TIME 31000UL  /* Virtual authenticated hint arrives after the UDP send. */
#define EXAMPLE_REPLY_TIME 32000UL /* Virtual successful TCP receive, independent from UDP. */
/* A host replaces virtual timestamps and printed decisions with its real monotonic clock/I/O. */
int main(void) /* This is an executable policy example, not a STUN or UDP server. */
{              /* Policy storage is caller-owned and contains no managed pointers. */
    FVM_RHeartbeatPolicy policy; /* Stack configuration copied into successful initialization. */
    FVM_RHeartbeat state;        /* One serialized owner mutates this peer's state. */
    FVM_RHeartbeatHint hint, /* Continue this explicitly counted policy operation or its error branch. */
        outgoing;              /* Authenticated ingress and independently sequenced output. */
    unsigned int actions;      /* Decisions do not prove that any packet was sent or received. */
    unsigned char record[128]; /* Private bounded wire output; native structs are not serialized. */
    size_t bytes;              /* Exact S-record byte count. */
    memset(&policy, 0, sizeof(policy));    /* All periodic/deadline policies default to disabled. */
    policy.struct_size = sizeof(policy);   /* Validate the native ABI descriptor. */
    policy.transports = FVM_RH_TRANSPORTS; /* Host has chosen both UDP and TCP paths. */
    policy.allowed_hints = /* Continue this explicitly counted policy operation or its error branch. */
        FVM_RH_HINTS; /* Explicitly allow probe and reconnect advice for this peer. */
    policy.udp_idle_ms = EXAMPLE_UDP_IDLE; /* Idle time on UDP before suggesting its probe. */
    policy.tcp_idle_ms = EXAMPLE_TCP_IDLE; /* TCP uses an independent, slower periodic timer. */
    policy.request_gap_ms = EXAMPLE_REQUEST_GAP; /* Requests are coalesced and rate-limited. */
    if (fvm_rheartbeat_init(&state, &policy, 7, 0) != FVM_R_OK) /* Check fvm_rheartbeat_init before using native outputs. */
        return 1; /* Epoch 7 models an already authenticated connection. */
    if (fvm_rheartbeat_due(&state, EXAMPLE_UDP_IDLE, &actions) != FVM_R_OK || /* Check fvm_rheartbeat_due before using native outputs. */
        actions != FVM_RH_SEND_UDP) /* Continue this explicitly counted policy operation or its error branch. */
        return 1; /* The TCP interval has not expired. */
    printf("periodic UDP due; TCP interval=%lu ms\n", /* Continue this explicitly counted policy operation or its error branch. */
           policy.tcp_idle_ms); /* Expected first decision. */
    if (fvm_rheartbeat_activity(&state, FVM_RH_UDP, 0, EXAMPLE_UDP_IDLE) != FVM_R_OK) /* Check fvm_rheartbeat_activity before using native outputs. */
        return 1; /* Report a successful virtual UDP send, not receipt. */
    hint.epoch = 7; /* Continue this explicitly counted policy operation or its error branch. */
    hint.sequence = 1; /* Continue this explicitly counted policy operation or its error branch. */
    hint.flags = /* Continue this explicitly counted policy operation or its error branch. */
        FVM_RH_HINTS; /* Model authenticated advice, not bytes from an unverified datagram. */
    if (fvm_rheartbeat_accept(&state, &hint, EXAMPLE_HINT_TIME) != FVM_R_OK) /* Check fvm_rheartbeat_accept before using native outputs. */
        return 1; /* Real ingress must authenticate before this call. */
    if (fvm_rheartbeat_due(&state, EXAMPLE_HINT_TIME, &actions) != FVM_R_OK) /* Check fvm_rheartbeat_due before using native outputs. */
        return 1; /* Requested TCP check can occur before its periodic timer. */
    if (actions != (FVM_RH_SEND_TCP | FVM_RH_RECONNECT)) /* Continue this explicitly counted policy operation or its error branch. */
        return 1; /* The independent minimum request gap already elapsed. */
    puts("authenticated UDP advice: TCP check due; reconnect remains a host decision"); /* No
                                                                                           connection
                                                                                           is opened
                                                                                           here. */
    if (fvm_rheartbeat_clear(&state, FVM_RH_HINT_RECONNECT, EXAMPLE_HINT_TIME) != FVM_R_OK) /* Check fvm_rheartbeat_clear before using native outputs. */
        return 1; /* Host declines reconnect advice without claiming TCP liveness. */
    if (fvm_rheartbeat_activity(&state, FVM_RH_TCP, 0, EXAMPLE_HINT_TIME) != FVM_R_OK) /* Check fvm_rheartbeat_activity before using native outputs. */
        return 1; /* A virtual TCP send serves the pending probe. */
    printf("TCP received-silence=%lu ms after send\n", /* Continue this explicitly counted policy operation or its error branch. */
           state.tcp_silence); /* Sending alone does not prove the peer responded. */
    if (fvm_rheartbeat_activity(&state, FVM_RH_TCP, 1, EXAMPLE_REPLY_TIME) != FVM_R_OK) /* Check fvm_rheartbeat_activity before using native outputs. */
        return 1; /* A real successful receive would reset only TCP silence. */
    if (state.tcp_silence != 0) /* Continue this explicitly counted policy operation or its error branch. */
        return 1; /* Verify the distinct receive-proof transition. */
    if (fvm_rheartbeat_next(&state, FVM_RH_HINT_TCP, &outgoing) != FVM_R_OK) /* Check fvm_rheartbeat_next before using native outputs. */
        return 1; /* Generate a nonwrapping outgoing sequence. */
    if (fvm_rheartbeat_hint_write(3, &outgoing, record, sizeof(record), &bytes) != FVM_S_OK) /* Check fvm_rheartbeat_hint_write before using native outputs. */
        return 1; /* Encode canonical S integers and CRC, without authentication. */
    printf("outgoing hint sequence=%lu; checked record=%lu bytes\n", outgoing.sequence, /* Continue this explicitly counted policy operation or its error branch. */
           (unsigned long)bytes); /* Keep the copied hint/record for I/O retry. */
    return 0; /* No heap, VM, SDK, thread, socket or filesystem cleanup is required. */
} /* End the deterministic hybrid heartbeat policy example. */
