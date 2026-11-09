/* Copyright (c) 2026 bhgv
 * EN: Publish prepared process roots and expose a stable traversal head.
 * RU: Публикация подготовленных корней процессов и получение головы списка.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "../../runtime_gc.h"
#include <pthread.h>

static pthread_mutex_t roots_lock = PTHREAD_MUTEX_INITIALIZER; /* Serializes link writers. */
static Prog *root_head; /* Atomic publication head, including startup residents. */


/* EN: Publish a fully initialized process exactly once. The creator keeps the
 * source roots until the registrar acknowledges publication.
 * RU: Однократно зарегистрировать готовый процесс. Создатель держит исходные
 * корни до подтверждения публикации регистратором. */
void dvm_runtime_process_root_add(Prog *process)
{
    pthread_mutex_lock(&roots_lock);
    process->next = __atomic_load_n(&root_head, __ATOMIC_RELAXED);
    __atomic_store_n(&root_head, process, __ATOMIC_RELEASE);
    pthread_mutex_unlock(&roots_lock);
}


/* EN: Borrow the current list head. Traversal requires the heap turn or a
 * collector stop to keep existing links and process storage alive.
 * RU: Получить голову списка. Обход требует допуска или остановки для GC,
 * чтобы сохранить существующие ссылки и память процессов. */
Prog *dvm_runtime_process_roots(void)
{
    return __atomic_load_n(&root_head, __ATOMIC_ACQUIRE);
}


/* EN: Withdraw process under the heap turn before destroying its roots.
 * RU: Снять процесс под допуском к куче до уничтожения его корней. */
int dvm_runtime_process_root_remove(Prog *process)
{
    pthread_mutex_lock(&roots_lock);
    Prog *head = __atomic_load_n(&root_head, __ATOMIC_RELAXED);
    if (head == process)
    {
        __atomic_store_n(&root_head, process->next, __ATOMIC_RELEASE);
        pthread_mutex_unlock(&roots_lock);
        return 1;
    }
    Prog *previous = head;
    while (previous && previous->next != process)
    {
        previous = previous->next;
    }
    if (previous)
    {
        previous->next = process->next;
    }
    pthread_mutex_unlock(&roots_lock);
    return previous != NULL;
}
