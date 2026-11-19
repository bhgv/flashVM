/* Copyright (c) 2026 bhgv
 * EN: Active-path cycle detection without shared node flags; bounded growth and balanced native scopes.
 * RU: Обнаружение циклов active path без общих node flags; bounded growth и balanced native scopes.
 */
#include "internal.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
__thread WalkStack
    walk_stack; /* Native thread isolation includes callback recursion and independent parallel searches. */
static pthread_mutex_t identity_lock =
    PTHREAD_MUTEX_INITIALIZER;     /* Protects one-time domain publication and identity minting. */
static unsigned long domain_id[4]; /* Host-configured descriptive VM identity, initially local-only zero. */
static __thread unsigned long
    cycle_serial;                  /* Native-thread diagnostic serial, never part of entity equality or permission. */
static uint64_t next_entity;       /* Monotonic factory identities, never reused after retirement. */

/* EN/RU: See internal.h; reject host-wide values rather than silently truncating portable identity limbs.
 * См. internal.h; host-wide values отказывать, не сужать portable identity limbs молча.
 */


/* [standalone-contract:walk_words]
 * EN: walk_words: walk/walk implementation and native state.
 * RU: walk_words: реализация walk/walk и нативное состояние.
 * words (const unsigned long words[4]): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * nonzero (int nonzero): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int walk_words(const unsigned long words[4], int nonzero)
{
    unsigned long any = 0;
    for (unsigned i = 0; i < 4; i++)
    {
        if (words[i] > 0xffffffffUL)
        {
            return 0;
        }
        any |= words[i];
    }
    return !nonzero || any != 0;
}

/* EN/RU: See walk.h; caller configures a descriptive namespace once, independent of provider access rights.
 * См. walk.h; caller один раз задаёт descriptive namespace независимо от provider access rights.
 */


/* [standalone-contract:walk_domain]
 * EN: walk_domain: walk/walk implementation and native state.
 * RU: walk_domain: реализация walk/walk и нативное состояние.
 * domain (const unsigned long domain[4]): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int walk_domain(const unsigned long domain[4])
{
    if (!domain || !walk_words(domain, 1) || walk_stack.count)
    {
        return 0;
    }
    pthread_mutex_lock(&identity_lock);
    int valid = !walk_words(domain_id, 1) || !memcmp(domain_id, domain, sizeof(domain_id));
    if (valid)
    {
        memcpy(domain_id, domain, sizeof(domain_id));
    }
    pthread_mutex_unlock(&identity_lock);
    return valid;
}

/* EN/RU: See walk.h; query immutable copied identity without changing entity minting or search scopes.
 * См. walk.h; запросить copied identity без изменения entity minting/search scopes.
 */


/* [standalone-contract:walk_domain_get]
 * EN: walk_domain_get: walk/walk implementation and native state.
 * RU: walk_domain_get: реализация walk/walk и нативное состояние.
 * output (unsigned long output[4]): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int walk_domain_get(unsigned long output[4])
{
    if (!output)
    {
        return 0;
    }
    pthread_mutex_lock(&identity_lock);
    int valid = walk_words(domain_id, 1);
    if (valid)
    {
        memcpy(output, domain_id, sizeof(domain_id));
    }
    pthread_mutex_unlock(&identity_lock);
    return valid;
}

/* EN/RU: See walk.h; identity minting has no source callback, I/O, native pointer encoding or authority inference.
 * См. walk.h; identity minting без callback/I/O/pointer encoding/authority inference.
 */


/* [standalone-contract:walk_entity]
 * EN: walk_entity: walk/walk implementation and native state.
 * RU: walk_entity: реализация walk/walk и нативное состояние.
 * output (WalkEntity *output): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int walk_entity(WalkEntity *output)
{
    if (!output)
    {
        return 0;
    }
    pthread_mutex_lock(&identity_lock);
    int valid = next_entity != UINT64_MAX;
    if (valid)
    {
        memset(output, 0, sizeof(*output));
        memcpy(output->domain, domain_id, sizeof(domain_id));
        uint64_t id = ++next_entity;
        output->object[2] = (unsigned long)(id >> 32);
        output->object[3] = (unsigned long)(id & 0xffffffffUL);
    }
    pthread_mutex_unlock(&identity_lock);
    return valid;
}

/* EN/RU: Bounded borrowed string length;1 valid/0 absent or too long, no allocation or lookup.
 * Bounded длина borrowed string;1/0 absent/too long, без allocation/lookup.
 */


/* [standalone-contract:bounded]
 * EN: bounded: walk/walk implementation and native state.
 * RU: bounded: реализация walk/walk и нативное состояние.
 * text (const char *text): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * limit (size_t limit): Explicit local quota; permitted ranges and wire/body units are given in the description.
 * RU: Явная локальная квота; диапазон и единицы wire/body указаны в описании.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int bounded(const char *text, size_t limit)
{
    if (!text)
    {
        return 0;
    }
    for (size_t i = 0; i < limit; i++)
    {
        if (!text[i])
        {
            return 1;
        }
    }
    return 0;
}

/* EN/RU: See walk.h; compare active semantic states, excluding occurrence IDs, rights, timestamps and provider load.
 * См. walk.h; сравнить active semantic states, без occurrence IDs/rights/timestamps/provider load.
 */


