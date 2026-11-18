/* Copyright (c) 2026 bhgv
 * EN: Native allocation refusals preserve ordinals/queues and other namespaces remain independent.
 * RU: Отказы native-выделения сохраняют номера/очереди, другие пространства имён независимы.
 */
#include "check.h"
static int fail_malloc; /* Armed only around synchronous single-threaded test calls. */
static int fail_calloc; /* Registry allocation failure selector, no worker touches this fixture. */


/* EN/RU: Linker's original malloc, caller owns returned block or NULL.
 * Исходный malloc линкера, caller владеет результатом либо получает NULL.
 */
extern void *__real_malloc(size_t size);


/* EN/RU: Linker's original calloc, caller owns zeroed block or NULL.
 * Исходный calloc линкера, caller владеет обнулённым блоком либо получает NULL.
 */
extern void *__real_calloc(size_t count, size_t size);


/* EN/RU: Refuse an armed copied-payload allocation, otherwise preserve original allocator.
 * Отклонить отмеченное выделение payload, иначе использовать исходный allocator.
 */
void *__wrap_malloc(size_t size)
{
    return fail_malloc ? NULL : __real_malloc(size);
}


/* EN/RU: Refuse an armed registry allocation, otherwise preserve original allocator.
 * Отклонить отмеченное выделение registry, иначе использовать исходный allocator.
 */
void *__wrap_calloc(size_t count, size_t size)
{
    return fail_calloc ? NULL : __real_calloc(count, size);
}


/* EN/RU: Exercise failures before acceptance, early RX boundary and exhausted-counter NACK.
 * Проверить отказы до приёма, раннюю границу RX и NACK исчерпанного счётчика.
 */
int main(void)
{
    StyxOrder *order = styxordercreate(128, 8); /* Owned first namespace. */
    StyxOrder *other = styxordercreate(128, 8); /* Same numeric tag has independent state. */
    unsigned char bytes[256]; /* Private spool. */
    size_t size = 777; /* Atomic refusal sentinel. */
    CHECK(order && other);
    fail_calloc = 1;
    CHECK(offer(order, 7, 0, "zero") == STYX_ORDER_NOMEM);
    CHECK(styxorderencode(order, 7, "zero", 4, bytes, sizeof(bytes), &size) == FVM_S_LIMIT && size == 777);
    fail_calloc = 0;
    CHECK(styxorderstart(order, 7, STYX_ORDER_RX, 0));
    fail_malloc = 1;
    CHECK(offer(order, 7, 0, "zero") == STYX_ORDER_NOMEM);
    fail_malloc = 0;
    CHECK(offer(order, 7, 0, "zero") == STYX_ORDER_ACCEPTED);
    CHECK(styxorderprepare(order, 7, STYX_ORDER_RX, 1, 1));
    CHECK(offer(order, 7, 2, "future") == STYX_ORDER_REFUSED);
    take(order, 7, 0, "zero");
    CHECK(!styxorderconfirm(order, 7, STYX_ORDER_RX, 1, 1));
    CHECK(offer(order, 7, 1, "one") == STYX_ORDER_ACCEPTED);
    take(order, 7, 1, "one");
    CHECK(styxorderconfirm(order, 7, STYX_ORDER_RX, 1, 1));
    CHECK(offer(other, 7, 0, "other") == STYX_ORDER_ACCEPTED);
    take(other, 7, 0, "other");
    CHECK(styxorderstart(order, 7, STYX_ORDER_TX, ~0ULL));
    CHECK(styxorderencode(order, 7, "max", 3, bytes, sizeof(bytes), &size) == FVM_S_OK);
    CHECK(styxorderprepare(order, 7, STYX_ORDER_TX, 1, ~0ULL));
    CHECK(styxorderconfirm(order, 7, STYX_ORDER_TX, 1, 0));
    CHECK(styxorderencode(order, 7, "max", 3, bytes, sizeof(bytes), &size) == FVM_S_CONFLICT);
    CHECK(styxorderprepare(order, 7, STYX_ORDER_TX, 2, ~0ULL));
    CHECK(styxorderconfirm(order, 7, STYX_ORDER_TX, 2, 1));
    CHECK(styxorderencode(order, 7, "new", 3, bytes, sizeof(bytes), &size) == FVM_S_OK);
    CHECK(styxorderfeed(order, bytes, size) == STYX_ORDER_ACCEPTED);
    take(order, 7, 0, "new");
    CHECK(offer(other, 7, 1, "continues") == STYX_ORDER_ACCEPTED);
    take(other, 7, 1, "continues");
    styxorderfree(other);
    styxorderfree(order);
    return 0;
}
