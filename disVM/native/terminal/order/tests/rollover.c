/* Copyright (c) 2026 bhgv
 * EN: Selected-counter acknowledged rollover at uint64 MAX, isolation and stale confirmation checks.
 * RU: Подтверждённый сброс одного счётчика у uint64 MAX, изоляция и устаревшие подтверждения.
 */
#include "check.h"


/* EN/RU: Encode and inspect the exact TX ordinal, returning owned packet size to caller.
 * Кодировать и проверить точный TX-номер, вернуть размер пакета в буфере caller.
 */
static size_t encode(StyxOrder *order, unsigned long tag, unsigned long long expected, unsigned char *bytes)
{
    size_t size; /* Complete packet extent. */
    size_t used; /* Verified decoder extent. */
    FVM_SOrderedRecord view; /* Borrowed checked output view. */
    CHECK(styxorderencode(order, tag, "data", 4, bytes, 256, &size) == FVM_S_OK);
    CHECK(fvm_sordered_read(bytes, size, 128, &view, &used) == FVM_S_OK && used == size);
    CHECK((((unsigned long long)view.ordinal.high << 32) | view.ordinal.low) == expected);
    return size;
}


/* EN/RU: Check no silent MAX wrap and only exact ready confirmation releases selected direction.
 * Проверить запрет автоматического wrap и сброс лишь выбранного готового направления.
 */
int main(void)
{
    StyxOrder *order = styxordercreate(128, 8); /* Owned namespace with independent RX/TX. */
    unsigned char bytes[256]; /* Caller-owned packet spool retained until feed. */
    unsigned char last[256]; /* Last old packet can arrive before previous packet. */
    size_t size = 777; /* Failure atomicity sentinel. */
    size_t last_size; /* Saved MAX packet extent. */
    unsigned long long maximum = ~0ULL; /* Full uint64 endpoint. */
    CHECK(order);
    CHECK(styxorderstart(order, 7, STYX_ORDER_TX, maximum - 1));
    CHECK(styxorderstart(order, 7, STYX_ORDER_RX, maximum - 1));
    CHECK(!styxorderstart(order, 7, STYX_ORDER_TX, 0));
    memset(bytes, 0xa5, sizeof(bytes));
    CHECK(styxorderencode(order, 7, "data", 4, bytes, 1, &size) == FVM_S_LIMIT);
    CHECK(size == 777 && bytes[0] == 0xa5);
    size = encode(order, 7, maximum - 1, bytes);
    last_size = encode(order, 7, maximum, last);
    CHECK(styxorderencode(order, 7, "data", 4, last, 256, &last_size) == FVM_S_CONFLICT);
    CHECK(styxorderfeed(order, last, last_size) == STYX_ORDER_ACCEPTED);
    CHECK(styxorderprepare(order, 7, STYX_ORDER_RX, 1, maximum));
    CHECK(styxorderprepare(order, 7, STYX_ORDER_TX, 1, maximum));
    CHECK(!styxorderconfirm(order, 7, STYX_ORDER_TX, 2, 1));
    CHECK(!styxorderconfirm(order, 7, STYX_ORDER_RX, 1, 1));
    CHECK(!styxorderready(order, 7, STYX_ORDER_RX, 1));
    encode(order, 9, 0, last);
    CHECK(styxorderfeed(order, bytes, size) == STYX_ORDER_ACCEPTED);
    CHECK(!styxorderconfirm(order, 7, STYX_ORDER_RX, 1, 1));
    take(order, 7, maximum - 1, "data");
    CHECK(!styxorderready(order, 7, STYX_ORDER_RX, 1));
    take(order, 7, maximum, "data");
    CHECK(styxorderready(order, 7, STYX_ORDER_RX, 1));
    CHECK(styxorderconfirm(order, 7, STYX_ORDER_RX, 1, 1));
    CHECK(styxorderencode(order, 7, "data", 4, bytes, 256, &size) == FVM_S_CONFLICT);
    CHECK(styxorderconfirm(order, 7, STYX_ORDER_TX, 1, 1));
    CHECK(styxorderconfirm(order, 7, STYX_ORDER_TX, 1, 1));
    CHECK(!styxorderconfirm(order, 7, STYX_ORDER_TX, 1, 0));
    size = encode(order, 7, 0, bytes);
    CHECK(styxorderfeed(order, bytes, size) == STYX_ORDER_ACCEPTED);
    take(order, 7, 0, "data");
    CHECK(!styxorderprepare(order, 7, STYX_ORDER_TX, 1, 0));
    CHECK(styxorderprepare(order, 7, STYX_ORDER_TX, 2, 0));
    CHECK(styxorderconfirm(order, 7, STYX_ORDER_TX, 2, 0));
    CHECK(styxorderconfirm(order, 7, STYX_ORDER_TX, 2, 0));
    CHECK(!styxorderconfirm(order, 7, STYX_ORDER_TX, 1, 1));
    encode(order, 7, 1, bytes);
    encode(order, 9, 1, bytes);
    CHECK(styxorderprepare(order, 7, STYX_ORDER_TX, 3, 1));
    styxorderclose(order);
    CHECK(!styxorderconfirm(order, 7, STYX_ORDER_TX, 3, 1));
    styxorderfree(order);
    CHECK(offer(NULL, 1, 0, "null") == STYX_ORDER_REFUSED);
    return 0;
}
