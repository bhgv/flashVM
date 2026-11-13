/* Copyright (c) 2026 bhgv
 * EN: Exercise root lifetime, replay, rollback, aliases, expiry and reconnect.
 * RU: Проверить roots, replay, rollback, aliases, expiry и переподключение.
 */
#include "session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN: Fail with the exact source expression; condition executes once.
 * RU: Завершить с точным выражением; condition вычисляется один раз. */
#define CHECK(condition)                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(condition))                                                                                              \
        {                                                                                                              \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);                                       \
            exit(1);                                                                                                   \
        }                                                                                                              \
    } while (0)


/* EN: Initialize one fully copied call; target and argument are borrowed handles.
 * RU: Создать полностью копируемый call; target и argument — borrowed handles. */
static DvmRemoteRequest call(uint64_t id, DvmRemoteHandle target, DvmRemoteHandle argument)
{
    DvmRemoteRequest request;
    memset(&request, 0, sizeof(request));
    request.id = id;
    request.kind = DVM_REMOTE_FUNCTION;
    request.operation = 7;
    request.target = target;
    request.ref_count = 2;
    request.refs[0] = argument;
    request.refs[1] = argument;
    request.payload_size = 3;
    memcpy(request.payload, "abc", 3);
    return request;
}


/* EN: Prove close cannot collect a target or repeated ref while its call runs.
 * RU: Доказать, что close не собирает target/repeated ref до конца call. */
static void active_close(void)
{
    int function_root = 1;
    int variable_root = 2;
    int done = 99;
    DvmRemoteHandle function;
    DvmRemoteHandle variable;
    DvmRemoteRoot root;
    DvmRemoteRequest request;
    DvmRemoteBorrow borrowed;
    DvmRemoteSession *session = dvm_remote_session_new(2, 4, 100);
    CHECK(session != NULL);
    CHECK(dvm_remote_publish(session, DVM_REMOTE_FUNCTION, &function_root, &function) == DVM_REMOTE_OK);
    CHECK(dvm_remote_publish(session, DVM_REMOTE_VARIABLE, &variable_root, &variable) == DVM_REMOTE_OK);
    request = call(1, function, variable);
    CHECK(dvm_remote_admit(session, &request, &done) == DVM_REMOTE_OK && done == 0);
    CHECK(dvm_remote_borrow(session, 0, &borrowed) == DVM_REMOTE_INVALID);
    CHECK(dvm_remote_borrow(session, 1, NULL) == DVM_REMOTE_INVALID);
    CHECK(dvm_remote_borrow(session, 2, &borrowed) == DVM_REMOTE_STALE);
    CHECK(dvm_remote_borrow(session, 1, &borrowed) == DVM_REMOTE_OK);
    CHECK(borrowed.target.token == &function_root && borrowed.refs[0].token == &variable_root &&
          borrowed.refs[1].token == borrowed.refs[0].token && borrowed.request.ref_count == 2);
    CHECK(dvm_remote_admit(session, &request, &done) == DVM_REMOTE_DUPLICATE && done == 0);
    CHECK(dvm_remote_release(session, variable) == DVM_REMOTE_OK);
    CHECK(dvm_remote_release(session, variable) == DVM_REMOTE_STALE);
    CHECK(dvm_remote_retain(session, variable) == DVM_REMOTE_STALE);
    CHECK(dvm_remote_collect(session, &root) == DVM_REMOTE_STALE);
    dvm_remote_session_close(session);
    dvm_remote_session_close(session);
    CHECK(dvm_remote_borrow(session, 1, &borrowed) == DVM_REMOTE_OK);
    CHECK(dvm_remote_admit(session, &request, &done) == DVM_REMOTE_CLOSED);
    CHECK(dvm_remote_session_tick(session, 1, 200) == DVM_REMOTE_CLOSED);
    CHECK(dvm_remote_collect(session, &root) == DVM_REMOTE_STALE);
    CHECK(dvm_remote_session_destroy(session) == DVM_REMOTE_BUSY);
    CHECK(dvm_remote_finish(session, 1) == DVM_REMOTE_OK);
    CHECK(dvm_remote_borrow(session, 1, &borrowed) == DVM_REMOTE_STALE);
    CHECK(dvm_remote_finish(session, 1) == DVM_REMOTE_STALE);
    CHECK(dvm_remote_collect(session, &root) == DVM_REMOTE_OK && root.token == &function_root);
    CHECK(dvm_remote_collect(session, &root) == DVM_REMOTE_OK && root.token == &variable_root);
    CHECK(dvm_remote_collect(session, &root) == DVM_REMOTE_STALE);
    CHECK(dvm_remote_session_destroy(session) == DVM_REMOTE_OK);
}


/* EN: Exact duplicate/conflict checks survive a lost completion response. Bounded
 * eviction rejects old IDs, active entries block overwrite, UINT64_MAX never wraps.
 * RU: Повтор/conflict проверяются после потери ответа. Окно отвергает старые IDs,
 * active entries не вытесняются, UINT64_MAX не переполняется. */
