/* Copyright (c) 2026 bhgv
 *
 * EN: Synchronized root occurrences and reusable registry entries.
 * RU: Синхронизированный реестр корней и переиспользуемые записи.
 */
#include "root_registry.h"
#include "root_registry_private.h"
#include "channel_sync.h"
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>

#define ROOT_BUCKETS 4096 /* Power-of-two native hash buckets; not an object quota. */
#if !defined(DVM_ROOT_FORCE_LOCKED) && defined(__GCC_ATOMIC_POINTER_LOCK_FREE) && __GCC_ATOMIC_POINTER_LOCK_FREE == 2
#define ROOT_ATOMIC 1     /* Acquire/release publication; otherwise use port registry mutex. */
#endif


static RootEntry *buckets[ROOT_BUCKETS]; /* Protected by root mutex. */
static RootEntry *unused;                /* Reusable metadata; no live referent pointer retained. */
static FVM_PortSync *root_sync;          /* Published once, stable through process exit. */


/* [GC:] EN: Borrow the process-lifetime mutex without acquiring it; no arguments,
 * allocation or referent access. Return NULL before first successful publication.
 * RU: Заимствовать мьютекс процесса без захвата; нет аргументов, выделения и доступа
 * к объектам. Вернуть NULL до первой успешной публикации. */
static FVM_PortSync *published(void)
{
    FVM_PortSync *result;
#ifdef ROOT_ATOMIC
    result = __atomic_load_n(&root_sync, __ATOMIC_ACQUIRE);
#else
    fvm_port_registry_lock();
    result = root_sync;
    fvm_port_registry_unlock();
#endif
    return result;
}


/* [GC:] EN: Borrow/create the process-lifetime mutex for publication. Allocate and
 * destroy losing candidates outside the publication lock; return NULL on failure
 * unless another thread already published a usable mutex. No root table mutation.
 * RU: Заимствовать/создать мьютекс публикации. Выделение и удаление проигравшего
 * кандидата выполняются вне замка публикации. При отказе вернуть NULL, если другой
 * поток ещё не опубликовал мьютекс. Таблица корней не меняется. */
static FVM_PortSync *create_sync(void)
{
    FVM_PortSync *result = published();
    FVM_PortSync *candidate;
    if (result)
    {
        return result;
    }
    candidate = fvm_port_sync_create();
    fvm_port_registry_lock();
#ifdef ROOT_ATOMIC
    result = __atomic_load_n(&root_sync, __ATOMIC_RELAXED);
#else
    result = root_sync;
#endif
    if (!result && candidate)
    {
        result = candidate;
#ifdef ROOT_ATOMIC
        __atomic_store_n(&root_sync, candidate, __ATOMIC_RELEASE);
#else
        root_sync = candidate;
#endif
        candidate = NULL;
    }
    fvm_port_registry_unlock();
    if (candidate)
    {
        fvm_port_sync_destroy(candidate);
    }
    return result;
}


/* [GC:] EN: Mix opaque key address bits without dereferencing the referent.
 * Return a stable bucket index; callers hold the root mutex for table access.
 * Aligned/strided heaps must not collapse into one long metadata-lock scan.
 * RU: Смешать биты адреса key без доступа к объекту. Вернуть стабильный индекс;
 * доступ к таблице требует root-мьютекс. Выровненные адреса с постоянным шагом
 * не должны образовывать одну длинную цепочку под замком метаданных. */
static unsigned int bucket(const void *key)
{
    uintptr_t hash = (uintptr_t)key;
#if UINTPTR_MAX > UINT32_MAX
    hash ^= hash >> 33;
    hash *= (uintptr_t)UINT64_C(0xff51afd7ed558ccd);
    hash ^= hash >> 33;
    hash *= (uintptr_t)UINT64_C(0xc4ceb9fe1a85ec53);
    hash ^= hash >> 33;
#else
    hash ^= hash >> 16;
    hash *= (uintptr_t)UINT32_C(0x7feb352d);
    hash ^= hash >> 15;
    hash *= (uintptr_t)UINT32_C(0x846ca68b);
    hash ^= hash >> 16;
#endif
    return (unsigned int)(hash & (ROOT_BUCKETS - 1));
}


/* [GC:] EN: Register one occurrence of borrowed nonnull key. Caller owns referent
 * lifetime/publication. Return zero unchanged on null/allocation failure; allocate
 * native entries outside root exclusion and initialize fully before linking.
 * RU: Зарегистрировать вхождение заимствованного ненулевого key. Вызывающий отвечает
 * за жизнь/публикацию объекта. Ноль — NULL/отказ выделения без изменения таблицы;
 * выделять записи вне root-замка, полностью инициализировать до включения. */
int dvm_runtime_root_add(void *key)
{
    FVM_PortSync *mutex;
    RootEntry *entry;
    unsigned int index;
    if (!key)
    {
        return 0;
    }
    mutex = create_sync();
    if (!mutex)
    {
        return 0;
    }
    index = bucket(key);
    fvm_port_sync_lock(mutex);
    entry = unused;
    if (entry)
    {
        unused = entry->next;
    }
    if (!entry)
    {
        fvm_port_sync_unlock(mutex);
        entry = malloc(sizeof(*entry));
        if (!entry)
        {
            return 0;
        }
        fvm_port_sync_lock(mutex);
    }
    entry->batch = NULL;
    entry->key = key;
    entry->next = buckets[index];
    buckets[index] = entry;
    fvm_port_sync_unlock(mutex);
    return 1;
}


