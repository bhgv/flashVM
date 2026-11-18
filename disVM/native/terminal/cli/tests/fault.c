/* Copyright (c) 2026 bhgv
 * EN: Post-connection CLI setup faults, preserving actual native owners, stdio restoration and joined cleanup.
 * RU: Отказы setup CLI после соединения, с настоящими owners, восстановлением stdio и join-cleanup.
 */
#define _POSIX_C_SOURCE 200809L
#include "../session.h"
#include "cancel.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
static __thread const char *scenario; /* Fixture-only immutable fault selector, never read by production CLI. */
static __thread unsigned calls; /* Synchronous CLI session worker creation count. */
static __thread int fail_allocation; /* Refuse next session calloc only, then restore real allocator. */


/* EN/RU: Original native session runner, no replacement I/O path.
 * Настоящий native-runner, без подмены пути I/O.
 */
extern int __real_unibuscli_run(StyxOrdered *, const UnibusCliConfig *);


/* EN/RU: Arm faults only after all connection/broker workers are already owned, then run the production session.
 * Отмечать отказы лишь после владения connection/broker workers, затем выполнять production-session.
 */
int __wrap_unibuscli_run(StyxOrdered *peer, const UnibusCliConfig *config)
{
    scenario = getenv("UNIBUS_CLI_TEST_FAULT");
    fail_allocation = scenario && !strcmp(scenario, "allocation");
    return __real_unibuscli_run(peer, config);
}


/* EN/RU: Original zeroed allocator, caller owns returned allocation or NULL.
 * Исходный обнулённый allocator, caller владеет результатом либо получает NULL.
 */
extern void *__real_calloc(size_t, size_t);


/* EN/RU: Reject one session allocation before publication, leaving other native worker allocations untouched.
 * Отказать одному выделению session до публикации, не меняя выделения остальных native-workers.
 */
void *__wrap_calloc(size_t count, size_t size)
{
    if (fail_allocation)
    {
        fail_allocation = 0;
        return NULL;
    }
    return __real_calloc(count, size);
}


/* EN/RU: Original native thread creation, stable argument until its owner joins.
 * Исходное создание native-потока, argument стабилен до join владельцем.
 */
extern int __real_pthread_create(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);


/* EN/RU: Reject selected signal/reader/input startup after earlier resources exist, without inventing a thread handle.
 * Отказать выбранному signal/reader/input-startup после создания ресурсов, не создавая фиктивный thread-handle.
 */
int __wrap_pthread_create(pthread_t *thread, const pthread_attr_t *attributes, void *(*entry)(void *), void *argument)
{
    if (scenario && scenario[0] >= '1' && scenario[0] <= '5' && !scenario[1] && ++calls == (unsigned)(scenario[0] - '0'))
    {
        return EAGAIN;
    }
    return __real_pthread_create(thread, attributes, entry, argument);
}


/* EN/RU: Original sticky broker subscription, state adopted only on success.
 * Исходная sticky-подписка брокера, state принят лишь при успехе.
 */
extern DVM_CancelTap *__real_styxorderedwatch(StyxOrdered *, void (*)(void *), void *, void (*)(void *));


/* EN/RU: Refuse CLI lifecycle subscription without adopting its fence; broker construction remains production code.
 * Отказать CLI-подписке без принятия fence; создание брокера остаётся production-кодом.
 */
DVM_CancelTap *__wrap_styxorderedwatch(StyxOrdered *peer, void (*receive)(void *), void *state, void (*release)(void *))
{
    if (scenario && !strcmp(scenario, "watch"))
    {
        return NULL;
    }
    return __real_styxorderedwatch(peer, receive, state, release);
}
