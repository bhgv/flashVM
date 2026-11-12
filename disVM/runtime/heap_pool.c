/* Copyright (c) 2026 bhgv
 * EN: Native arena allocation, stable block boundaries and bounded metadata search.
 * RU: Выделение native-арены, стабильные границы блоков и поиск под замком метаданных.
 */
#include "lib9.h"
#include "pool.h"
#include "heap_pool.h"
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static DVM_HeapGeneration last_generation; /* Process-wide, under the port registry lock. */

typedef char
    DVM_HeapGenerationMustBeUnsigned[((DVM_HeapGeneration)-1 > 0) ? 1 : -1]; /* C99 compile-time unsigned check. */


/* [GC:] EN: Reserve a nonzero process-wide generation under the native registry
 * lock. No arguments/heap access; return zero on exhaustion, never wrap or reuse.
 * RU: Выделить ненулевое поколение процесса под native-замком реестра. Аргументов
 * и доступа к куче нет; ноль — исчерпание, переполнение/повтор запрещены. */
static DVM_HeapGeneration next_generation(void)
{
    fvm_port_registry_lock();
    DVM_HeapGeneration result = 0;
    if (last_generation != (DVM_HeapGeneration)-1)
    {
        result = ++last_generation;
    }
    fvm_port_registry_unlock();
    return result;
}


/* [GC:] EN: Locate private generation storage in borrowed valid block. Caller
 * holds its pool mutex; return address only, no read or ownership transfer.
 * RU: Найти память поколения в заимствованном корректном block. Вызывающий
 * держит pool-мьютекс; вернуть адрес без чтения и передачи владения. */
static void *generation_slot(Bhdr *block)
{
    return (uchar *)B2T(block) - sizeof(DVM_HeapGeneration);
}


/* [GC:] EN: Return native per-block overhead including generation/tail storage.
 * No arguments, shared mutation or managed/serialized layout changes.
 * RU: Вернуть накладной native-размер блока с поколением/хвостом. Аргументов,
 * общей мутации и изменения управляемого/сериализованного формата нет. */
static ulong overhead(void)
{
    return (ulong)offsetof(Bhdr, u.data) + sizeof(Btail) + sizeof(DVM_HeapGeneration);
}


/* [GC:] EN: Validate block geometry/magic within borrowed [block,end) under
 * pool exclusion. Both pointers lie in owned arena; return validity, no callbacks.
 * RU: Проверить размер/magic block внутри [block,end) под pool-замком. Адреса
 * принадлежат арене; вернуть корректность метаданных без callback. */
static int valid_block(Bhdr *block, uchar *end)
{
    ulong remaining = (ulong)(end - (uchar *)block);
    return remaining >= offsetof(Bhdr, u.data) && block->size >= overhead() && block->size <= remaining &&
           !(block->size & 15UL) && (block->magic == MAGIC_F || block->magic == MAGIC_A || block->magic == MAGIC_I);
}


/* [GC:] EN: Report borrowed value/reason only outside metadata exclusion.
 * Hook may terminate, raise or reenter; returning never authorizes bad access.
 * No result or ownership transfer; caller retains its failure outcome.
 * RU: Сообщить value/reason только вне замка метаданных. Hook может завершить,
 * вызвать ошибку или войти повторно; возврат не разрешает неверный доступ.
 * Результата/передачи владения нет; вызывающий сохраняет отказ. */
static void fault(void *value, char *reason)
{
    if (poolfault)
    {
        poolfault(value, reason, 0);
    }
}


/* [GC:] EN: Initialize a zeroed private pool with checked bytes before users
 * exist. Return zero unchanged on invalid extent/allocation failure. Success
 * transfers arena/mutex ownership to pool and publishes the first search cursor.
 * RU: Инициализировать нулевой личный pool размером bytes до появления потоков.
 * Ноль — неверный размер/отказ выделения без изменений. Успех передаёт pool
 * владение ареной/мьютексом и задаёт первый курсор поиска. */