/* [GC:] EN: Remove one ordinary key occurrence if optional permit(argument)
 * succeeds under root exclusion. Callback must not block, allocate or reenter.
 * Return zero unchanged when absent/refused; batch entries and referents survive.
 * RU: Снять обычное вхождение key при успехе необязательного permit(argument)
 * под root-замком. Callback не ждёт, не выделяет и не входит повторно. Ноль —
 * отсутствие/отказ без изменений; пакетные записи и объекты не удаляются. */
int dvm_runtime_root_remove_if(const void *key, int (*permit)(void *), void *argument)
{
    FVM_PortSync *mutex = published();
    RootEntry **link;
    RootEntry *entry;
    if (!key || !mutex)
    {
        return 0;
    }
    fvm_port_sync_lock(mutex);
    for (link = &buckets[bucket(key)]; (entry = *link) != NULL; link = &entry->next)
    {
        if (entry->key == key && !entry->batch)
        {
            if (permit && !permit(argument))
            {
                break;
            }
            *link = entry->next;
            entry->key = NULL;
            entry->next = unused;
            unused = entry;
            fvm_port_sync_unlock(mutex);
            return 1;
        }
    }
    fvm_port_sync_unlock(mutex);
    return 0;
}


/* [GC:] EN: Remove one ordinary borrowed key occurrence without a predicate.
 * Return one on removal, zero absent/null; no allocation, callback or dereference.
 * RU: Снять обычное вхождение заимствованного key без предиката. Один — снято,
 * ноль — NULL/отсутствие; выделения, callback и доступа к объекту нет. */
int dvm_runtime_root_remove(const void *key)
{
    return dvm_runtime_root_remove_if(key, NULL, NULL);
}


/* [GC:] EN: Count all occurrences of borrowed opaque key under root exclusion.
 * No ownership/payload access is granted; return zero for absent/null, saturate
 * at ULONG_MAX. No allocation or traversal cursor escapes.
 * RU: Посчитать все вхождения непрозрачного key под root-замком. Это не даёт
 * владения/доступа к данным. Ноль — отсутствие/NULL; насыщать на ULONG_MAX.
 * Выделения и выхода курсора обхода нет. */
unsigned long dvm_runtime_root_count(const void *key)
{
    FVM_PortSync *mutex = published();
    RootEntry *entry;
    unsigned long count = 0;
    if (!key || !mutex)
    {
        return 0;
    }
    fvm_port_sync_lock(mutex);
    for (entry = buckets[bucket(key)]; entry; entry = entry->next)
    {
        if (entry->key == key && count != ULONG_MAX)
        {
            count++;
        }
    }
    fvm_port_sync_unlock(mutex);
    return count;
}


/* [GC:] EN: Visit each stable occurrence with borrowed visitor/argument under
 * root exclusion; NULL visitor is a no-op. Visitor cannot allocate, block,
 * reenter or raise VM errors. Managed callers independently exclude root lifetime
 * changes; this mutex alone grants no payload access. No cursor escapes.
 * RU: Посетить стабильные вхождения через visitor/argument под root-замком;
 * NULL visitor ничего не делает. Callback не выделяет, не ждёт, не входит
 * повторно и не вызывает ошибки VM. Управляемый вызов отдельно защищает жизнь
 * корней; мьютекс не даёт доступа к данным. Курсор не выходит наружу. */
void dvm_runtime_roots_visit(DVM_RootVisitor visitor, void *argument)
{
    FVM_PortSync *mutex = published();
    RootEntry *entry;
    unsigned int index;
    if (!visitor || !mutex)
    {
        return;
    }
    fvm_port_sync_lock(mutex);
    for (index = 0; index < ROOT_BUCKETS; index++)
    {
        for (entry = buckets[index]; entry; entry = entry->next)
        {
            visitor(entry->key, argument);
        }
    }
    fvm_port_sync_unlock(mutex);
}


/* [GC:] EN: Borrow registry mutex for batch bridge; create permits native
 * initialization outside root exclusion. Return NULL when unpublished/refused;
 * process-lifetime metadata grants no referent access or ownership.
 * RU: Заимствовать мьютекс для пакетов; create разрешает native-инициализацию
 * вне root-замка. NULL — отсутствие/отказ. Метаданные процесса не дают доступа
 * к объекту и не передают владение. */
FVM_PortSync *dvm_root_registry_sync(int create)
{
    return create ? create_sync() : published();
}


/* [GC:] EN: Return the bucket head for borrowed opaque key. Caller must hold
 * the registry mutex through every head/link access; storage stays static for
 * process lifetime. No allocation, referent access or ownership proof.
 * RU: Вернуть голову корзины непрозрачного key. Вызывающий держит мьютекс реестра
 * до конца доступа к голове/связям; память статична на весь процесс. Выделения,
 * доступа к объекту и подтверждения владения нет. */
RootEntry **dvm_root_registry_bucket(const void *key)
{
    return &buckets[bucket(key)];
}
