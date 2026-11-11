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


/* EN: Borrow the current list head. Traversal requires a collector stop or the external registry
 * lifetime lock; concurrent GC admission alone does not keep links/storage alive.
 * RU: Получить голову списка. Обход требует остановки для GC либо внешней
 * блокировки реестра; один допуск GC не сохраняет ссылки и память процессов. */
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


/* [GC:] EN: Test membership and acyclicity while root links/storage are protected.
 * process is a borrowed identity retained independently by caller. Return boolean;
 * no node pointer escapes, ownership changes, callbacks or GC waits occur locked.
 * RU: Проверить принадлежность и отсутствие цикла под защитой ссылок/памяти.
 * process — заимствованный идентификатор, отдельно удерживаемый вызывающим.
 * Вернуть bool; под mutex нет передачи узлов, владения, callbacks или ожидания GC. */
int dvm_runtime_process_root_contains(Prog *process)
{
    if (!process)
    {
        return 0;
    }
    pthread_mutex_lock(&roots_lock);
    Prog *head = __atomic_load_n(&root_head, __ATOMIC_RELAXED);
    Prog *slow = head;
    Prog *fast = head;
    int found = 0;
    int valid = 1;
    for (Prog *cursor = head; cursor; cursor = cursor->next)
    {
        if (cursor == process)
        {
            found = 1;
        }
        if (fast && fast->next)
        {
            slow = slow->next;
            fast = fast->next->next;
            if (slow == fast)
            {
                valid = 0;
                break;
            }
        }
        else
        {
            fast = NULL;
        }
    }
    pthread_mutex_unlock(&roots_lock);
    return found && valid;
}
