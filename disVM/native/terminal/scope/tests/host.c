/* Copyright (c) 2026 bhgv
 * EN: Real Styx nested isolation, atomic RX swaps, frozen TX paths and rollback of malformed/NOMEM envelopes.
 * RU: Real Styx nested isolation, atomic RX swaps, frozen TX paths и rollback malformed/NOMEM envelopes.
 */
#include "scope.h"
#include "mount.h"
#include "flashvm/srecord.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Arm native constructor fault before scoped subscription/worker setup; mode0 is ordinary.
 * Включить native constructor fault до subscription/worker setup, mode0 обычный.
 */
extern void scopefaultarm(void);


/* EN/RU: Deliver already-admitted root callbacks after rootfree, 1 ownership checkpoints valid/0 invalid.
 * Выдать already-admitted root callbacks после rootfree, 1 checkpoints valid/0 invalid.
 */
extern int scopelate(void);


/* EN/RU: Own actual bidir original-Styx fixture client, no fake transport replies.
 * Свой actual bidir original-Styx fixture client, без fake transport replies.
 */
extern StyxClient *linkfixture(void);


/* EN/RU: Consume physical fixture after all client/recipient users retire, join original server owner.
 * Поглотить physical fixture после всех client/recipient users, join original server owner.
 */
extern void linkfixturefree(void);


/* EN/RU: Explicit fixture contract, diagnostic borrowed through return/exit.
 * Явный fixture contract, diagnostic borrowed до return/exit.
 */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "nested scopes: %s\n", why);
        exit(2);
    }
}


/* EN/RU: One independent native client readiness caller, outputs read only after join.
 * Один independent native client readiness caller, outputs читаются после join.
 */
typedef struct Waiting
{
    StyxScopeClient *client; /* Borrowed inbox through join. */
    int result;             /* Native waiter result, no message consumption. */
} Waiting;


/* EN/RU: Block only one client on its own queue; no broker access or guest dispatch.
 * Блокировать лишь одного клиента на своей очереди, без broker access либо guest dispatch.
 */
static void *waiting(void *state)
{
    Waiting *wait = state; /* Main-owned joined context. */
    wait->result = styxscopeclientwait(wait->client, 120000);
    return NULL;
}


/* EN/RU: Consume exactly one native root/group service, caller owns output, no client sees these records.
 * Потребить ровно одну native root/group service, clients не видят эти records.
 */
static void service(StyxScope *scope, const char *expected)
{
    char bytes[64]; /* One local native service body. */
    size_t size; /* Actual body extent. */
    check(styxscopeservicewait(scope, 120000) &&
        styxscopeservicenext(scope, bytes, sizeof(bytes), &size) == STYX_CONTROL_RECORD &&
        size == strlen(expected) && !memcmp(bytes, expected, size), "native service stays in its own scope");
}


/* EN/RU: One copied client record with independent expected local tag/body, SMALL never consumes it.
 * Одна client record с independent expected local tag/body, SMALL не потребляет запись.
 */
static void message(StyxScopeClient *client, unsigned long expected, const void *value, size_t length)
{
    unsigned char bytes[128]; /* Caller-owned one-message output. */
    unsigned long tag = 999; /* Preserved by SMALL. */
    size_t size = 999; /* Required/copied body extent. */
    if (length)
    {
        check(styxscopeclientnext(client, &tag, NULL, 0, &size) == STYX_CONTROL_SMALL && tag == 999 && size == length,
            "client SMALL preserves exact local head");
    }
    check(styxscopeclientnext(client, &tag, bytes, sizeof(bytes), &size) == STYX_CONTROL_RECORD &&
        tag == expected && size == length && (!size || (value && !memcmp(bytes, value, size))),
        "one client-local ordered payload, never another scope's tag interpretation");
}


/* EN/RU: Native-only fixture command uses a borrowed control owned by root, no application receives this pointer.
 * Native-only fixture command через borrowed control root; application этот pointer не получает.
 */
static void command(StyxBidirControl *control, const char *value)
{
    check(styxbidircontrolsendwait(control, 102, value, 4, 120000, NULL) && styxbidircontrolflush(control, 120000),
        "actual native fixture command crossed original Styx");
}


/* EN/RU: Copy current native binding, validate exact local expected generation without wire assumptions.
 * Скопировать native binding, проверить local generation без wire assumptions.
 */
static void binding(StyxScope *scope, unsigned direction, unsigned long expected_tag, unsigned long long expected_generation)
{
    unsigned long tag; /* Native-only copied outer identifier. */
    unsigned long long generation; /* Native-only stale-decision guard. */
    check(styxscopebinding(scope, direction, &tag, &generation) && tag == expected_tag && generation == expected_generation,
        "exact native directional binding generation");
}


