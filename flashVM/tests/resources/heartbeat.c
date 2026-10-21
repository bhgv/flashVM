/* Deterministic hybrid transport policy: no sleeps, sockets, NAT assumptions or VM runtime. */
#include "flashvm/rheartbeat.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x)                                                                                   \
    do                                                                                             \
    {                                                                                              \
        if (!(x))                                                                                  \
        {                                                                                          \
            fprintf(stderr, "heartbeat:%d %s\n", __LINE__, #x);                                    \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)
int main(void)
{
    FVM_RHeartbeatPolicy policy;
    FVM_RHeartbeat h, before;
    FVM_RHeartbeatHint hint, next, saved;
    unsigned int actions;
    memset(&policy, 0, sizeof(policy));
    policy.struct_size = sizeof(policy);
    policy.transports = FVM_RH_TRANSPORTS;
    policy.allowed_hints = FVM_RH_HINTS;
    policy.udp_idle_ms = 100;
    policy.tcp_idle_ms = 1000;
    policy.request_gap_ms = 300;
    policy.udp_silence_ms = 500;
    policy.tcp_silence_ms = 2000;
    CHECK(fvm_rheartbeat_init(&h, &policy, 7, 0) == FVM_R_OK);
    CHECK(fvm_rheartbeat_due(&h, 99, &actions) == FVM_R_OK && !actions);
    CHECK(fvm_rheartbeat_due(&h, 100, &actions) == FVM_R_OK && actions == FVM_RH_SEND_UDP);
    CHECK(fvm_rheartbeat_due(&h, 100, &actions) == FVM_R_OK && actions == FVM_RH_SEND_UDP);
    CHECK(fvm_rheartbeat_activity(&h, FVM_RH_UDP, 0, 100) == FVM_R_OK && h.udp_silence == 100);
    CHECK(fvm_rheartbeat_due(&h, 599, &actions) == FVM_R_OK &&
          actions == (FVM_RH_SEND_UDP | FVM_RH_UDP_STALE));
    hint.epoch = 7;
    hint.sequence = 1;
    hint.flags = FVM_RH_HINT_TCP;
    CHECK(fvm_rheartbeat_accept(&h, &hint, 600) == FVM_R_OK && h.tcp_idle == 600 &&
          h.udp_silence == 0);
    CHECK(fvm_rheartbeat_due(&h, 600, &actions) == FVM_R_OK && actions == FVM_RH_SEND_TCP);
    CHECK(fvm_rheartbeat_activity(&h, FVM_RH_TCP, 0, 600) == FVM_R_OK && h.tcp_silence == 600 &&
          !h.pending);
    hint.sequence = 2;
    hint.flags = FVM_RH_HINTS;
    CHECK(fvm_rheartbeat_accept(&h, &hint, 700) == FVM_R_OK);
    CHECK(fvm_rheartbeat_due(&h, 700, &actions) == FVM_R_OK && !actions);
    CHECK(fvm_rheartbeat_due(&h, 900, &actions) == FVM_R_OK &&
          actions == (FVM_RH_SEND_UDP | FVM_RH_SEND_TCP | FVM_RH_RECONNECT));
    CHECK(fvm_rheartbeat_clear(&h, FVM_RH_HINT_RECONNECT, 900) == FVM_R_OK &&
          h.pending == FVM_RH_HINT_TCP && h.tcp_idle == 300);
    CHECK(fvm_rheartbeat_due(&h, 900, &actions) == FVM_R_OK && actions == FVM_RH_SEND_UDP);
    CHECK(fvm_rheartbeat_activity(&h, FVM_RH_TCP, 0, 900) == FVM_R_OK);
    CHECK(fvm_rheartbeat_due(&h, 2000, &actions) == FVM_R_OK && (actions & FVM_RH_TCP_STALE));
    CHECK(fvm_rheartbeat_activity(&h, FVM_RH_UDP, 1, 2000) == FVM_R_OK && h.udp_silence == 0 &&
          h.tcp_silence == 2000);
    CHECK(fvm_rheartbeat_due(&h, 2000, &actions) == FVM_R_OK && (actions & FVM_RH_TCP_STALE));
    CHECK(fvm_rheartbeat_activity(&h, FVM_RH_TCP, 1, 2000) == FVM_R_OK && h.tcp_silence == 0);
    before = h;
    hint.epoch = 8;
    hint.sequence = 3;
    CHECK(fvm_rheartbeat_accept(&h, &hint, 2001) == FVM_R_DENIED &&
          !memcmp(&h, &before, sizeof(h)));
    hint.epoch = 7;
    hint.sequence = 2;
    CHECK(fvm_rheartbeat_accept(&h, &hint, 2001) == FVM_R_CONFLICT &&
          !memcmp(&h, &before, sizeof(h)));
    hint.sequence = 3;
    hint.flags = 4;
    CHECK(fvm_rheartbeat_accept(&h, &hint, 2001) == FVM_R_INVALID &&
          !memcmp(&h, &before, sizeof(h)));
    actions = 99;
    CHECK(fvm_rheartbeat_due(&h, 1999, &actions) == FVM_R_INVALID && actions == 99 &&
          !memcmp(&h, &before, sizeof(h)));
    CHECK(fvm_rheartbeat_activity(&h, FVM_RH_TRANSPORTS, 1, 2001) == FVM_R_INVALID &&
          !memcmp(&h, &before, sizeof(h)));
    CHECK(fvm_rheartbeat_activity(&h, FVM_RH_UDP, 2, 2001) == FVM_R_INVALID &&
          !memcmp(&h, &before, sizeof(h)));
    CHECK(fvm_rheartbeat_next(&h, FVM_RH_HINT_TCP, &next) == FVM_R_OK && next.epoch == 7 &&
          next.sequence == 1);
    h.sent_sequence = FVM_SRECORD_U32;
    before = h;
    saved = next;
    CHECK(fvm_rheartbeat_next(&h, FVM_RH_HINT_TCP, &next) == FVM_R_LIMIT &&
          !memcmp(&next, &saved, sizeof(next)) && !memcmp(&h, &before, sizeof(h)));
    policy.request_gap_ms = 0;
    CHECK(fvm_rheartbeat_init(&h, &policy, 7, 0) == FVM_R_INVALID &&
          !memcmp(&h, &before, sizeof(h)));
    policy.allowed_hints = 0;
    policy.transports = FVM_RH_UDP;
    policy.udp_idle_ms = 10;
    CHECK(fvm_rheartbeat_init(&h, &policy, 9, FVM_SRECORD_U32 - 10) == FVM_R_OK);
    CHECK(fvm_rheartbeat_due(&h, 5, &actions) == FVM_R_OK && h.udp_idle == 16 &&
          actions == FVM_RH_SEND_UDP);
    before = h;
    hint.epoch = 9;
    hint.sequence = 1;
    hint.flags = FVM_RH_HINT_TCP;
    CHECK(fvm_rheartbeat_accept(&h, &hint, 6) == FVM_R_DENIED && !memcmp(&h, &before, sizeof(h)));
    CHECK(fvm_rheartbeat_init(&h, &policy, 9, 0) == FVM_R_OK);
    CHECK(fvm_rheartbeat_due(&h, FVM_RH_MAX_INTERVAL, &actions) == FVM_R_OK);
    CHECK(fvm_rheartbeat_due(&h, FVM_SRECORD_U32 - 1, &actions) == FVM_R_OK);
    CHECK(fvm_rheartbeat_due(&h, FVM_RH_MAX_INTERVAL - 2, &actions) == FVM_R_OK);
    CHECK(h.udp_idle == FVM_RH_MAX_INTERVAL && h.udp_silence == FVM_RH_MAX_INTERVAL &&
          (actions & FVM_RH_UDP_STALE));
    CHECK(fvm_rheartbeat_activity(&h, FVM_RH_UDP, 0, h.last_now) == FVM_R_OK && !h.udp_idle &&
          h.udp_silence == FVM_RH_MAX_INTERVAL);

    /* A delayed host decision starts its throttle at the actual decision time. */
    policy.transports = FVM_RH_TRANSPORTS;
    policy.allowed_hints = FVM_RH_HINTS;
    policy.request_gap_ms = 300;
    CHECK(fvm_rheartbeat_init(&h, &policy, 10, 0) == FVM_R_OK);
    hint.epoch = 10;
    hint.sequence = 1;
    hint.flags = FVM_RH_HINT_RECONNECT;
    CHECK(fvm_rheartbeat_accept(&h, &hint, 600) == FVM_R_OK);
    CHECK(fvm_rheartbeat_clear(&h, FVM_RH_HINT_RECONNECT, 1000) == FVM_R_OK && h.request_age == 0);
    hint.sequence = 2;
    CHECK(fvm_rheartbeat_accept(&h, &hint, 1001) == FVM_R_OK);
    CHECK(fvm_rheartbeat_due(&h, 1001, &actions) == FVM_R_OK && !(actions & FVM_RH_RECONNECT));
    CHECK(fvm_rheartbeat_due(&h, 1300, &actions) == FVM_R_OK && (actions & FVM_RH_RECONNECT));
    before = h;
    CHECK(fvm_rheartbeat_clear(&h, FVM_RH_HINT_RECONNECT, 1299) == FVM_R_INVALID &&
          !memcmp(&h, &before, sizeof(h)));
    /* Complete typed record path plus malformed prefixes, under memory sanitizers too. */
    {
        FVM_SBindings table;
        FVM_SBinding slots[1], declaration;
        FVM_RHeartbeatHint decoded, original;
        unsigned char body[256], wire[256], damaged[256];
        size_t bytes, used, metadata, cut;
        memset(&declaration, 0, sizeof(declaration));
        declaration.tag = 3;
        declaration.version = 1;
        declaration.type_size = (unsigned int)strlen(FVM_RH_HINT_TYPE);
        memcpy(declaration.type, FVM_RH_HINT_TYPE, declaration.type_size);
        CHECK(fvm_sbindings_init(&table, slots, 1) == FVM_S_OK);
        CHECK(fvm_sbindings_payload_write(&declaration, 1, body, sizeof(body), &metadata) ==
              FVM_S_OK);
        CHECK(fvm_srecord_write(0, body, metadata, wire, sizeof(wire), &bytes) == FVM_S_OK);
        CHECK(fvm_sbindings_apply(&table, wire, bytes, &used) == FVM_S_OK);
        hint.epoch = FVM_SRECORD_U32;
        hint.sequence = 64;
        hint.flags = FVM_RH_HINTS;
        CHECK(fvm_rheartbeat_hint_write(3, &hint, wire, sizeof(wire), &bytes) == FVM_S_OK);
        CHECK(fvm_rheartbeat_hint_read(&table, wire, bytes, &decoded, &used) == FVM_S_OK &&
              used == bytes);
        CHECK(decoded.epoch == hint.epoch && decoded.sequence == 64 &&
              decoded.flags == FVM_RH_HINTS);
        original = decoded;
        for (cut = 0; cut < bytes; ++cut)
        {
            used = 777;
            CHECK(fvm_rheartbeat_hint_read(&table, wire, cut, &decoded, &used) == FVM_S_MORE &&
                  used == 777);
            CHECK(!memcmp(&decoded, &original, sizeof(decoded)));
        }
        memcpy(damaged, wire, bytes);
        damaged[bytes - 1] ^= 1;
        CHECK(fvm_rheartbeat_hint_read(&table, damaged, bytes, &decoded, &used) == FVM_S_CHECKSUM &&
              used == 777);
        CHECK(!memcmp(&decoded, &original, sizeof(decoded)));
    }
    puts("PASS independent UDP/TCP clocks, requested probe throttling, replay/epoch checks, actual "
         "receive proof and clock wrap/saturation");
    return 0;
}
