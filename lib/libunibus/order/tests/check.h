/* Copyright (c) 2026 bhgv
 * EN: Explicit fatal checks and packet helpers for ordering acceptance tests.
 * RU: Явные проверки и создание пакетов для тестов упорядочивания.
 */
#ifndef ORDER_TEST_CHECK_H
#define ORDER_TEST_CHECK_H
#include "order.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(condition)                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(condition))                                                                                              \
        {                                                                                                              \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition);                                            \
            exit(1);                                                                                                   \
        }                                                                                                              \
    } while (0) /* Fatal evaluated check, never compiled out. */

/* EN: Encode one test packet into borrowed output, returning exact size or failing.
 * RU: Создать тестовый пакет в переданном буфере, вернуть размер либо завершить тест.
 */


/* [standalone-contract:packet]
 * EN: packet: independent per-tag check state.
 * RU: packet: независимое состояние тегов check.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * ordinal (unsigned long long ordinal): Unsigned sequence value for the selected tag/direction, never a machine address.
 * RU: Беззнаковый номер выбранного тега/направления, не адрес машины.
 * body (const char *body): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * output (unsigned char *output): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * Return: size_t; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: size_t; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static size_t packet(unsigned long tag, unsigned long long ordinal, const char *body, unsigned char *output)
{
    FVM_SOrdinal number = {(unsigned long)(ordinal >> 32),
                           (unsigned long)(ordinal & 0xffffffffULL)}; /* Portable limbs. */
    size_t size;                                                      /* Complete encoded extent. */
    CHECK(fvm_sordered_write(&number, tag, body, strlen(body), output, 256, &size) == FVM_S_OK);
    return size;
}

/* EN: Feed one generated packet; engine copies bytes before stack storage expires.
 * RU: Передать пакет; очередь копирует байты до освобождения временного буфера.
 */


/* [standalone-contract:offer]
 * EN: offer: independent per-tag check state.
 * RU: offer: независимое состояние тегов check.
 * order (StyxOrder *order): Native per-tag queue engine; keep it alive until all concurrent calls retire.
 * RU: Нативный механизм очередей тегов; удерживать до завершения параллельных вызовов.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * ordinal (unsigned long long ordinal): Unsigned sequence value for the selected tag/direction, never a machine address.
 * RU: Беззнаковый номер выбранного тега/направления, не адрес машины.
 * body (const char *body): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int offer(StyxOrder *order, unsigned long tag, unsigned long long ordinal, const char *body)
{
    unsigned char bytes[256];                        /* Caller-owned complete packet. */
    size_t size = packet(tag, ordinal, body, bytes); /* Exact packet size. */
    return styxorderfeed(order, bytes, size);
}

/* EN: Consume and compare exactly one selected record; all outputs are local copies.
 * RU: Получить и проверить одну запись выбранного тега; результаты скопированы.
 */


/* [standalone-contract:take]
 * EN: take: independent per-tag check state.
 * RU: take: независимое состояние тегов check.
 * order (StyxOrder *order): Native per-tag queue engine; keep it alive until all concurrent calls retire.
 * RU: Нативный механизм очередей тегов; удерживать до завершения параллельных вызовов.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * expected (unsigned long long expected): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * body (const char *body): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void take(StyxOrder *order, unsigned long tag, unsigned long long expected, const char *body)
{
    char bytes[128];                        /* Private received payload. */
    size_t size = 0;                        /* Returned payload extent. */
    unsigned long long ordinal = ~expected; /* Sentinel proves successful assignment. */
    CHECK(styxordernext(order, tag, &ordinal, bytes, sizeof(bytes), &size) == STYX_ORDER_RECORD);
    CHECK(ordinal == expected && size == strlen(body) && !memcmp(bytes, body, size));
}
#endif