/* EN/RU: Explicitly stop transport/source or malformed input and verify independent client wake/atomic rollback.
 * Завершить transport/source либо malformed input; проверить независимые wake и atomic rollback.
 */
static void failure(StyxScope *root, StyxScope *group, StyxScopeClient **clients,
    StyxClient *client, StyxBidirControl *control)
{
    if (STYX_SCOPE_CASE == 4)
    {
        styxclientshutdown(client);
    }
    else if (STYX_SCOPE_CASE == 5)
    {
        styxscopestop(root);
    }
    else
    {
        command(control, "fill");
    }
    check(!styxscopeclientwait(clients[2], 120000), "empty client wakes on protocol/stop/source retirement");
    int expected = STYX_SCOPE_CASE == 2 ? STYX_CONTROL_NOMEM : STYX_SCOPE_CASE == 4 ? STYX_CONTROL_READ_FAILED :
        STYX_SCOPE_CASE == 5 ? STYX_CONTROL_STOPPED : STYX_CONTROL_PROTOCOL; /* First native outcome. */
    check(styxscopestate(root) == expected, "precise scoped broker terminal reason");
    unsigned char bytes[128]; /* Rejected client/group transaction must not leak a prefix. */
    unsigned long tag; /* Local output sentinel. */
    size_t size; /* Local output size. */
    check(styxscopeclientnext(clients[2], &tag, bytes, sizeof(bytes), &size) == STYX_CONTROL_END &&
        styxscopeservicenext(group, bytes, sizeof(bytes), &size) == STYX_CONTROL_END,
        "malformed group transaction publishes neither service prefix nor client prefix");
    if (STYX_SCOPE_CASE != 6)
    {
        check(styxscopeclientnext(clients[0], &tag, bytes, sizeof(bytes), &size) == STYX_CONTROL_END,
            "malformed/failed outer transaction never publishes earlier staged client record");
    }
    else
    {
        message(clients[0], 7, "alpha", 5);
        message(clients[1], 7, "beta", 4);
    }
}


/* EN/RU: Main owns native root/tree and real Styx fixture through every inbox/assignment/TX call.
 * Main владеет native root/tree и real Styx fixture до конца inbox/assignment/TX calls.
 */