/* [standalone-contract:walk_enter]
 * EN: walk_enter: walk/walk implementation and native state.
 * RU: walk_enter: реализация walk/walk и нативное состояние.
 * scope (WalkScope *scope): Request-local active traversal guard; successful entry requires its matching leave.
 * RU: Локальный guard активного обхода запроса; успешный enter требует соответствующего leave.
 * entity (const WalkEntity *entity): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * high (unsigned long high): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * low (unsigned long low): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * operation (unsigned operation): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * remaining (const char *remaining): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * detail (const char *detail): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: WALK_CYCLE, WALK_INVALID, WALK_LIMIT, WALK_NOMEM, WALK_OK. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: WALK_CYCLE, WALK_INVALID, WALK_LIMIT, WALK_NOMEM, WALK_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int walk_enter(WalkScope *scope,
               const WalkEntity *entity,
               unsigned long high,
               unsigned long low,
               unsigned operation,
               const char *remaining,
               const char *detail)
{
    if (!scope || scope->active || !entity || !walk_words(entity->domain, 0) || !walk_words(entity->object, 1) ||
        high > 0xffffffffUL || low > 0xffffffffUL || operation < WALK_INFO || operation > WALK_FSTAT ||
        !bounded(remaining, WALK_PATH_LIMIT) || !bounded(detail, WALK_DETAIL_LIMIT))
    {
        return WALK_INVALID;
    }
    WalkStack *stack = &walk_stack;
    if (!stack->entries)
    {
        stack->entries = stack->small;
        stack->capacity = WALK_INLINE;
    }
    for (size_t i = 0; i < stack->count; i++)
    {
        WalkEntry *entry = &stack->entries[i];
        if (!memcmp(&entry->entity, entity, sizeof(*entity)) && entry->high == high && entry->low == low &&
            entry->operation == operation && !strcmp(entry->remaining, remaining) && !strcmp(entry->detail, detail))
        {
            walk_note_cycle();
            return WALK_CYCLE;
        }
    }
    if (stack->count == WALK_DEPTH_LIMIT)
    {
        return WALK_LIMIT;
    }
    if (stack->count == stack->capacity)
    {
        size_t capacity = stack->capacity * 2;
        WalkEntry *entries = malloc(capacity * sizeof(*entries));
        if (!entries)
        {
            return WALK_NOMEM;
        }
        memcpy(entries, stack->entries, stack->count * sizeof(*entries));
        if (stack->entries != stack->small)
        {
            free(stack->entries);
        }
        stack->entries = entries;
        stack->capacity = capacity;
    }
    scope->owner = stack;
    scope->mark = stack->count;
    WalkEntry *entry = &stack->entries[stack->count++];
    entry->entity = *entity;
    entry->high = high;
    entry->low = low;
    entry->operation = operation;
    entry->remaining = remaining;
    entry->detail = detail;
    scope->entered = stack->count;
    scope->active = 1;
    return WALK_OK;
}

/* EN/RU: See walk.h; removing an active suffix allows the same entity in another completed union branch.
 * См. walk.h; удаление active suffix допускает ту же entity в другой завершённой union branch.
 */


/* [standalone-contract:walk_leave]
 * EN: walk_leave: walk/walk implementation and native state.
 * RU: walk_leave: реализация walk/walk и нативное состояние.
 * scope (WalkScope *scope): Request-local active traversal guard; successful entry requires its matching leave.
 * RU: Локальный guard активного обхода запроса; успешный enter требует соответствующего leave.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int walk_leave(WalkScope *scope)
{
    WalkStack *stack = &walk_stack;
    if (!scope || !scope->active || scope->owner != stack || scope->entered != stack->count ||
        scope->mark > stack->count)
    {
        return 0;
    }
    stack->count = scope->mark;
    free(scope->owned);
    memset(scope, 0, sizeof(*scope));
    if (!stack->count && stack->entries != stack->small)
    {
        free(stack->entries);
        stack->entries = stack->small;
        stack->capacity = WALK_INLINE;
    }
    return 1;
}

/* EN/RU: See walk.h; diagnostic depth never changes traversal state.
 * См. walk.h; diagnostic depth не меняет traversal state.
 */


/* [standalone-contract:walk_depth]
 * EN: walk_depth: walk/walk implementation and native state.
 * RU: walk_depth: реализация walk/walk и нативное состояние.
 * Parameters: none. Параметры: нет.
 * Return: size_t; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: size_t; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
size_t walk_depth(void)
{
    return walk_stack.count;
}

/* EN/RU: See walk.h; scalar native diagnostic only, no source callback or mutation of active entries.
 * См. walk.h; только scalar native diagnostic, без source callback и изменения active entries.
 */


/* [standalone-contract:walk_cycles]
 * EN: walk_cycles: walk/walk implementation and native state.
 * RU: walk_cycles: реализация walk/walk и нативное состояние.
 * Parameters: none. Параметры: нет.
 * Return: unsigned long; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: unsigned long; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
unsigned long walk_cycles(void)
{
    return cycle_serial;
}

/* EN/RU: See walk.h; propagate a proven remote/local loop on this operation's actual native caller.
 * См. walk.h; перенести proven remote/local loop на actual native caller операции.
 */


/* [standalone-contract:walk_note_cycle]
 * EN: walk_note_cycle: walk/walk implementation and native state.
 * RU: walk_note_cycle: реализация walk/walk и нативное состояние.
 * Parameters: none. Параметры: нет.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void walk_note_cycle(void)
{
    cycle_serial++;
}
