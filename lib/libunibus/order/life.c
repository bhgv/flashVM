/* Copyright (c) 2026 bhgv
 * EN: Native per-namespace engine lifetime and explicit first-counter agreement, no owned threads.
 * RU: Жизненный цикл очередей одного пространства имён и явное начальное значение счётчика; без собственных потоков.
 */
#include "internal.h"

/* EN/RU: See internal.h; caller holds registry lock, allocation failure changes no existing tag.
 * См. internal.h; registry lock у caller, allocation failure не меняет existing tag.
 */


/* [standalone-contract:ordertag]
 * EN: ordertag: independent per-tag life state.
 * RU: ordertag: независимое состояние тегов life.
 * order (StyxOrder *order): Native per-tag queue engine; keep it alive until all concurrent calls retire.
 * RU: Нативный механизм очередей тегов; удерживать до завершения параллельных вызовов.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * create (int create): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: OrderTag *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: OrderTag *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
OrderTag *ordertag(StyxOrder *order, unsigned long tag, int create)
{
    OrderTag *entry = order->tags; /* Borrowed registry cursor. */
    while (entry && entry->tag != tag)
    {
        entry = entry->next;
    }
    if (!entry && create)
    {
        entry = calloc(1, sizeof(*entry));
        if (entry)
        {
            entry->tag = tag;
            entry->next = order->tags;
            order->tags = entry;
        }
    }
    return entry;
}

/* EN/RU: See order.h; publish only fully initialized native synchronization and bounds.
 * См. order.h; публиковать лишь полностью initialized native synchronization/bounds.
 */


/* [standalone-contract:styxordercreate]
 * EN: styxordercreate: independent per-tag life state.
 * RU: styxordercreate: независимое состояние тегов life.
 * limit (unsigned limit): Explicit local quota; permitted ranges and wire/body units are given in the description.
 * RU: Явная локальная квота; диапазон и единицы wire/body указаны в описании.
 * window (unsigned long long window): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: StyxOrder *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: StyxOrder *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxOrder *styxordercreate(unsigned limit, unsigned long long window)
{
    if (!limit || limit > FVM_SRECORD_BODY_MAX || !window || window >= (1ULL << 63))
    {
        return NULL;
    }
    StyxOrder *order = calloc(1, sizeof(*order)); /* Private until every native resource initialized. */
    if (!order)
    {
        return NULL;
    }
    if (pthread_mutex_init(&order->lock, NULL))
    {
        free(order);
        return NULL;
    }
    pthread_condattr_t attributes;                         /* Temporary native monotonic wait configuration. */
    int initialized = !pthread_condattr_init(&attributes); /* Attribute disposal guard. */
    int ready = initialized && !pthread_condattr_setclock(&attributes, CLOCK_MONOTONIC) &&
                !pthread_cond_init(&order->changed, &attributes);
    if (initialized)
    {
        pthread_condattr_destroy(&attributes);
    }
    if (!ready)
    {
        pthread_mutex_destroy(&order->lock);
        free(order);
        return NULL;
    }
    order->limit = limit;
    order->window = window;
    return order;
}

/* EN/RU: See order.h; default directions stay independent when the other direction was already seeded.
 * См. order.h; directions независимы, даже если другой direction уже seeded.
 */


/* [standalone-contract:styxorderstart]
 * EN: styxorderstart: independent per-tag life state.
 * RU: styxorderstart: независимое состояние тегов life.
 * order (StyxOrder *order): Native per-tag queue engine; keep it alive until all concurrent calls retire.
 * RU: Нативный механизм очередей тегов; удерживать до завершения параллельных вызовов.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * direction (unsigned direction): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * first (unsigned long long first): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxorderstart(StyxOrder *order, unsigned long tag, unsigned direction, unsigned long long first)
{
    if (!order || tag > FVM_SRECORD_U32 || direction > STYX_ORDER_TX)
    {
        return 0;
    }
    pthread_mutex_lock(&order->lock);
    OrderTag *entry = order->closed ? NULL : ordertag(order, tag, 1); /* Owned registry entry or refusal. */
    int valid = entry && !entry->used[direction] && !entry->pending[direction];
    if (valid)
    {
        entry->used[direction] = 1;
        if (direction == STYX_ORDER_RX)
        {
            entry->expected = first;
        }
        else
        {
            entry->transmit = first;
        }
    }
    pthread_mutex_unlock(&order->lock);
    return valid;
}