static void replay_window(void)
{
    int roots[2] = {1, 2};
    int done;
    DvmRemoteHandle function;
    DvmRemoteHandle variable;
    DvmRemoteRequest first;
    DvmRemoteRequest next;
    DvmRemoteRoot root;
    DvmRemoteSession *session = dvm_remote_session_new(2, 2, 100);
    CHECK(session != NULL);
    CHECK(dvm_remote_publish(session, DVM_REMOTE_FUNCTION, &roots[0], &function) == DVM_REMOTE_OK);
    CHECK(dvm_remote_publish(session, DVM_REMOTE_VARIABLE, &roots[1], &variable) == DVM_REMOTE_OK);
    first = call(1, function, variable);
    CHECK(dvm_remote_admit(session, &first, &done) == DVM_REMOTE_OK);
    next = first;
    next.payload[1] = 'X';
    CHECK(dvm_remote_admit(session, &next, &done) == DVM_REMOTE_CONFLICT);
    next = first;
    next.ref_count = 1;
    CHECK(dvm_remote_admit(session, &next, &done) == DVM_REMOTE_CONFLICT);
    next = first;
    next.operation++;
    CHECK(dvm_remote_admit(session, &next, &done) == DVM_REMOTE_CONFLICT);
    next = first;
    next.id = 3;
    CHECK(dvm_remote_admit(session, &next, &done) == DVM_REMOTE_CAPACITY);
    CHECK(dvm_remote_finish(session, 1) == DVM_REMOTE_OK);
    CHECK(dvm_remote_admit(session, &first, &done) == DVM_REMOTE_DUPLICATE && done == 1);
    CHECK(dvm_remote_admit(session, &next, &done) == DVM_REMOTE_OK);
    CHECK(dvm_remote_admit(session, &first, &done) == DVM_REMOTE_STALE);
    CHECK(dvm_remote_finish(session, 1) == DVM_REMOTE_STALE);
    first.id = 2;
    CHECK(dvm_remote_admit(session, &first, &done) == DVM_REMOTE_OK);
    CHECK(dvm_remote_finish(session, 2) == DVM_REMOTE_OK);
    CHECK(dvm_remote_finish(session, 3) == DVM_REMOTE_OK);
    first.id = UINT64_MAX;
    CHECK(dvm_remote_admit(session, &first, &done) == DVM_REMOTE_OK);
    CHECK(dvm_remote_finish(session, UINT64_MAX) == DVM_REMOTE_OK);
    first.id = 4;
    CHECK(dvm_remote_admit(session, &first, &done) == DVM_REMOTE_STALE);
    dvm_remote_session_close(session);
    CHECK(dvm_remote_collect(session, &root) == DVM_REMOTE_OK);
    CHECK(dvm_remote_collect(session, &root) == DVM_REMOTE_OK);
    CHECK(dvm_remote_session_destroy(session) == DVM_REMOTE_OK);
}


/* EN: Test transactional failed admission, generation reuse, foreign sessions,
 * bounded publication and monotonic expiration without a real-time sleep.
 * RU: Проверить rollback admission, generation reuse, чужие сессии, capacity
 * и monotonic expiry без real-time sleep. */
static void invalid_and_expiry(void)
{
    int tokens[3] = {0, 1, 2};
    int done = 123;
    DvmRemoteHandle function;
    DvmRemoteHandle old;
    DvmRemoteHandle replacement;
    DvmRemoteRequest request;
    DvmRemoteRoot root;
    DvmRemoteSession *session = dvm_remote_session_new(2, 4, 10);
    DvmRemoteSession *other = dvm_remote_session_new(1, 1, 10);
    CHECK(session != NULL && other != NULL);
    CHECK(dvm_remote_session_new(SIZE_MAX, 1, 10) == NULL);
    CHECK(dvm_remote_session_new(1, SIZE_MAX, 10) == NULL);
    CHECK(dvm_remote_publish(session, DVM_REMOTE_FUNCTION, &tokens[0], &function) == DVM_REMOTE_OK);
    CHECK(dvm_remote_publish(session, DVM_REMOTE_VARIABLE, &tokens[1], &old) == DVM_REMOTE_OK);
    CHECK(dvm_remote_publish(session, DVM_REMOTE_VARIABLE, &tokens[2], &replacement) == DVM_REMOTE_CAPACITY);
    request = call(1, function, old);
    request.refs[1] = function;
    CHECK(dvm_remote_admit(session, &request, &done) == DVM_REMOTE_INVALID && done == 123);
    request = call(1, function, old);
    request.ref_count = DVM_REMOTE_REFS + 1;
    CHECK(dvm_remote_admit(session, &request, &done) == DVM_REMOTE_INVALID);
    request = call(1, function, old);
    request.payload_size = DVM_REMOTE_PAYLOAD + 1;
    CHECK(dvm_remote_admit(session, &request, &done) == DVM_REMOTE_INVALID);
    CHECK(dvm_remote_retain(other, old) == DVM_REMOTE_STALE);
    CHECK(dvm_remote_release(session, old) == DVM_REMOTE_OK);
    CHECK(dvm_remote_collect(session, &root) == DVM_REMOTE_OK && root.token == &tokens[1]);
    CHECK(dvm_remote_publish(session, DVM_REMOTE_VARIABLE, &tokens[2], &replacement) == DVM_REMOTE_OK);
    CHECK(replacement.slot == old.slot && replacement.generation != old.generation);
    request = call(1, function, old);
    CHECK(dvm_remote_admit(session, &request, &done) == DVM_REMOTE_STALE);
    request = call(1, function, replacement);
    CHECK(dvm_remote_admit(session, &request, &done) == DVM_REMOTE_OK);
    CHECK(dvm_remote_session_tick(session, 5, 20) == DVM_REMOTE_OK);
    CHECK(dvm_remote_session_tick(session, 4, 0) == DVM_REMOTE_INVALID);
    CHECK(dvm_remote_session_tick(session, 19, 0) == DVM_REMOTE_OK);
    CHECK(dvm_remote_session_tick(session, 20, 30) == DVM_REMOTE_CLOSED);
    CHECK(dvm_remote_session_tick(session, 21, 40) == DVM_REMOTE_CLOSED);
    CHECK(dvm_remote_collect(session, &root) == DVM_REMOTE_STALE);
    CHECK(dvm_remote_finish(session, 1) == DVM_REMOTE_OK);
    CHECK(dvm_remote_collect(session, &root) == DVM_REMOTE_OK);
    CHECK(dvm_remote_collect(session, &root) == DVM_REMOTE_OK);
    CHECK(dvm_remote_session_destroy(session) == DVM_REMOTE_OK);
    CHECK(dvm_remote_session_destroy(other) == DVM_REMOTE_OK);
}


