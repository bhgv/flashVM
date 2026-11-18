/* Copyright (c) 2026 bhgv
 * EN: Refuse the second staged native message allocation in one actual outer-envelope transaction.
 * RU: Отказать во второй staged native message allocation одной настоящей outer-envelope transaction.
 */
#include "../internal.h"
#include <errno.h>
static __thread unsigned remaining; /* Dispatcher-local fault, never another native worker's allocation. */


/* EN/RU: Original allocator and scope admission, preserving all ordinary ownership contracts.
 * Исходный allocator, обычные ownership contracts сохраняются.
 */
extern void *__real_malloc(size_t);


/* EN/RU: Original whole-envelope admission, hub/input borrowed through return.
 * Исходный whole-envelope admission, hub/input borrowed до return.
 */
extern int __real_scopefeed(ScopeHub *, unsigned long, const void *, size_t);


/* EN/RU: Inject only the armed allocation on this thread; ordinary native library allocations remain real.
 * Отказать лишь armed allocation этого потока; обычные native allocations остаются настоящими.
 */
void *__wrap_malloc(size_t size)
{
    if (remaining && !--remaining)
    {
        return NULL;
    }
    return __real_malloc(size);
}


/* EN/RU: Arm second staged allocation only in case2's first application envelope, restore after admission.
 * Во case2 включить отказ второй staged allocation первого application envelope, снять после admission.
 */
int __wrap_scopefeed(ScopeHub *hub, unsigned long tag, const void *bytes, size_t size)
{
    if (STYX_SCOPE_CASE == 2 && tag == 11)
    {
        remaining = 2;
    }
    int result = __real_scopefeed(hub, tag, bytes, size);
    remaining = 0;
    return result;
}


/* EN/RU: One main-thread constructor fault; static delayed subscription state remains owned until explicit finish.
 * Один main-thread constructor fault; delayed subscription state живёт до explicit finish.
 */
static __thread int armed;                   /* Case7 watch refusal, case9 worker refusal, case10 sticky replay. */
static void (*late_receive)(void *); /* Original native callback, delayed until after rootfree. */
static void (*late_release)(void *); /* Original independent context destructor. */
static void *late_state;            /* Adopted original context, never the root allocation. */
static int admitted;                /* Atomic actual retirement callback marker. */
static int detached;                /* Atomic actual subscription final-release marker. */


/* EN/RU: Original native subscription with state adoption on success only.
 * Исходная native subscription с adoption state лишь при успехе.
 */
extern DVM_CancelTap *__real_styxbidircontrolwatch(StyxBidirControl *, void (*)(void *), void *, void (*)(void *));


/* EN/RU: Original native worker creation, argument ownership unchanged on refusal.
 * Исходное native worker creation, ownership argument не меняется при отказе.
 */
extern int __real_pthread_create(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);


/* EN/RU: Arm after low-level control workers exist, immediately before scoped broker construction.
 * Включить после появления control workers, непосредственно перед scoped broker construction.
 */
void scopefaultarm(void)
{
    armed = STYX_SCOPE_CASE == 12 ? 2 : 1;
}


/* EN/RU: Mark actual source callback admission, retaining original state for an already-admitted late callback.
 * Зафиксировать actual source callback admission, удерживая state для late callback.
 */
static void delayed(void *unused)
{
    (void)unused;
    __atomic_store_n(&admitted, 1, __ATOMIC_RELEASE);
}


/* EN/RU: Mark source disposal without releasing original context until explicit post-free callback finish.
 * Зафиксировать source disposal, сохранив original context до post-free callback finish.
 */
static void delayed_release(void *unused)
{
    (void)unused;
    __atomic_store_n(&detached, 1, __ATOMIC_RELEASE);
}


/* EN/RU: Refuse watch once, inject real sticky stop, or retain original callback through joined root disposal.
 * Один отказ watch, настоящий sticky stop либо удержание callback до joined root disposal.
 */
DVM_CancelTap *__wrap_styxbidircontrolwatch(StyxBidirControl *control, void (*receive)(void *), void *state, void (*release)(void *))
{
    if (armed && STYX_SCOPE_CASE == 7)
    {
        armed = 0;
        return NULL;
    }
    if (armed && STYX_SCOPE_CASE == 10)
    {
        armed = 0;
        styxbidircontrolstop(control);
    }
    if (STYX_SCOPE_CASE == 8)
    {
        late_receive = receive;
        late_release = release;
        late_state = state;
        return __real_styxbidircontrolwatch(control, delayed, NULL, delayed_release);
    }
    return __real_styxbidircontrolwatch(control, receive, state, release);
}


/* EN/RU: Refuse only scoped constructor's first worker in case9; later native operations remain real.
 * Отказать лишь scoped constructor worker во case9, последующие native operations настоящие.
 */
int __wrap_pthread_create(pthread_t *worker, const pthread_attr_t *attributes, void *(*entry)(void *), void *state)
{
    if ((STYX_SCOPE_CASE == 12 && armed && !--armed) || (armed && STYX_SCOPE_CASE == 9))
    {
        armed = 0;
        return EAGAIN;
    }
    return __real_pthread_create(worker, attributes, entry, state);
}


/* EN/RU: After rootfree deliver duplicate admitted notices then release independent context exactly once.
 * После rootfree выдать duplicate admitted notices и один раз освободить independent context.
 */
int scopelate(void)
{
    if (!late_state || !__atomic_load_n(&admitted, __ATOMIC_ACQUIRE) || !__atomic_load_n(&detached, __ATOMIC_ACQUIRE))
    {
        return 0;
    }
    late_receive(late_state);
    late_receive(late_state);
    late_release(late_state);
    late_state = NULL;
    return 1;
}
