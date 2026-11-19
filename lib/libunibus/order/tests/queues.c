/* Copyright (c) 2026 bhgv
 * EN: Independent tag gaps, bounded reordering, exact copied receives and native waits.
 * RU: Независимость пропусков разных тегов, перестановки, копирование и native-ожидания.
 */
#include "check.h"
#include <pthread.h>

/* EN/RU: One native consumer waiting only tag7; owner keeps engine alive through join.
 * Один native-потребитель ждёт только тег7; engine существует до join.
 */


/* [standalone-contract:waiter]
 * EN: waiter: independent per-tag queues state.
 * RU: waiter: независимое состояние тегов queues.
 * owner (void *owner): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: void *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: void *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void *waiter(void *owner)
{
    CHECK(styxorderwait(owner, 7, 3000));
    take(owner, 7, 0, "zero");
    return NULL;
}

/* EN/RU: A missing tag wait must wake on close, independently of ready messages elsewhere.
 * Ожидание отсутствующего тега завершается при close независимо от чужих сообщений.
 */


/* [standalone-contract:closing]
 * EN: closing: independent per-tag queues state.
 * RU: closing: независимое состояние тегов queues.
 * owner (void *owner): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: void *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: void *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void *closing(void *owner)
{
    CHECK(!styxorderwait(owner, 99, 3000));
    return NULL;
}

/* EN/RU: Exercise public queue contracts with real native threads, no guest polling or sleeps.
 * Проверить очереди с native-потоками, без гостевого polling и sleep.
 */


/* [standalone-contract:main]
 * EN: Validate the program's command-line contract, own its resources and return a process exit status.
 * RU: Проверить контракт командной строки, владеть ресурсами программы и вернуть код завершения.
 * Parameters: none. Параметры: нет.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int main(void)
{
    StyxOrder *order = styxordercreate(128, 8); /* Owned independent namespace. */
    pthread_t thread;                           /* Native selected-tag waiter. */
    unsigned long long ordinal = 123;           /* Unchanged on SMALL. */
    size_t size = 456;                          /* Required size on SMALL. */
    char tiny = 'x';                            /* Too-small output sentinel. */
    unsigned char bytes[256];                   /* Corruption/trailing-byte fixture. */
    CHECK(order);
    CHECK(!styxordercreate(0, 8) && !styxordercreate(128, 0));
    CHECK(!styxordercreate(128, 1ULL << 63));
    CHECK(!pthread_create(&thread, NULL, waiter, order));
    CHECK(offer(order, 7, 2, "two") == STYX_ORDER_ACCEPTED);
    CHECK(offer(order, 7, 2, "two") == STYX_ORDER_DUPLICATE);
    CHECK(offer(order, 7, 2, "other") == STYX_ORDER_CONFLICT);
    CHECK(!styxorderwait(order, 7, 0));
    CHECK(offer(order, 9, 0, "nine") == STYX_ORDER_ACCEPTED);
    take(order, 9, 0, "nine");
    CHECK(offer(order, 7, 9, "far") == STYX_ORDER_WINDOW);
    CHECK(offer(order, 7, 0, "zero") == STYX_ORDER_ACCEPTED);
    CHECK(!pthread_join(thread, NULL));
    CHECK(styxorderreached(order, 7, 0) && !styxorderreached(order, 7, 1));
    CHECK(!styxorderwait(order, 7, 0));
    CHECK(offer(order, 7, 1, "one") == STYX_ORDER_ACCEPTED);
    CHECK(styxordernext(order, 7, &ordinal, &tiny, 1, &size) == STYX_ORDER_SMALL);
    CHECK(ordinal == 123 && size == 3 && tiny == 'x' && !styxorderreached(order, 7, 1));
    take(order, 7, 1, "one");
    take(order, 7, 2, "two");
    CHECK(offer(order, 7, 1, "one") == STYX_ORDER_DUPLICATE);
    CHECK(offer(order, 7, 3, "") == STYX_ORDER_ACCEPTED);
    take(order, 7, 3, "");
    size = packet(7, 4, "four", bytes);
    bytes[size] = 0;
    CHECK(styxorderfeed(order, bytes, size + 1) == STYX_ORDER_REFUSED);
    bytes[size - 1] ^= 1;
    CHECK(styxorderfeed(order, bytes, size) == STYX_ORDER_REFUSED);
    CHECK(!styxorderwait(order, 7, 0));
    CHECK(!pthread_create(&thread, NULL, closing, order));
    CHECK(offer(order, 9, 1, "drain") == STYX_ORDER_ACCEPTED);
    styxorderclose(order);
    CHECK(!pthread_join(thread, NULL));
    take(order, 9, 1, "drain");
    CHECK(styxordernext(order, 9, &ordinal, NULL, 0, &size) == STYX_ORDER_END);
    CHECK(offer(order, 9, 2, "late") == STYX_ORDER_REFUSED);
    styxorderfree(order);
    return 0;
}
