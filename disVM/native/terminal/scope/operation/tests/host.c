/* Copyright (c) 2026 bhgv
 * EN: Real Styx ACK/NACK activation, independent RX, ordered native service callbacks and held service output.
 * RU: Real Styx ACK/NACK activation, независимый RX, ordered native callbacks и удержанный service output.
 */
#include "handler.h"
#include "mount.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Owned original native bidir fixture client, no synthetic RPC replies.
 * Свой original native bidir fixture client, без synthetic RPC replies.
 */
extern StyxClient *linkfixture(void);


/* EN/RU: Consume physical fixture only after all root/client users retire, join original server.
 * Поглотить physical fixture после всех root/client users, join original server.
 */
extern void linkfixturefree(void);


/* EN/RU: Refuse next caller-thread allocator once; kind0 malloc, kind1 calloc.
 * Отказать один раз next caller-thread allocator, kind0 malloc, kind1 calloc.
 */
extern void operationallocation(int);


/* EN/RU: Arm actual native service sender stall before original control admission.
 * Включить native service sender stall до original control admission.
 */
extern void operationgatearm(void);


/* EN/RU: Wait for actual held sender, then release; main must first witness broker progress.
 * Ждать actual held sender и отпустить; main сперва наблюдает broker progress.
 */
extern void operationgaterelease(void);


/* EN/RU: Optimized-build contract, diagnostic borrowed through return/exit.
 * Contract optimized build, diagnostic borrowed до return/exit.
 */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "confirmed scope operations: %s\n", why);
        exit(2);
    }
}


/* EN/RU: One native root service record, never a client envelope or a synthetic state check.
 * Одна native root service record, не client envelope либо synthetic state check.
 */
static void service(StyxScope *root, const char *value)
{
    char bytes[64]; /* One copied unknown-to-handler observation. */
    size_t size; /* Actual original body extent. */
    check(styxscopeservicewait(root, 120000) && styxscopeservicenext(root, bytes, sizeof(bytes), &size) == STYX_CONTROL_RECORD &&
        size == strlen(value) && !memcmp(bytes, value, size), "real ordered native service observation");
}


/* EN/RU: Native test control body copied into root-service outbox, no direct client access to root tags.
 * Native test control body копируется в root-service outbox, client не получает root tags.
 */
static void send(StyxScope *root, const char *value)
{
    check(styxscopeservicesend(root, value, strlen(value)), "native root-service outbox admission");
}


/* EN/RU: Client consumes one local message independently of root-service dispatcher.
 * Client потребляет local message независимо от root-service dispatcher.
 */
static void message(StyxScopeClient *client, unsigned long expected, const char *value)
{
    char bytes[64]; /* One client-local body. */
    unsigned long tag; /* Local ID, never transport root ID. */
    size_t size; /* Exact body extent. */
    check(styxscopeclientwait(client, 120000) && styxscopeclientnext(client, &tag, bytes, sizeof(bytes), &size) == STYX_CONTROL_RECORD &&
        tag == expected && size == strlen(value) && !memcmp(bytes, value, size), "client RX progresses during TX operation hold");
}


/* EN/RU: Exact native binding and generation snapshot, no on-wire generation assumption.
 * Exact native binding/generation snapshot, без on-wire generation assumption.
 */
static void binding(StyxScope *scope, unsigned direction, unsigned long value, unsigned long long expected)
{
    unsigned long tag; /* Actual native parent tag. */
    unsigned long long generation; /* Local optimistic-concurrency generation. */
    check(styxscopebinding(scope, direction, &tag, &generation) && tag == value && generation == expected,
        "expected directional map/generation");
}


/* EN/RU: Native transaction state snapshot, no request replay or queue consumption.
 * Native transaction state snapshot, без request replay либо queue consumption.
 */
static void state(StyxScope *root, int expected, unsigned long long id)
{
    unsigned long long operation = 99; /* Native output sentinel. */
    check(styxscopetxstate(root, &operation) == expected && operation == id, "exact confirmation state and correlation");
}


/* EN/RU: Wait on unresolved proposal from independent native transaction owner, result read after join.
 * Ждать unresolved proposal на independent native transaction owner, result читается после join.
 */
typedef struct Waiter
{
    StyxScope *root; /* Borrowed until joined. */
    int result;     /* Worker-owned result before join. */
} Waiter;


