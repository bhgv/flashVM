/* Copyright (c) 2026 bhgv
 * EN: Private source-pinning cancellation fence for one borrowed broker wait.
 * RU: Закрытый fence отмены одного ожидания, удерживающий source и borrowed broker.
 */
#ifndef STYX_ORDERED_WAIT_CANCEL_H
#define STYX_ORDERED_WAIT_CANCEL_H
#include "cancel.h"
#include <pthread.h>


/* EN/RU: Tap owns storage; waiter detaches peer before consuming tap, never a managed address.
 * Tap владеет storage; waiter отделяет peer перед освобождением tap, managed-адресов нет.
 */
typedef struct OrderedWaitCancel
{
    pthread_mutex_t fence;    /* Protects callback access to peer during detach. */
    struct StyxOrdered *peer; /* Borrowed through waiter lifetime, NULL after detach. */
    DVM_CancelTap *tap;       /* Owned source pin, consumed at detach. */
    int cancelled;            /* Sticky predicate protected by peer mutex. */
} OrderedWaitCancel;


/* EN/RU: Borrow peer/source, create fenced subscription1/0; NULL source succeeds with NULL output.
 * Заимствовать peer/source, создать fenced-подписку1/0; NULL source даёт успех и NULL output.
 */
int orderedwaitattach(struct StyxOrdered *peer, DVM_Cancel *source, OrderedWaitCancel **output);


/* EN/RU: Retire callback access before waiter returns; state may outlive waiter until admitted callback finishes.
 * Отделить callback-доступ до возврата waiter; state может жить до завершения допущенного callback.
 */
void orderedwaitdetach(OrderedWaitCancel *wait);
#endif