int dvm_heap_pool_init(Pool *pool, ulong bytes)
{
    ulong header = (sizeof(Bhdr) + 15UL) & ~15UL;
    Bhdr *arena;
    Bhdr *block;
    FVM_PortSync *sync;
    if (!pool || pool->arena || pool->sync || pool->bytes || pool->cursor || bytes > INT_MAX || (bytes & 15UL) ||
        bytes < header + sizeof(Bhdr) + 16UL)
    {
        return 0;
    }
    arena = calloc(1, bytes);
    if (!arena)
    {
        return 0;
    }
    sync = fvm_port_sync_create();
    if (!sync)
    {
        free(arena);
        return 0;
    }
    arena->magic = MAGIC_E;
    arena->size = header;
    arena->csize = (int)bytes;
    arena->clink = nil;
    block = B2NB(arena);
    block->magic = MAGIC_F;
    block->size = bytes - header;
    pool->arena = arena;
    pool->bytes = bytes;
    pool->sync = sync;
    pool->cursor = block;
    return 1;
}


/* EN: Search the borrowed block interval [start,end) under pool exclusion.
 * need is the checked aligned size; corrupt receives invalid geometry. No
 * allocation/callback occurs. Return a suitable free header or NULL.
 * RU: Искать в заимствованном интервале [start,end) под pool-замком. need —
 * проверенный выровненный размер; corrupt отмечает неверную геометрию.
 * Выделения/callback нет. Вернуть подходящий свободный заголовок либо NULL. */
static Bhdr *find_free(Bhdr *start, uchar *end, ulong need, int *corrupt)
{
    Bhdr *block;
    for (block = start; (uchar *)block < end; block = B2NB(block))
    {
        if (!valid_block(block, end))
        {
            *corrupt = 1;
            return nil;
        }
        if (block->magic == MAGIC_F && block->size >= need)
        {
            return block;
        }
    }
    return nil;
}


/* EN: Search from the protected cursor, then wrap once through the prefix.
 * pool is borrowed/live; size is payload bytes, report_fault permits diagnostics
 * after unlock. Preserve block addresses and return NULL on refusal/corruption.
 * No managed initialization/destruction or callback runs under the mutex.
 * RU: Искать от защищённого курсора, затем один раз обойти префикс. pool живёт
 * весь вызов; size — размер данных, report_fault разрешает диагностику после
 * unlock. Сохранять адреса блоков; NULL — отказ/повреждение. Под мьютексом нет
 * управляемой инициализации/удаления и callback. */
static void *allocate(Pool *pool, ulong size, int report_fault)
{
    Bhdr *block;
    Bhdr *remainder;
    uchar *end;
    ulong need;
    void *result = nil;
    int corrupt = 0;
    if (!pool || !pool->sync || size > pool->bytes - overhead() - 16UL)
    {
        return nil;
    }
    DVM_HeapGeneration generation = next_generation();
    if (!generation)
    {
        return nil;
    }
    need = (size + overhead() + 15UL) & ~15UL;
    fvm_port_sync_lock(pool->sync);
    end = (uchar *)pool->arena + pool->bytes;
    Bhdr *start = pool->cursor;
    block = find_free(start, end, need, &corrupt);
    if (!block && !corrupt && start != B2NB(pool->arena))
    {
        block = find_free(B2NB(pool->arena), (uchar *)start, need, &corrupt);
    }
    if (block)
    {
        if (block->size - need >= sizeof(Bhdr) + 16UL)
        {
            remainder = (Bhdr *)((uchar *)block + need);
            remainder->magic = MAGIC_F;
            remainder->size = block->size - need;
            block->size = need;
        }
        memcpy(generation_slot(block), &generation, sizeof(generation));
        block->magic = MAGIC_A;
        result = B2D(block);
        pool->cursor = B2NB(block);
        if ((uchar *)pool->cursor == end)
        {
            pool->cursor = B2NB(pool->arena);
        }
    }
    fvm_port_sync_unlock(pool->sync);
    if (corrupt && report_fault)
    {
        fault(pool, "alloc:block");
    }
    return result;
}