/* EN/RU: Block only the transaction owner, never broker or application RX execution.
 * Блокировать лишь transaction owner, не broker либо application RX.
 */
static void *waiting(void *opaque)
{
    Waiter *waiter = opaque; /* Main-owned context through join. */
    waiter->result = styxscopetxwait(waiter->root, 3, 120000);
    return NULL;
}


/* EN/RU: Own real root and all native handler contexts until their final releases and worker joins.
 * Владеть real root и native handler contexts до final releases и worker joins.
 */
int main(void)
{
    StyxClient *client = linkfixture(); /* Independent native client ownership. */
    check(client && styxclientretain(client), "original Styx client pin");
    DVM_Provider *grant = styxmountproviderat(client, ".", FVM_ACCESS_MASK); /* Consumes retained client pin. */
    DVM_TerminalChannels channels = {0}; /* Captured actual native root, no alternative lookup. */
    channels.provider = grant;
    strcpy(channels.receive, "records");
    strcpy(channels.send, "records");
    StyxBidirControl *control = styxbidircontrol(client, &channels, 4096, 1); /* Borrowed below after root takes ownership. */
    StyxScope *root = styxbidirscopes(control, 4096); /* Own control on every outcome. */
    check(root != NULL, "scoped root");
    service(root, "ready");
    StyxScopeClient *a_client; /* Application A has only its isolated inbox/send capability. */
    StyxScopeClient *b_client; /* Application B has no native root-service authority. */
    StyxScope *a = styxscopeclient(root, &a_client); /* Native authority retained by host. */
    StyxScope *b = styxscopeclient(root, &b_client); /* Native sibling authority retained by host. */
    StyxScope *group = styxscopegroup(root); /* Distinct inner namespace with its own outstanding operation. */
    StyxScopeClient *c_client;
    StyxScopeClient *d_client;
    StyxScope *c = styxscopeclient(group, &c_client);
    StyxScope *d = styxscopeclient(group, &d_client);
    check(a && b && group && c && d, "native authorized siblings and independent nested namespace");
    StyxScopeAssignment rx[] = {{a, 11, 0}, {b, 12, 0}}; /* Independent incoming namespace. */
    StyxScopeAssignment tx[] = {{a, 31, 0}, {b, 32, 0}, {group, 50, 0}}; /* Existing outgoing tags before any proposal. */
    check(styxscopeassign(root, STYX_SCOPE_RX, rx, 2) && styxscopeassign(root, STYX_SCOPE_TX, tx, 3), "initial native grants");
    StyxScopeAssignment children[] = {{c, 51, 0}, {d, 52, 0}};
    check(styxscopeassign(group, STYX_SCOPE_TX, children, 2), "nested outgoing client tags");
    OperationHandler second = {a, group, b, NULL, 0, 0, 0}; /* Replacement adopted only after ordered native callback. */
    OperationHandler first = {a, group, b, &second, 0, 0, 0}; /* Main-owned native state through rootfree. */
    OperationHandler refused = {a, group, b, NULL, 0, 0, 0}; /* Refusal leaves state ownership with caller. */
    check(styxscopeservicehandler(root, operationhandler, &first, operationrelease), "ordered native handler");
    operationallocation(1);
    check(!styxscopeservicehandler(root, operationhandler, &refused, operationrelease) && !refused.released,
        "handler allocation refusal leaves original registration and caller state intact");
    check(styxscopeclientsend(a_client, 7, "old", 3), "old application frame accepted before proposal");
    StyxScopeAssignment candidate[] = {{a, 33, 1}}; /* Native candidate copy, not active yet. */
    operationallocation(0);
    check(!styxscopetxprepare(root, 1, candidate, 1, "proposal:1", 10), "proposal allocation refusal has no partial hold/request");
    state(root, STYX_SCOPE_OPERATION_IDLE, 0);
    char request[] = "proposal:1"; /* Overwritten after admission to prove copied outbox data. */
    check(styxscopetxprepare(root, 1, candidate, 1, request, 10), "first ACK-gated proposal");
    memset(request, 'x', 10);
    candidate[0].tag = 999; /* Caller storage cannot rewrite accepted candidate. */
    service(root, "old-seen");
    service(root, "pending:1");
    message(a_client, 0, "ack:1"); /* Client-local zero must not invoke root operation handler. */
    message(a_client, 7, "rx-during-wait");
    state(root, STYX_SCOPE_OPERATION_WAITING, 1);
    binding(a, STYX_SCOPE_TX, 31, 1);
    StyxScopeAssignment blocked = {a, 35, 1}; /* The affected native tag cannot bypass its own confirmation hold. */
    StyxScopeAssignment reserved = {b, 33, 1}; /* Pending new tag is reserved without freezing sibling traffic. */
    StyxScopeAssignment unrelated = {b, 32, 1}; /* Unrelated native sibling assignment remains legal. */
    check(!styxscopeclientsend(a_client, 7, "bad", 3) && !styxscopetxconfirm(root, 99, 1) && !styxscopetxwait(root, 1, 0) &&
        !styxscopeassign(root, STYX_SCOPE_TX, &blocked, 1) && !styxscopeassign(root, STYX_SCOPE_TX, &reserved, 1) &&
        styxscopeassign(root, STYX_SCOPE_TX, &unrelated, 1), "hold and tag reservation affect only selected client");
    check(styxscopeclientsend(b_client, 7, "B-live", 6), "unrelated client TX continues before selected client's ACK");
    service(root, "B-seen");
    StyxScopeAssignment nested = {c, 33, 1}; /* Same candidate ID33 belongs to another tag namespace. */
    check(styxscopetxprepare(group, 1, &nested, 1, "proposal:group", 14), "independent nested operation while root client waits");
    service(root, "group-pending");
    /* EN/RU: The peer may publish its receipt before the last Twrite ACK frees the one-slot native outbox.
     * Fence that fixture write before testing nonblocking sibling admission; the selected profile still awaits its explicit ACK.
     * Peer может прислать receipt до освобождения односвязного outbox последним Twrite ACK.
     * Дождаться этого fixture-write перед nonblocking sibling admission; выбранный профиль всё ещё ждёт своего явного ACK.
     */
    check(styxbidircontrolflush(control, 120000), "proposal write completed before nonblocking sibling admission");
    state(root, STYX_SCOPE_OPERATION_WAITING, 1);
    state(group, STYX_SCOPE_OPERATION_WAITING, 1);
    check(!styxscopeclientsend(c_client, 7, "bad", 3) && styxscopeclientsend(d_client, 7, "D-live", 6),
        "nested hold affects selected C, never sibling D");
    service(root, "D-seen");
    StyxScopeAssignment ancestor = {group, 55, 1}; /* Cannot rewrite wrapper needed by pending child C. */
    check(!styxscopeassign(root, STYX_SCOPE_TX, &ancestor, 1), "pending child protects only its actual ancestor wrapper");
    send(root, "release:group");
    service(root, "group-confirmed");
    state(group, STYX_SCOPE_OPERATION_ACCEPTED, 1);
    state(root, STYX_SCOPE_OPERATION_WAITING, 1);
    binding(c, STYX_SCOPE_TX, 33, 2);
    check(styxscopeclientsend(c_client, 7, "C-new", 5), "nested C resumes on its own ACK while root A remains held");
    service(root, "C-seen");
    send(root, "release:1");
    service(root, "confirmed:1");
    check(styxscopetxwait(root, 1, 0), "actual peer ACK resolves native owner wait");
    state(root, STYX_SCOPE_OPERATION_ACCEPTED, 1);
    binding(a, STYX_SCOPE_TX, 33, 2);
    check(styxscopetxconfirm(root, 1, 1) && !styxscopetxconfirm(root, 1, 0), "duplicate exact ACK idempotent, conflicting outcome refused");
    check(styxscopeclientsend(a_client, 7, "new", 3), "new tag only after actual ACK");
    service(root, "new-seen");
    StyxScopeAssignment rejected[] = {{a, 35, 2}}; /* Peer will explicitly reject this proposal. */
    check(styxscopetxprepare(root, 2, rejected, 1, "proposal:2", 10), "second proposal");
    service(root, "pending:2");
    check(styxscopetxconfirm(root, 1, 1), "previous exact ACK may repeat during a newer pending operation");
    state(root, STYX_SCOPE_OPERATION_WAITING, 2);
    check(styxscopetxwait(root, 1, 0) && !styxscopetxwait(root, 2, 0) && !styxscopetxwait(root, 99, 120000),
        "operation wait never resolves from another ACK or waits on another operation");
    send(root, "release:2");
    service(root, "confirmed:2");
    state(root, STYX_SCOPE_OPERATION_REJECTED, 2);
    binding(a, STYX_SCOPE_TX, 33, 2);
    check(styxscopeclientsend(a_client, 7, "after-nack", 10), "explicit NACK preserves old map and restores TX");
    service(root, "nack-seen");
    operationgatearm();
    send(root, "change-rx");
    service(root, "probe-during-ack"); /* Broker progressed while native service sender cannot admit ACK. */
    binding(a, STYX_SCOPE_RX, STYX_OPERATION_CASE == 4 ? 11 : 12, STYX_OPERATION_CASE == 4 ? 1 : 2);
    check(styxscopeclientsend(b_client, 7, "B-during-ack", 12), "unrelated TX continues while native service ACK sender is blocked");
    service(root, "B-during-ack-seen");
    operationgaterelease();
    service(root, STYX_OPERATION_CASE == 4 ? "rx-rejected:4" : "rx-accepted:4");
    message(a_client, 7, "before-rx");
    message(a_client, 7, STYX_OPERATION_CASE == 4 ? "after-rejected-rx" : "after-rx");
    send(root, "bad-rx");
    service(root, "rx-rejected:5");
    binding(a, STYX_SCOPE_RX, STYX_OPERATION_CASE == 4 ? 11 : 12, STYX_OPERATION_CASE == 4 ? 1 : 2);
    message(a_client, 7, "unchanged-rx");
    StyxScopeAssignment uncertain[] = {{a, 37, 2}}; /* No peer confirmation will arrive. */
    check(!styxscopetxprepare(root, 2, uncertain, 1, "proposal:3", 10) &&
        styxscopetxprepare(root, 3, uncertain, 1, "proposal:3", 10), "retired request ID cannot be reused");
    service(root, "pending:3");
    check(styxscopetxconfirm(root, 2, 0) && !styxscopetxconfirm(root, 2, 1) && !styxscopetxwait(root, 3, 0),
        "old negative repeat cannot release current uncertain operation");
    state(root, STYX_SCOPE_OPERATION_WAITING, 3);
    check(styxscopeclientsend(b_client, 7, "B-live", 6), "missing ACK does not freeze unrelated client");
    service(root, "B-uncertain-seen");
    binding(a, STYX_SCOPE_TX, 33, 2);
    send(root, "replace");
    service(root, "replacement-observed");
    check(__atomic_load_n(&first.released, __ATOMIC_ACQUIRE) == 1 && !first.bad_release && !second.released,
        "handler self-replacement releases old state only after admitted callback returns");
    if (STYX_OPERATION_CASE == 2)
    {
        check(styxscopeservicehandler(root, NULL, NULL, NULL) && second.released == 1, "explicit handler detachment releases adopted state");
    }
    Waiter waiter = {root, -1}; /* Independent native pending-operation owner. */
    pthread_t worker; /* Joined before rootfree. */
    check(!pthread_create(&worker, NULL, waiting, &waiter), "native transaction owner wait");
    if (STYX_OPERATION_CASE == 1)
    {
        styxclientshutdown(client);
    }
    else if (STYX_OPERATION_CASE == 3)
    {
        send(root, "malformed");
    }
    else
    {
        styxscopestop(root);
    }
    check(!pthread_join(worker, NULL) && !waiter.result, "retirement wakes transaction owner without uncertain TX activation");
    state(root, STYX_SCOPE_OPERATION_TERMINAL, 3);
    check(!styxscopetxconfirm(root, 3, 1), "late ACK after retirement cannot revive TX");
    message(a_client, 7, "rx-while-uncertain");
    styxscopefree(root);
    check(first.released == 1 && second.released == 1 && !first.bad_release && !second.bad_release && !refused.released,
        "exact adopted handler disposal, refused state never consumed");
    check(STYX_OPERATION_CASE == 1 || styxclientalive(client), "local operation retirement preserves shared client");
    dvm_provider_release(grant);
    styxclientshutdown(client);
    styxclientfree(client);
    linkfixturefree();
    puts("PASS real ACK/NACK gating, ordered service handling and independent broker RX");
    return 0;
}
