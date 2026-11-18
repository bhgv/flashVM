/* Copyright (c) 2026 bhgv
 * EN: Cancellable native TX-capacity waiting and atomic single-record queue admission.
 * RU: Прерываемое native-ожидание TX capacity и атомарное принятие одной S-записи.
 */
#include "internal.h"
#include "cancel.h"


/* EN/RU: Subscription-owned cancellation gate; detach control before deferred callbacks may outlive call.
 * Gate подписки: отделить control до возврата, callbacks могут завершаться позднее.
 */
typedef struct SendWait
{
    pthread_mutex_t lock; /* Order: gate lock before control lock; never reversed. */
    StyxBidirControl *control; /* Borrowed while attached; NULL after final detach. */
    int cancelled; /* Protected by control lock while attached. */
} SendWait;


/* EN/RU: Native cancellation wake only, no transport/guest work; state pinned by the subscription.
 * Только native wake отмены, без transport/guest работы; state удерживает подписка.
 */
static void cancelled(void *state)
{
    SendWait *wait = state; /* Subscription-owned native context. */
    pthread_mutex_lock(&wait->lock);
    if (wait->control)
    {
        pthread_mutex_lock(&wait->control->lock);
        wait->cancelled = 1;
        pthread_cond_broadcast(&wait->control->changed);
        pthread_mutex_unlock(&wait->control->lock);
    }
    pthread_mutex_unlock(&wait->lock);
}


/* EN/RU: Release context after all admitted callbacks; no remaining control access.
 * Освободить context после всех callbacks, доступа к control уже нет.
 */
static void release(void *state)
{
    SendWait *wait = state; /* Final context owner. */
    pthread_mutex_destroy(&wait->lock);
    free(wait);
}


/* EN/RU: See control.h; validate/encode before waiting, then publish exactly once under queue mutex.
 * См. control.h; кодирование до ожидания, публикация ровно один раз под queue mutex.
 */
int styxbidircontrolsendwait(StyxBidirControl *control, unsigned long tag, const void *payload,
    size_t size, unsigned timeout, DVM_Cancel *cancel)
{
    if (!control || (!payload && size) || size >= control->limit || timeout > 120000)
    {
        return 0;
    }
    struct timespec deadline; /* Original monotonic capacity-wait deadline. */
    if (clock_gettime(CLOCK_MONOTONIC, &deadline))
    {
        return 0;
    }
    deadline.tv_sec += timeout / 1000;
    deadline.tv_nsec += (long)(timeout % 1000) * 1000000L;
    if (deadline.tv_nsec >= 1000000000L)
    {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }
    unsigned char *encoded = malloc(control->extent); /* Private immutable frame before queue admission. */
    size_t encoded_size; /* Complete checked frame extent. */
    size_t consumed; /* Exact decoder verification extent. */
    FVM_SRecord record; /* Borrowed validation view, never published to a foreign owner. */
    if (!encoded)
    {
        return 0;
    }
    if (fvm_srecord_write(tag, payload, size, encoded, control->extent, &encoded_size) != FVM_S_OK ||
        fvm_srecord_read(encoded, encoded_size, control->limit, &record, &consumed) != FVM_S_OK || consumed != encoded_size)
    {
        free(encoded);
        return 0;
    }
    SendWait *wait = NULL; /* Optional callback gate adopted by a tap only on success. */
    DVM_CancelTap *tap = NULL; /* Source-pinning cancellation subscription. */
    if (cancel)
    {
        wait = calloc(1, sizeof(*wait));
        if (!wait || pthread_mutex_init(&wait->lock, NULL))
        {
            free(wait);
            free(encoded);
            return 0;
        }
        wait->control = control;
        tap = dvm_cancel_subscribe(cancel, cancelled, wait, release);
        if (!tap)
        {
            release(wait);
            free(encoded);
            return 0;
        }
    }
    pthread_mutex_lock(&control->lock);
    int error = 0; /* Condition timeout/failure never admits a frame after the wait fails. */
    while (control->outgoing.count == control->slots && !control->reason &&
        !(wait && wait->cancelled) && timeout && !error)
    {
        error = pthread_cond_timedwait(&control->changed, &control->lock, &deadline);
    }
    int accepted = !error && !control->reason && !(wait && wait->cancelled) &&
        control->outgoing.count < control->slots; /* Linearized one-record acceptance. */
    if (accepted)
    {
        unsigned slot = (control->outgoing.first + control->outgoing.count) % control->slots; /* Protected tail. */
        memcpy(control->outgoing.bytes + (size_t)slot * control->extent, encoded, encoded_size);
        control->outgoing.sizes[slot] = encoded_size;
        control->outgoing.count++;
        pthread_cond_broadcast(&control->changed);
    }
    pthread_mutex_unlock(&control->lock);
    if (wait)
    {
        pthread_mutex_lock(&wait->lock);
        wait->control = NULL;
        pthread_mutex_unlock(&wait->lock);
        dvm_cancel_unsubscribe(tap);
    }
    free(encoded);
    return accepted;
}
