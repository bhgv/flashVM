/* Copyright (c) 2026 bhgv
 * EN: C89 named-service codec boundary, canonical fields, exact correlation and atomic failure checks.
 * RU: Проверки C89-кодека именованной службы: каноничность полей, точность операций и атомарность отказов.
 */
#include "flashvm/sordctl.h"
#include <stdio.h>
#include <string.h>


/* EN/RU: Fatal explicit check, condition evaluated in every build, no assertion macro.
 * Явная обязательная проверка во всех сборках, без assertion-макроса.
 */
static int check(int condition, int line)
{
    if (!condition)
    {
        fprintf(stderr, "named order codec line %d\n", line);
    }
    return condition;
}
#define CHECK(value) \
    do \
    { \
        if (!check((value), __LINE__)) \
        { \
            return 1; \
        } \
    } while (0) /* Explicit test exit. */


/* EN/RU: Encode maximum unsigned operation/boundary, exercise malformed known and opaque unknown bodies.
 * Кодировать максимальные operation/boundary, проверить ошибочные известные и непрозрачные чужие тела.
 */
int main(void)
{
    FVM_SOrderControl message; /* Source portable scalars. */
    FVM_SOrderControl decoded; /* Preserved output sentinel. */
    unsigned char bytes[FVM_SORDER_CTL_MAX]; /* Complete named body. */
    unsigned char copy[FVM_SORDER_CTL_MAX]; /* Atomic output sentinel. */
    unsigned char malformed[FVM_SORDER_CTL_MAX]; /* Changed known command body. */
    FVM_SRecord identity; /* Borrowed identity frame. */
    FVM_SRecord command; /* Borrowed command frame. */
    size_t first; /* Identity extent. */
    size_t second; /* Command extent. */
    size_t size; /* Encoded body extent. */
    size_t used = 555; /* Refused encode extent sentinel. */
    memset(&message, 0, sizeof(message));
    message.command = FVM_SORDER_RENEW;
    message.operation.high = 0xffffffffUL;
    message.operation.low = 0xffffffffUL;
    message.lane = 0xffffffffUL;
    message.boundary = message.operation;
    memset(bytes, 0xa5, sizeof(bytes));
    memcpy(copy, bytes, sizeof(bytes));
    CHECK(fvm_sorder_control_write(&message, bytes, 1, &used) == FVM_S_LIMIT && used == 555);
    CHECK(!memcmp(bytes, copy, sizeof(bytes)));
    CHECK(fvm_sorder_control_write(&message, bytes, sizeof(bytes), &size) == FVM_S_OK);
    CHECK(fvm_sorder_control_read(bytes, size, &decoded) == FVM_S_OK && decoded.boundary.high == 0xffffffffUL && decoded.boundary.low == 0xffffffffUL);
    CHECK(decoded.operation.high == 0xffffffffUL && decoded.operation.low == 0xffffffffUL && decoded.lane == 0xffffffffUL && !decoded.tag);
    CHECK(fvm_srecord_read(bytes, size, sizeof(bytes), &identity, &first) == FVM_S_OK && !identity.tag);
    CHECK(fvm_srecord_read(bytes + first, size - first, sizeof(bytes), &command, &second) == FVM_S_OK && command.tag == FVM_SORDER_RENEW);
    decoded.command = 123;
    CHECK(fvm_sorder_control_read("other", 5, &decoded) == FVM_S_UNKNOWN && decoded.command == 123);
    CHECK(fvm_sorder_control_read(bytes, first, &decoded) == FVM_S_INVALID && decoded.command == 123);
    bytes[size] = 0;
    CHECK(fvm_sorder_control_read(bytes, size + 1, &decoded) == FVM_S_INVALID && decoded.command == 123);
    memcpy(malformed, bytes, size);
    malformed[size - 1] ^= 1;
    CHECK(fvm_sorder_control_read(malformed, size, &decoded) == FVM_S_CHECKSUM && decoded.command == 123);
    message.command = FVM_SORDER_OFFER;
    message.operation.high = 0;
    message.operation.low = 1;
    message.limit = 128;
    message.window.low = 8;
    message.boundary.high = 0;
    message.boundary.low = 0;
    CHECK(fvm_sorder_control_write(&message, bytes, sizeof(bytes), &size) == FVM_S_OK);
    CHECK(fvm_sorder_control_read(bytes, size, &decoded) == FVM_S_OK && decoded.limit == 128 && decoded.window.low == 8);
    message.tag = 7;
    CHECK(fvm_sorder_control_write(&message, bytes, sizeof(bytes), &used) == FVM_S_INVALID && used == 555);
    message.tag = 0;
    message.window.high = 0x80000000UL;
    CHECK(fvm_sorder_control_write(&message, bytes, sizeof(bytes), &used) == FVM_S_INVALID);
    return 0;
}
