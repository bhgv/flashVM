/* Independent observers, permission-filtered discovery, revocation and bounded cleanup. */
#include "flashvm/revents.h"
#include <stdio.h>
#include <string.h>
#include "../../src/protocol/rdispatch.c"
#define CHECK(x)                                                                                   \
    do                                                                                             \
    {                                                                                              \
        if (!(x))                                                                                  \
        {                                                                                          \
            fprintf(stderr, "events:%d %s\n", __LINE__, #x);                                       \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)
static FVM_RDispatch *dispatch;
static int owner_thread = 1, locked, failed;
static unsigned long executed[8];
static size_t executions;
static void acquire(void *raw)
{
    (void)raw;
    if (locked)
        failed = 1;
    locked = 1;
}
static void release(void *raw)
{
    (void)raw;
    if (!locked)
        failed = 1;
    locked = 0;
}
static int is_owner(void *raw)
{
    (void)raw;
    return owner_thread;
}
static FVM_RResult handler(const FVM_RRequest *q, void *reply, size_t capacity, size_t *used,
                           void *raw)
{
    unsigned long sequence;
    (void)reply;
    (void)capacity;
    (void)raw;
    if (locked || fvm_rdispatch_disconnect(dispatch, q->session) != FVM_R_BUSY ||
        fvm_rdispatch_publish(dispatch, q->target, 7, q->data, q->size, &sequence) != FVM_R_OK)
        failed = 1;
    executed[executions++] = q->id;
    *used = 0;
    return FVM_R_OK;
}
int main(void)
{
    FVM_RPolicy policy;
    FVM_ROperation operations[2] = {{1, FVM_R_READ}, {2, FVM_R_CONTROL}};
    FVM_RHandle target, hidden, replacement;
    FVM_RWatch first, second, old, extra[FVM_R_WATCHES];
    FVM_REvent event, before;
    FVM_RResource rows[2], saved[2];
    FVM_RRequest q;
    FVM_RReply reply;
    unsigned long sequence = 99;
    unsigned char data = 17;
    size_t count, done, i;
    memset(&policy, 0, sizeof(policy));
    policy.struct_size = sizeof(policy);
    policy.lock = acquire;
    policy.unlock = release;
    policy.is_owner = is_owner;
    CHECK(fvm_rdispatch_create(&policy, &dispatch) == FVM_R_OK);
    CHECK(fvm_rdispatch_register(dispatch, "/debug/ctl", operations, 2, handler, 0, &target) ==
          FVM_R_OK);
    CHECK(fvm_rdispatch_register(dispatch, "/audio/ctl", operations, 2, handler, 0, &hidden) ==
          FVM_R_OK);
    CHECK(fvm_rdispatch_grant(dispatch, target, 1, FVM_R_READ | FVM_R_SUBSCRIBE | FVM_R_CONTROL) ==
          FVM_R_OK);
    CHECK(fvm_rdispatch_grant(dispatch, target, 2, FVM_R_READ | FVM_R_SUBSCRIBE) == FVM_R_OK);
    count = 99;
    CHECK(fvm_rdispatch_resources(dispatch, 2, 0, 0, &count) == FVM_R_OK && count == 1);
    memset(rows, 0xa5, sizeof(rows));
    memcpy(saved, rows, sizeof(rows));
    count = 99;
    CHECK(fvm_rdispatch_resources(dispatch, 2, rows, 0, &count) == FVM_R_LIMIT && count == 99);
    CHECK(memcmp(rows, saved, sizeof(rows)) == 0);
    CHECK(fvm_rdispatch_resources(dispatch, 2, rows, 2, &count) == FVM_R_OK && count == 1);
    CHECK(!strcmp(rows[0].path, "/debug/ctl") && rows[0].count == 1 &&
          rows[0].operations[0].id == 1);
    CHECK(rows[0].rights == (FVM_R_READ | FVM_R_SUBSCRIBE));
    CHECK(fvm_rdispatch_subscribe(dispatch, 3, target, &first) == FVM_R_DENIED);
    CHECK(fvm_rdispatch_subscribe(dispatch, 1, hidden, &first) == FVM_R_DENIED);
    CHECK(fvm_rdispatch_subscribe(dispatch, 1, target, &first) == FVM_R_OK);
    CHECK(fvm_rdispatch_subscribe(dispatch, 2, target, &second) == FVM_R_OK);
    memset(&event, 0xa5, sizeof(event));
    before = event;
    CHECK(fvm_rdispatch_event_take(dispatch, 2, first, &event) == FVM_R_DENIED);
    CHECK(fvm_rdispatch_event_take(dispatch, 1, first, &event) == FVM_R_PENDING);
    CHECK(!memcmp(&event, &before, sizeof(event)));
    owner_thread = 0;
    CHECK(fvm_rdispatch_publish(dispatch, target, 1, &data, 1, &sequence) == FVM_R_DENIED &&
          sequence == 99);
    CHECK(fvm_rdispatch_disconnect(dispatch, 1) == FVM_R_DENIED);
    owner_thread = 1;
    for (i = 0; i < FVM_R_EVENT_QUEUE + 3; ++i)
    {
        data = (unsigned char)i;
        CHECK(fvm_rdispatch_publish(dispatch, target, 3, &data, 1, &sequence) == FVM_R_OK &&
              sequence == i + 1);
        CHECK(fvm_rdispatch_event_take(dispatch, 1, first, &event) == FVM_R_OK);
        CHECK(event.sequence == sequence && !event.missed && event.data[0] == i && event.size == 1);
        data = 255;
    }
    CHECK(fvm_rdispatch_event_take(dispatch, 2, second, &event) == FVM_R_OK);
    CHECK(event.sequence == 4 && event.missed == 3 && event.data[0] == 3);
    CHECK(fvm_rdispatch_event_take(dispatch, 2, second, &event) == FVM_R_OK && !event.missed);
    old = second;
    CHECK(fvm_rdispatch_grant(dispatch, target, 2, FVM_R_READ) == FVM_R_OK);
    CHECK(fvm_rdispatch_event_take(dispatch, 2, old, &event) == FVM_R_UNKNOWN);
    CHECK(fvm_rdispatch_grant(dispatch, target, 2, FVM_R_SUBSCRIBE) == FVM_R_OK);
    CHECK(fvm_rdispatch_subscribe(dispatch, 2, target, &second) == FVM_R_OK);
    CHECK(second.generation != old.generation);
    CHECK(fvm_rdispatch_event_take(dispatch, 2, second, &event) == FVM_R_PENDING);
    CHECK(fvm_rdispatch_unsubscribe(dispatch, 1, second) == FVM_R_DENIED);
    CHECK(fvm_rdispatch_unsubscribe(dispatch, 2, second) == FVM_R_OK);
    CHECK(fvm_rdispatch_unsubscribe(dispatch, 2, second) == FVM_R_UNKNOWN);
    for (i = 0; i < FVM_R_WATCHES - 1; ++i)
        CHECK(fvm_rdispatch_subscribe(dispatch, 1, target, extra + i) == FVM_R_OK);
    CHECK(fvm_rdispatch_subscribe(dispatch, 1, target, &second) == FVM_R_LIMIT);
    /* Exhausted subscription generations are retired rather than wrapped. */
    second = extra[0];
    second.generation = FVM_SRECORD_U32;
    dispatch->subscriptions[second.id - 1].generation = second.generation;
    CHECK(fvm_rdispatch_unsubscribe(dispatch, 1, second) == FVM_R_OK);
    CHECK(fvm_rdispatch_subscribe(dispatch, 1, target, &second) == FVM_R_LIMIT);
    CHECK(fvm_rdispatch_disconnect(dispatch, 1) == FVM_R_OK);
    CHECK(fvm_rdispatch_event_take(dispatch, 1, first, &event) == FVM_R_UNKNOWN);
    CHECK(fvm_rdispatch_resources(dispatch, 1, rows, 2, &count) == FVM_R_OK && count == 0);
    /* Disconnect must unlink queued jobs at all FIFO positions and discard replies. */
    CHECK(fvm_rdispatch_grant(dispatch, target, 1, FVM_R_CONTROL | FVM_R_SUBSCRIBE) == FVM_R_OK);
    CHECK(fvm_rdispatch_grant(dispatch, target, 2, FVM_R_CONTROL) == FVM_R_OK);
    memset(&q, 0, sizeof(q));
    q.target = target;
    q.operation = 2;
    for (i = 1; i <= 5; ++i)
    {
        q.id = (unsigned long)i;
        q.session = (i == 2 || i == 4) ? 2 : 1;
        CHECK(fvm_rdispatch_post(dispatch, &q) == FVM_R_OK);
    }
    CHECK(fvm_rdispatch_disconnect(dispatch, 1) == FVM_R_OK);
    CHECK(fvm_rdispatch_pump(dispatch, FVM_R_REQUESTS, &done) == FVM_R_OK && done == 2);
    CHECK(executions == 2 && executed[0] == 2 && executed[1] == 4 && !failed);
    CHECK(fvm_rdispatch_take(dispatch, 1, 1, &reply) == FVM_R_UNKNOWN);
    CHECK(fvm_rdispatch_disconnect(dispatch, 2) == FVM_R_OK);
    CHECK(fvm_rdispatch_take(dispatch, 2, 2, &reply) == FVM_R_UNKNOWN);
    CHECK(fvm_rdispatch_grant(dispatch, target, 3, FVM_R_SUBSCRIBE) == FVM_R_OK);
    CHECK(fvm_rdispatch_subscribe(dispatch, 3, target, &first) == FVM_R_OK);
    CHECK(fvm_rdispatch_unregister(dispatch, target) == FVM_R_OK);
    CHECK(fvm_rdispatch_event_take(dispatch, 3, first, &event) == FVM_R_UNKNOWN);
    CHECK(fvm_rdispatch_register(dispatch, "/debug/ctl", operations, 2, handler, 0, &replacement) ==
          FVM_R_OK);
    CHECK(replacement.generation != target.generation);
    CHECK(fvm_rdispatch_publish(dispatch, target, 1, 0, 0, &sequence) == FVM_R_UNKNOWN);
    dispatch->sequence = FVM_SRECORD_U32;
    sequence = 99;
    CHECK(fvm_rdispatch_publish(dispatch, replacement, 1, 0, 0, &sequence) == FVM_R_LIMIT &&
          sequence == 99);
    CHECK(fvm_rdispatch_publish(dispatch, replacement, 1, 0, FVM_R_BYTES + 1, &sequence) ==
          FVM_R_INVALID);
    CHECK(fvm_rdispatch_destroy(&dispatch) == FVM_R_OK && !dispatch && !locked && !failed);
    return 0;
}