/* EN/RU: See order.h; close is idempotent, retained ready packets remain independently drainable.
 * См. order.h; close idempotent, retained ready packets дочитываются независимо.
 */


/* [standalone-contract:styxorderclose]
 * EN: styxorderclose: independent per-tag life state.
 * RU: styxorderclose: независимое состояние тегов life.
 * order (StyxOrder *order): Native per-tag queue engine; keep it alive until all concurrent calls retire.
 * RU: Нативный механизм очередей тегов; удерживать до завершения параллельных вызовов.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxorderclose(StyxOrder *order)
{
    if (order)
    {
        pthread_mutex_lock(&order->lock);
        order->closed = 1;
        pthread_cond_broadcast(&order->changed);
        pthread_mutex_unlock(&order->lock);
    }
}

/* EN/RU: Consume an owned record chain without callbacks or external/native object access.
 * Поглотить owned record chain без callbacks либо external object access.
 */


/* [standalone-contract:clear]
 * EN: clear: independent per-tag life state.
 * RU: clear: независимое состояние тегов life.
 * record (OrderRecord *record): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void clear(OrderRecord *record)
{
    while (record)
    {
        OrderRecord *next = record->next; /* Owned continuation transferred before free. */
        free(record);
        record = next;
    }
}

/* EN/RU: See order.h; all users retire before final queue/registry disposal, no workers to join.
 * См. order.h; все users завершаются до queue/registry disposal, workers нет.
 */


/* [standalone-contract:styxorderfree]
 * EN: styxorderfree: independent per-tag life state.
 * RU: styxorderfree: независимое состояние тегов life.
 * order (StyxOrder *order): Native per-tag queue engine; keep it alive until all concurrent calls retire.
 * RU: Нативный механизм очередей тегов; удерживать до завершения параллельных вызовов.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxorderfree(StyxOrder *order)
{
    if (order)
    {
        styxorderclose(order);
        while (order->tags)
        {
            OrderTag *entry = order->tags; /* Transfer owned registry node. */
            order->tags = entry->next;
            clear(entry->head);
            clear(entry->held);
            free(entry);
        }
        pthread_cond_destroy(&order->changed);
        pthread_mutex_destroy(&order->lock);
        free(order);
    }
}

/* EN/RU: See order.h; tighten the agreed receive window before first RX data, preserving existing TX counters.
 * См. order.h; сузить окно RX до первых данных, сохраняя существующие TX-счётчики.
 */


/* [standalone-contract:styxorderreceivewindow]
 * EN: styxorderreceivewindow: independent per-tag life state.
 * RU: styxorderreceivewindow: независимое состояние тегов life.
 * order (StyxOrder *order): Native per-tag queue engine; keep it alive until all concurrent calls retire.
 * RU: Нативный механизм очередей тегов; удерживать до завершения параллельных вызовов.
 * window (unsigned long long window): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxorderreceivewindow(StyxOrder *order, unsigned long long window)
{
    if (!order || !window || window >= (1ULL << 63))
    {
        return 0;
    }
    pthread_mutex_lock(&order->lock);
    int valid = !order->closed && window <= order->window; /* Never enlarge the current agreed window. */
    if (window != order->window)
    {
        for (OrderTag *tag = order->tags; valid && tag; tag = tag->next)
        {
            valid = !tag->used[STYX_ORDER_RX];
        }
    }
    if (valid)
    {
        order->window = window;
    }
    pthread_mutex_unlock(&order->lock);
    return valid;
}