/* EN: Route Retain/Release effects through the same request gate. A replay after
 * collection/reuse never releases a replacement root or retains the old one.
 * RU: Выполнять Retain/Release через тот же request gate. Replay после collection/
 * reuse не снимает replacement root и не возобновляет старый. */
static void management_replay(void)
{
    int token = 1;
    int done;
    DvmRemoteSession *session = dvm_remote_session_new(1, 4, 100);
    DvmRemoteHandle handle;
    DvmRemoteHandle replacement;
    DvmRemoteRequest request;
    DvmRemoteRoot root;
    CHECK(session != NULL);
    CHECK(dvm_remote_publish(session, DVM_REMOTE_VARIABLE, &token, &handle) == DVM_REMOTE_OK);
    memset(&request, 0, sizeof(request));
    request.id = 1;
    request.target = handle;
    request.kind = DVM_REMOTE_VARIABLE;
    request.operation = 1;
    CHECK(dvm_remote_admit(session, &request, &done) == DVM_REMOTE_OK);
    CHECK(dvm_remote_retain(session, handle) == DVM_REMOTE_OK);
    CHECK(dvm_remote_finish(session, request.id) == DVM_REMOTE_OK);
    CHECK(dvm_remote_admit(session, &request, &done) == DVM_REMOTE_DUPLICATE && done == 1);
    CHECK(dvm_remote_release(session, handle) == DVM_REMOTE_OK);
    CHECK(dvm_remote_collect(session, &root) == DVM_REMOTE_STALE);
    request.id = 2;
    request.operation = 2;
    CHECK(dvm_remote_admit(session, &request, &done) == DVM_REMOTE_OK);
    CHECK(dvm_remote_release(session, handle) == DVM_REMOTE_OK);
    CHECK(dvm_remote_collect(session, &root) == DVM_REMOTE_STALE);
    CHECK(dvm_remote_finish(session, request.id) == DVM_REMOTE_OK);
    CHECK(dvm_remote_collect(session, &root) == DVM_REMOTE_OK && root.token == &token);
    CHECK(dvm_remote_publish(session, DVM_REMOTE_VARIABLE, &token, &replacement) == DVM_REMOTE_OK);
    CHECK(dvm_remote_admit(session, &request, &done) == DVM_REMOTE_DUPLICATE && done == 1);
    request.target = replacement;
    CHECK(dvm_remote_admit(session, &request, &done) == DVM_REMOTE_CONFLICT);
    CHECK(dvm_remote_collect(session, &root) == DVM_REMOTE_STALE);
    request.id = 3;
    CHECK(dvm_remote_admit(session, &request, &done) == DVM_REMOTE_OK);
    CHECK(dvm_remote_release(session, replacement) == DVM_REMOTE_OK);
    CHECK(dvm_remote_finish(session, request.id) == DVM_REMOTE_OK);
    CHECK(dvm_remote_collect(session, &root) == DVM_REMOTE_OK && root.token == &token);
    CHECK(dvm_remote_session_destroy(session) == DVM_REMOTE_OK);
}


/* EN/RU: Run deterministic public-API regressions, fail immediately on mismatch.
 * Выполнить public-API регрессии; завершить при первом несовпадении. No VM boot. */
int main(void)
{
    active_close();
    replay_window();
    invalid_and_expiry();
    management_replay();
    puts("PASS remote session: roots aliases replay close expiry rollback reconnect");
    return 0;
}