int main(void)
{
    StyxClient *client = linkfixture(); /* Caller-owned pin independent of provider/control. */
    check(client && styxclientretain(client), "original native bidir client");
    DVM_Provider *grant = styxmountproviderat(client, ".", FVM_ACCESS_MASK); /* Consumes retained client pin. */
    DVM_TerminalChannels channels = {0}; /* Actual captured native grant. */
    channels.provider = grant;
    strcpy(channels.receive, "records");
    strcpy(channels.send, "records");
    StyxBidirControl *control = styxbidircontrol(client, &channels, 4096, 1); /* Owned by scoped root after create. */
    scopefaultarm();
    StyxScope *root = styxbidirscopes(control, STYX_SCOPE_CASE == 11 ? 128 : 4096); /* Explicit nested profile, never auto-upgrades legacy route. */
    if (STYX_SCOPE_CASE == 7 || STYX_SCOPE_CASE == 9 || STYX_SCOPE_CASE == 10)
    {
        check(!root && styxclientalive(client), "constructor fault consumes only owned control and private root");
        control = styxbidircontrol(client, &channels, 4096, 1);
        root = styxbidirscopes(control, 4096);
    }
    check(root != NULL, "native scoped broker");
    service(root, "ready");
    StyxScopeClient *clients[3]; /* Application handles expose no parent/root service access. */
    StyxScope *a = styxscopeclient(root, &clients[0]); /* Native host authority A. */
    StyxScope *b = styxscopeclient(root, &clients[1]); /* Native host authority B. */
    StyxScope *group = styxscopegroup(root); /* Native intermediary namespace. */
    StyxScope *c = styxscopeclient(group, &clients[2]); /* Native host authority C. */
    check(a && b && group && c && !styxscopegroup(a), "separate group and leaf authority");
    StyxScopeAssignment initial[] = {{a, 11, 0}, {b, 12, 0}, {group, 20, 0}}; /* Fixture-only negotiated RX tags. */
    StyxScopeAssignment child = {c, 11, 0}; /* Same outer11 reused safely inside group. */
    check(styxscopeassign(root, STYX_SCOPE_RX, initial, 3) && styxscopeassign(group, STYX_SCOPE_RX, &child, 1),
        "independent nested RX namespaces");
    check(!styxscopeclientsend(clients[0], 7, "client-tx", 9), "RX assignment never grants outbound authority");
    if ((STYX_SCOPE_CASE > 0 && STYX_SCOPE_CASE < 7) || STYX_SCOPE_CASE == 11)
    {
        failure(root, group, clients, client, control);
    }
    else
    {
        Waiting waits[] = {{clients[0], -1}, {clients[1], -1}, {clients[2], -1}}; /* Independent native readiness callers. */
        pthread_t workers[3]; /* Joined before root retirement. */
        for (unsigned i = 0; i < 3; i++)
        {
            check(!pthread_create(&workers[i], NULL, waiting, &waits[i]), "independent blocked client");
        }
        command(control, "fill");
        service(root, "system");
        service(root, "done"); /* Wire-order barrier proves all preceding envelopes committed. */
        service(group, "group-system");
        for (unsigned i = 0; i < 3; i++)
        {
            check(!pthread_join(workers[i], NULL) && waits[i].result, "each client wakes on its own scope");
        }
        StyxScopeAssignment swap[] = {{a, 12, 1}, {b, 11, 1}}; /* One atomic sibling swap. */
        check(styxscopeassign(root, STYX_SCOPE_RX, swap, 2), "atomic tag reassignment without intermediate conflicts");
        check(!styxscopeassign(root, STYX_SCOPE_RX, swap, 2), "stale native generation cannot reapply remap");
        binding(a, STYX_SCOPE_RX, 12, 2);
        binding(b, STYX_SCOPE_RX, 11, 2);
        StyxScopeAssignment collision = {a, 11, 2}; /* Would collide with B after swap. */
        check(!styxscopeassign(root, STYX_SCOPE_RX, &collision, 1) && !styxscopeassign(root, STYX_SCOPE_RX, &child, 1),
            "final duplicate tags and foreign-parent capabilities are refused atomically");
        command(control, "next");
        service(root, "after");
        message(clients[0], 7, "alpha", 5);
        message(clients[0], 0, "local-zero", 10);
        unsigned char opaque[64]; /* Client opaque payload may itself look like a transport-control record. */
        size_t used; /* Exact independent codec oracle. */
        check(fvm_srecord_write(0, "not-system", 10, opaque, sizeof(opaque), &used) == FVM_S_OK, "opaque golden frame");
        message(clients[0], 8, opaque, used);
        for (unsigned i = 0; i < 512; i++)
        {
            unsigned char value[] = {0, 0, (unsigned char)(i >> 8), (unsigned char)i}; /* Independent FIFO oracle. */
            message(clients[0], 7, value, sizeof(value));
        }
        message(clients[0], 7, "new-A", 5);
        message(clients[1], 7, "beta", 4);
        message(clients[1], 7, "new-B", 5);
        message(clients[2], 7, "group", 5);
        check(!styxscopeservicewait(root, 0) && !styxscopeservicewait(group, 0), "client-local zero never becomes ancestor service");
        StyxScopeAssignment outgoing[] = {{a, 31, 0}, {group, 40, 0}}; /* Explicit TX tags unrelated to RX. */
        StyxScopeAssignment inner = {c, 41, 0}; /* Nested outgoing client grant. */
        check(styxscopeassign(root, STYX_SCOPE_TX, outgoing, 2) && styxscopeassign(group, STYX_SCOPE_TX, &inner, 1), "explicit TX tree");
        check(styxscopeclientsend(clients[0], 7, "client-tx", 9), "application sends only local7");
        StyxScopeAssignment changed = {a, 33, 1}; /* Accepted queued TX must still carry frozen outer31. */
        check(styxscopeassign(root, STYX_SCOPE_TX, &changed, 1) && styxbidircontrolflush(control, 120000), "remap after TX acceptance");
        service(root, "tx-ack");
        check(styxscopeclientsend(clients[2], 0, "child-control", 13) && styxbidircontrolflush(control, 120000), "nested local-zero TX");
        service(root, "nested-tx");
        unsigned char large[4096] = {0}; /* Inner body plus envelope overhead exceeds broker limit. */
        check(!styxscopeclientsend(clients[0], 7, large, sizeof(large)) && !styxscopeclientsend(clients[1], 7, "x", 1) &&
            !styxscopestate(root), "size/ungranted TX refusal preserves broker");
        StyxScope *deep = root; /* Native-only depth guard, no wire probing or recursive payload guessing. */
        for (unsigned i = 0; i < STYX_SCOPE_DEPTH; i++)
        {
            deep = styxscopegroup(deep);
            check(deep != NULL, "bounded native group nesting");
        }
        check(!styxscopegroup(deep), "depth overflow refused before publication");
    }
    styxscopefree(root);
    check(STYX_SCOPE_CASE != 8 || scopelate(), "late callbacks use detached native fence after rootfree");
    check(STYX_SCOPE_CASE == 4 || styxclientalive(client), "root disposal preserves host's shared client");
    dvm_provider_release(grant);
    styxclientshutdown(client);
    styxclientfree(client);
    linkfixturefree();
    puts("PASS isolated nested namespaces and native-only transport authority");
    return 0;
}
