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
#define CHECK(condition) \
    do \
    { \
        if (!(condition)) \
        { \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
            exit(1); \
        } \
    } while (0) /* Fatal evaluated check, never compiled out. */


/* EN: Encode one test packet into borrowed output, returning exact size or failing.
 * RU: Создать тестовый пакет в переданном буфере, вернуть размер либо завершить тест.
 */
static size_t packet(unsigned long tag, unsigned long long ordinal, const char *body, unsigned char *output)
{
    FVM_SOrdinal number = {(unsigned long)(ordinal >> 32), (unsigned long)(ordinal & 0xffffffffULL)}; /* Portable limbs. */
    size_t size; /* Complete encoded extent. */
    CHECK(fvm_sordered_write(&number, tag, body, strlen(body), output, 256, &size) == FVM_S_OK);
    return size;
}


/* EN: Feed one generated packet; engine copies bytes before stack storage expires.
 * RU: Передать пакет; очередь копирует байты до освобождения временного буфера.
 */
static int offer(StyxOrder *order, unsigned long tag, unsigned long long ordinal, const char *body)
{
    unsigned char bytes[256]; /* Caller-owned complete packet. */
    size_t size = packet(tag, ordinal, body, bytes); /* Exact packet size. */
    return styxorderfeed(order, bytes, size);
}


/* EN: Consume and compare exactly one selected record; all outputs are local copies.
 * RU: Получить и проверить одну запись выбранного тега; результаты скопированы.
 */
static void take(StyxOrder *order, unsigned long tag, unsigned long long expected, const char *body)
{
    char bytes[128]; /* Private received payload. */
    size_t size = 0; /* Returned payload extent. */
    unsigned long long ordinal = ~expected; /* Sentinel proves successful assignment. */
    CHECK(styxordernext(order, tag, &ordinal, bytes, sizeof(bytes), &size) == STYX_ORDER_RECORD);
    CHECK(ordinal == expected && size == strlen(body) && !memcmp(bytes, body, size));
}
#endif
