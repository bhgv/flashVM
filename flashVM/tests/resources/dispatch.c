/* Native resource state-machine contract, including allocation failure without VM side effects. */
#include "flashvm/rdispatch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int fail_allocation;
/* Inject exactly the dispatcher allocation while preserving ordinary native cleanup. */
static void *allocate_dispatch(size_t n, size_t size)
{
    return fail_allocation ? 0 : calloc(n, size);
}
#define calloc allocate_dispatch
#include "../../src/protocol/rdispatch.c"
#undef calloc
#define CHECK(x)                                                                                   \
    do                                                                                             \
    {                                                                                              \
        if (!(x))                                                                                  \
        {                                                                                          \
            fprintf(stderr, "resource line %d: %s\n", __LINE__, #x);                               \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)
/* Trusted owner policy verifies no callback is invoked while the queue lock is held. */
typedef struct Host
{
    FVM_RDispatch *dispatch;
    FVM_RHandle target;
    int locked, calls, owner, failed;
    unsigned long order[FVM_R_REQUESTS];
} Host;
/* Test lock deliberately rejects recursive acquisition on the same thread. */
static void acquire(void *raw)
{
    Host *h = (Host *)raw;
    if (h->locked)
        h->failed = 1;
    h->locked = 1;
}
/* Release is paired even on every public error return. */
static void release(void *raw)
{
    Host *h = (Host *)raw;
    if (!h->locked)
        h->failed = 1;
    h->locked = 0;
}
/* Owner thread identity is represented explicitly in this deterministic fixture. */
static int is_owner(void *raw)
{
    return ((Host *)raw)->owner;
}
/* Exercise callback reentry guards and copied payload/result ownership. */
static FVM_RResult handler(const FVM_RRequest *request, void *out, size_t capacity, size_t *used,
                           void *raw)
{
    Host *h = (Host *)raw;
    FVM_RReply reply;
    size_t done = 777;
    if (h->locked || !h->owner || capacity != FVM_R_BYTES ||
        fvm_rdispatch_pump(h->dispatch, 1, &done) != FVM_R_BUSY || done != 777 ||
        fvm_rdispatch_unregister(h->dispatch, h->target) != FVM_R_BUSY ||
        fvm_rdispatch_cancel(h->dispatch, request->session, request->id) != FVM_R_BUSY ||
        fvm_rdispatch_take(h->dispatch, request->session, request->id, &reply) != FVM_R_PENDING ||
        fvm_rdispatch_destroy(&h->dispatch) != FVM_R_BUSY)
        h->failed = 1;
    h->order[h->calls++ % FVM_R_REQUESTS] = request->id;
    if (request->size)
        memcpy(out, request->data, request->size);
    *used = request->size;
    return FVM_R_OK;
}
/* Malformed handler output is rejected after committed native side effects. */
static FVM_RResult malformed(const FVM_RRequest *q, void *out, size_t capacity, size_t *used,
                             void *raw)
{
    (void)q;
    (void)out;
    (void)raw;
    if (q->operation == 2)
    {
        *used = 0;
        return FVM_R_PENDING;
    }
    *used = capacity + 1;
    return FVM_R_OK;
}
/* Bound acceptance/results and prove stale generation/revoked commands never invoke owner code. */
int main(void)
{
    Host host;
    FVM_RPolicy policy;
    FVM_ROperation ops[2] = {{1, FVM_R_READ}, {2, FVM_R_WRITE}};
    FVM_RRequest request;
    FVM_RReply reply, before;
    FVM_RHandle old, found;
    FVM_RDispatch *sentinel = (FVM_RDispatch *)1;
    unsigned char bytes[2] = {7, 0};
    size_t done, i;
    memset(&host, 0, sizeof(host));
    host.owner = 1;
    memset(&policy, 0, sizeof(policy));
    policy.struct_size = sizeof(policy);
    policy.lock = acquire;
    policy.unlock = release;
    policy.is_owner = is_owner;
    policy.userdata = &host;
    fail_allocation = 1;
    CHECK(fvm_rdispatch_create(&policy, &sentinel) == FVM_R_NOMEM &&
          sentinel == (FVM_RDispatch *)1);
    fail_allocation = 0;
    CHECK(fvm_rdispatch_create(&policy, &host.dispatch) == FVM_R_OK);
    CHECK(fvm_rdispatch_register(host.dispatch, "/scene/root/x", ops, 2, handler, &host,
                                 &host.target) == FVM_R_OK);
    old = host.target;
    CHECK(fvm_rdispatch_register(host.dispatch, "/scene/root/../x", ops, 2, handler, &host,
                                 &found) == FVM_R_INVALID);
    CHECK(fvm_rdispatch_register(host.dispatch, "/scene/root/x", ops, 2, handler, &host, &found) ==
          FVM_R_CONFLICT);
    CHECK(fvm_rdispatch_lookup(host.dispatch, 1, "/scene/root/x", &found) == FVM_R_DENIED);
    CHECK(fvm_rdispatch_grant(host.dispatch, old, 1, FVM_R_READ) == FVM_R_OK);
    CHECK(fvm_rdispatch_lookup(host.dispatch, 1, "/scene/root/x", &found) == FVM_R_OK &&
          found.id == old.id);
    memset(&request, 0, sizeof(request));
    request.session = 1;
    request.id = 1;
    request.target = old;
    request.operation = 2;
    request.data = bytes;
    request.size = 2;
    CHECK(fvm_rdispatch_post(host.dispatch, &request) == FVM_R_DENIED);
    request.operation = 1;
    CHECK(fvm_rdispatch_post(host.dispatch, &request) == FVM_R_OK);
    bytes[0] = 99;
    CHECK(fvm_rdispatch_post(host.dispatch, &request) == FVM_R_CONFLICT && !host.calls);
    memset(&reply, 0xa5, sizeof(reply));
    memcpy(&before, &reply, sizeof(reply));
    CHECK(fvm_rdispatch_take(host.dispatch, 1, 1, &reply) == FVM_R_PENDING &&
          !memcmp(&before, &reply, sizeof(reply)));
    CHECK(fvm_rdispatch_pump(host.dispatch, 1, &done) == FVM_R_OK && done == 1 && host.calls == 1 &&
          !host.failed);
    CHECK(fvm_rdispatch_take(host.dispatch, 1, 1, &reply) == FVM_R_OK && reply.result == FVM_R_OK &&
          reply.size == 2 && reply.data[0] == 7);
    CHECK(fvm_rdispatch_take(host.dispatch, 1, 1, &reply) == FVM_R_UNKNOWN);
    request.id = 2;
    CHECK(fvm_rdispatch_post(host.dispatch, &request) == FVM_R_OK);
    CHECK(fvm_rdispatch_grant(host.dispatch, old, 1, 0) == FVM_R_OK);
    CHECK(fvm_rdispatch_pump(host.dispatch, 1, &done) == FVM_R_OK && host.calls == 1);
    CHECK(fvm_rdispatch_take(host.dispatch, 1, 2, &reply) == FVM_R_OK &&
          reply.result == FVM_R_DENIED);
    CHECK(fvm_rdispatch_grant(host.dispatch, old, 1, FVM_R_READ) == FVM_R_OK);
    request.id = 3;
    CHECK(fvm_rdispatch_post(host.dispatch, &request) == FVM_R_OK);
    CHECK(fvm_rdispatch_unregister(host.dispatch, old) == FVM_R_OK);
    CHECK(fvm_rdispatch_register(host.dispatch, "/scene/root/x", ops, 2, handler, &host,
                                 &host.target) == FVM_R_OK &&
          host.target.generation != old.generation);
    CHECK(fvm_rdispatch_grant(host.dispatch, host.target, 1, FVM_R_READ) == FVM_R_OK);
    CHECK(fvm_rdispatch_pump(host.dispatch, 1, &done) == FVM_R_OK && host.calls == 1);
    CHECK(fvm_rdispatch_take(host.dispatch, 1, 3, &reply) == FVM_R_OK &&
          reply.result == FVM_R_UNKNOWN);
    CHECK(fvm_rdispatch_post(host.dispatch, &request) == FVM_R_UNKNOWN);
    request.target = host.target;
    host.calls = 0;
    for (i = 0; i < FVM_R_REQUESTS; ++i)
    {
        request.id = (unsigned long)i + 1;
        CHECK(fvm_rdispatch_post(host.dispatch, &request) == FVM_R_OK);
    }
    request.id = 100;
    CHECK(fvm_rdispatch_post(host.dispatch, &request) == FVM_R_LIMIT);
    CHECK(fvm_rdispatch_destroy(&host.dispatch) == FVM_R_BUSY);
    CHECK(fvm_rdispatch_cancel(host.dispatch, 1, 3) == FVM_R_OK);
    CHECK(fvm_rdispatch_take(host.dispatch, 1, 3, &reply) == FVM_R_OK &&
          reply.result == FVM_R_CANCELLED);
    CHECK(fvm_rdispatch_post(host.dispatch, &request) == FVM_R_OK);
    CHECK(fvm_rdispatch_pump(host.dispatch, FVM_R_REQUESTS, &done) == FVM_R_OK &&
          done == FVM_R_REQUESTS);
    CHECK(host.order[0] == 1 && host.order[1] == 2 && host.order[2] == 4 &&
          host.order[FVM_R_REQUESTS - 1] == 100 && !host.failed);
    for (i = 1; i <= FVM_R_REQUESTS; ++i)
        if (i != 3)
            CHECK(fvm_rdispatch_take(host.dispatch, 1, (unsigned long)i, &reply) == FVM_R_OK);
    CHECK(fvm_rdispatch_take(host.dispatch, 1, 100, &reply) == FVM_R_OK);
    host.owner = 0;
    done = 777;
    CHECK(fvm_rdispatch_pump(host.dispatch, 1, &done) == FVM_R_DENIED && done == 777);
    CHECK(fvm_rdispatch_unregister(host.dispatch, host.target) == FVM_R_DENIED);
    host.owner = 1;
    CHECK(fvm_rdispatch_unregister(host.dispatch, host.target) == FVM_R_OK);
    CHECK(fvm_rdispatch_register(host.dispatch, "/bad", ops, 2, malformed, 0, &host.target) ==
          FVM_R_OK);
    CHECK(fvm_rdispatch_grant(host.dispatch, host.target, 1, FVM_R_READ | FVM_R_WRITE) == FVM_R_OK);
    request.target = host.target;
    request.id = 200;
    CHECK(fvm_rdispatch_post(host.dispatch, &request) == FVM_R_OK);
    CHECK(fvm_rdispatch_pump(host.dispatch, 1, &done) == FVM_R_OK);
    CHECK(fvm_rdispatch_take(host.dispatch, 1, 200, &reply) == FVM_R_OK &&
          reply.result == FVM_R_HANDLER && reply.size == 0);
    request.id = 201;
    request.operation = 2;
    CHECK(fvm_rdispatch_post(host.dispatch, &request) == FVM_R_OK);
    CHECK(fvm_rdispatch_pump(host.dispatch, 1, &done) == FVM_R_OK);
    CHECK(fvm_rdispatch_take(host.dispatch, 1, 201, &reply) == FVM_R_OK &&
          reply.result == FVM_R_HANDLER && !reply.size);
    /* Exhaustion is injected at the internal boundary to avoid billions of unregister cycles. */
    old = host.target;
    host.dispatch->resources[old.id - 1].generation = FVM_SRECORD_U32;
    old.generation = FVM_SRECORD_U32;
    CHECK(fvm_rdispatch_unregister(host.dispatch, old) == FVM_R_OK);
    CHECK(host.dispatch->resources[old.id - 1].generation == 0);
    CHECK(fvm_rdispatch_register(host.dispatch, "/new", ops, 2, handler, &host, &host.target) ==
              FVM_R_OK &&
          host.target.id != old.id);
    for (i = 1; i <= FVM_R_GRANTS; ++i)
        CHECK(fvm_rdispatch_grant(host.dispatch, host.target, (unsigned long)i, FVM_R_READ) ==
              FVM_R_OK);
    CHECK(fvm_rdispatch_grant(host.dispatch, host.target, FVM_R_GRANTS + 1, FVM_R_READ) ==
          FVM_R_LIMIT);
    CHECK(fvm_rdispatch_grant(host.dispatch, host.target, 2, 0) == FVM_R_OK);
    CHECK(fvm_rdispatch_grant(host.dispatch, host.target, FVM_R_GRANTS + 1, FVM_R_READ) ==
          FVM_R_OK);
    CHECK(fvm_rdispatch_destroy(&host.dispatch) == FVM_R_OK && !host.dispatch && !host.failed &&
          !host.locked);
    puts("PASS copied FIFO, grants/revocation, stale generations, cancellation/reuse, owner "
         "reentry and allocation failure");
    return 0;
}