/* [GC:] EN: Allocate size payload bytes from borrowed live pool. Return owned
 * native storage or NULL; corruption hook runs after unlock, never under mutex.
 * RU: Выделить size байт из живого pool. Вернуть native-память во владение либо
 * NULL; hook повреждения вызывается после unlock, а не под мьютексом. */
void *poolalloc(Pool *pool, ulong size)
{
    return allocate(pool, size, 1);
}


/* [GC:] EN: Try size bytes from borrowed pool without VM/fault callbacks.
 * Return owned native storage or NULL unchanged on refusal/corruption.
 * RU: Пробовать size байт из pool без callback VM/ошибки. Вернуть native-память
 * во владение либо NULL при отказе/повреждении без разрешения плохого доступа. */
void *dvm_heap_pool_try_alloc(Pool *pool, ulong size)
{
    return allocate(pool, size, 0);
}


/* [GC:] EN: Check numeric value range/alignment before reading its candidate
 * header in live pool. Caller holds mutex and independently owns allocation;
 * return valid live block or NULL. This is no lifetime/provenance acquisition.
 * RU: Проверить диапазон/выравнивание value до чтения заголовка в pool. Вызывающий
 * держит мьютекс и отдельно владеет выделением; вернуть живой блок либо NULL.
 * Проверка не приобретает жизнь объекта и не подтверждает происхождение ссылки. */
static Bhdr *owned_block(Pool *pool, void *value)
{
    uintptr_t address = (uintptr_t)value;
    uintptr_t first = (uintptr_t)B2NB(pool->arena) + offsetof(Bhdr, u.data);
    uintptr_t limit = (uintptr_t)pool->arena + pool->bytes;
    Bhdr *block;
    if (address < first || address > limit - overhead() ||
        ((address - (uintptr_t)pool->arena - offsetof(Bhdr, u.data)) & 15UL) != 0)
    {
        return nil;
    }
    block = (Bhdr *)(address - offsetof(Bhdr, u.data));
    if (!valid_block(block, (uchar *)limit) || (block->magic != MAGIC_A && block->magic != MAGIC_I))
    {
        return nil;
    }
    return block;
}


/* [GC:] EN: Consume caller-owned value in live pool and publish it as the next
 * search start under mutex. Invalid input faults after unlock without changing
 * metadata. Payload destructors never run here; no result or implicit retain.
 * RU: Снять собственное выделение value в pool и сделать его началом поиска
 * под мьютексом. Неверный ввод даёт fault после unlock без изменения метаданных.
 * Деструкторы данных здесь не вызываются; результата и скрытого retain нет. */
void poolfree(Pool *pool, void *value)
{
    Bhdr *block;
    if (!pool || !pool->sync)
    {
        fault(value, "alloc:D2B");
        return;
    }
    fvm_port_sync_lock(pool->sync);
    block = owned_block(pool, value);
    if (block)
    {
        DVM_HeapGeneration invalid = 0;
        memcpy(generation_slot(block), &invalid, sizeof(invalid));
        block->magic = MAGIC_F;
        pool->cursor = block;
    }
    fvm_port_sync_unlock(pool->sync);
    if (!block)
    {
        fault(value, "alloc:D2B");
    }
}


/* [GC:] EN: Observe generation of independently live value in borrowed pool
 * under mutex. Return zero for invalid/freed storage; never retain or dereference
 * payload. Caller must exclude concurrent free while using the result.
 * RU: Прочитать поколение отдельно живого value в pool под мьютексом. Ноль —
 * неверная/свободная память; retain и чтения данных нет. Вызывающий исключает
 * одновременное удаление при использовании результата. */
DVM_HeapGeneration dvm_heap_pool_generation(Pool *pool, void *value)
{
    DVM_HeapGeneration generation = 0;
    if (pool && pool->sync)
    {
        fvm_port_sync_lock(pool->sync);
        Bhdr *block = owned_block(pool, value);
        if (block)
        {
            memcpy(&generation, generation_slot(block), sizeof(generation));
        }
        fvm_port_sync_unlock(pool->sync);
    }
    return generation;
}


