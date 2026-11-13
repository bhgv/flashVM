/* Copyright (c) 2026 bhgv
 * EN: Inject otherwise unreachable native counter/generation limits under lock.
 * RU: Под lock внедрить практически недостижимые пределы counters/generation.
 */
#include "session_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Evaluate once, report exact failed condition and terminate the fixture.
 * Вычислить один раз, сообщить точный failed condition и завершить fixture. */
#define CHECK(condition)                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(condition))                                                                                              \
        {                                                                                                              \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);                                       \
            exit(1);                                                                                                   \
        }                                                                                                              \
    } while (0)


/* EN: Pin overflow with repeated target/ref aliases must refuse transactionally;
 * native test injection is locked and never part of a public runtime operation.
 * RU: Pin overflow с repeated target/ref aliases отказывает транзакционно;
 * внедрение под lock относится только к тесту, не к public runtime operation. */
static void pin_and_reference_limits(void)
{
    int token = 1;
    int done = 123;
    DvmRemoteHandle handle;
    DvmRemoteRequest request;
    DvmRemoteRoot root;
    DvmRemoteSession *session = dvm_remote_session_new(1, 1, 100);
    CHECK(session != NULL);
    CHECK(dvm_remote_publish(session, DVM_REMOTE_VARIABLE, &token, &handle) == DVM_REMOTE_OK);
    memset(&request, 0, sizeof(request));
    request.id = 1;
    request.target = handle;
    request.kind = DVM_REMOTE_VARIABLE;
    request.ref_count = 2;
    request.refs[0] = handle;
    request.refs[1] = handle;
    pthread_mutex_lock(&session->mutex);
    session->objects[0].pins = SIZE_MAX - 1;
    session->objects[0].references = SIZE_MAX;
    pthread_mutex_unlock(&session->mutex);
    CHECK(dvm_remote_retain(session, handle) == DVM_REMOTE_CAPACITY);
    CHECK(dvm_remote_admit(session, &request, &done) == DVM_REMOTE_CAPACITY && done == 123);
    pthread_mutex_lock(&session->mutex);
    CHECK(session->objects[0].pins == SIZE_MAX - 1);
    CHECK(session->objects[0].references == SIZE_MAX);
    CHECK(session->requests[0].request.id == 0 && session->highest == 0);
    session->objects[0].pins = 0;
    session->objects[0].references = 1;
    pthread_mutex_unlock(&session->mutex);
    CHECK(dvm_remote_admit(session, &request, &done) == DVM_REMOTE_OK);
    CHECK(dvm_remote_finish(session, 1) == DVM_REMOTE_OK);
    dvm_remote_session_close(session);
    CHECK(dvm_remote_collect(session, &root) == DVM_REMOTE_OK && root.token == &token);
    CHECK(dvm_remote_session_destroy(session) == DVM_REMOTE_OK);
}


/* EN: An exhausted generation is permanently retired; failed publish preserves
 * root ownership and caller's output. Expiration wins even over invalid renewal.
 * RU: Исчерпанная generation навсегда retired; failed publish сохраняет root
 * ownership и output caller. Expiry действует даже при invalid renewal. */
static void exhausted_generation(void)
{
    int token = 1;
    DvmRemoteHandle output = {11, 22, 33};
    DvmRemoteSession *session = dvm_remote_session_new(1, 1, 10);
    CHECK(session != NULL);
    pthread_mutex_lock(&session->mutex);
    session->objects[0].generation = UINT64_MAX;
    pthread_mutex_unlock(&session->mutex);
    CHECK(dvm_remote_publish(session, DVM_REMOTE_VARIABLE, &token, &output) == DVM_REMOTE_CAPACITY);
    CHECK(output.session == 11 && output.generation == 22 && output.slot == 33);
    CHECK(dvm_remote_session_tick(session, 10, 9) == DVM_REMOTE_CLOSED);
    CHECK(dvm_remote_session_tick(session, 11, 20) == DVM_REMOTE_CLOSED);
    CHECK(dvm_remote_session_destroy(session) == DVM_REMOTE_OK);
}


/* EN/RU: Run fault-injection limits without relaxing any production checks.
 * Выполнить fault-injection limits без ослабления production checks. */
int main(void)
{
    pin_and_reference_limits();
    exhausted_generation();
    puts("PASS remote limits: alias pin overflow rollback reference limit generation exhaustion");
    return 0;
}