/* [GC:] EN: Return metadata identity for independently owned value in pool,
 * or a zero key if invalid. No lifetime extension; generation query holds mutex.
 * RU: Вернуть метаданные идентичности собственного value в pool либо нулевой
 * ключ при неверном вводе. Жизнь не продлевается; поколение читается под мьютексом. */
DVM_HeapKey dvm_heap_pool_key(Pool *pool, void *value)
{
    DVM_HeapGeneration generation = dvm_heap_pool_generation(pool, value);
    return (DVM_HeapKey){generation ? (uintptr_t)value : 0, generation};
}


/* [GC:] EN: Compare borrowed key against independently live value in pool.
 * Return boolean without retaining; a stale key grants no dereference rights.
 * RU: Сравнить заимствованный key с отдельно живым value в pool. Вернуть bool
 * без retain; устаревший ключ не разрешает доступ к объекту. */
int dvm_heap_pool_key_matches(Pool *pool, void *value, DVM_HeapKey key)
{
    return key.generation && key.address == (uintptr_t)value && key.generation == dvm_heap_pool_generation(pool, value);
}


/* [GC:] EN: Borrow live pool's stable arena header without locking. Raw walking
 * requires independent quiescence/exclusion; no ownership or concurrent payload
 * permission is granted. Header/block addresses remain stable across next-fit.
 * RU: Заимствовать стабильный заголовок арены pool без захвата. Сырой обход
 * требует отдельного покоя/исключения; владение и конкурентный доступ к данным
 * не даются. Адреса заголовка/блоков стабильны при поиске от курсора. */
Bhdr *poolchain(Pool *pool)
{
    return pool->arena;
}


/* [GC:] EN: Read value allocation size from live pool under metadata mutex.
 * Caller independently owns storage; no payload pin/COW permission. Invalid
 * input reports after unlock and returns zero; no allocation or owner transfer.
 * RU: Прочитать размер выделения value в pool под мьютексом. Вызывающий отдельно
 * владеет памятью; доступа pin/COW не даётся. Неверный ввод сообщает ошибку после
 * unlock и возвращает ноль; выделения и передачи владения нет. */
ulong poolmsize(Pool *pool, void *value)
{
    Bhdr *block;
    ulong result = 0;
    if (!pool || !pool->sync)
    {
        fault(value, "alloc:D2B");
        return 0;
    }
    fvm_port_sync_lock(pool->sync);
    block = owned_block(pool, value);
    if (block)
    {
        result = block->size - overhead();
    }
    fvm_port_sync_unlock(pool->sync);
    if (!block)
    {
        fault(value, "alloc:D2B");
    }
    return result;
}


/* [GC:] EN: Destroy an independently quiescent pool only when every block is
 * valid/free. Return zero unchanged on refusal. On success clear all fields
 * under mutex and destroy native resources after unlock; no concurrent close.
 * RU: Удалить отдельно успокоенный pool, только если все блоки корректны/свободны.
 * Ноль — отказ без изменений. При успехе снять поля под мьютексом и удалить
 * native-ресурсы после unlock; конкурентное закрытие не допускается. */
int dvm_heap_pool_destroy(Pool *pool)
{
    Bhdr *block;
    Bhdr *arena;
    FVM_PortSync *sync;
    uchar *end;
    if (!pool || !pool->sync)
    {
        return 0;
    }
    sync = pool->sync;
    fvm_port_sync_lock(sync);
    end = (uchar *)pool->arena + pool->bytes;
    for (block = B2NB(pool->arena); (uchar *)block < end; block = B2NB(block))
    {
        if (!valid_block(block, end) || block->magic != MAGIC_F)
        {
            fvm_port_sync_unlock(sync);
            return 0;
        }
    }
    arena = pool->arena;
    pool->arena = nil;
    pool->sync = nil;
    pool->bytes = 0;
    pool->cursor = nil;
    fvm_port_sync_unlock(sync);
    fvm_port_sync_destroy(sync);
    free(arena);
    return 1;
}
