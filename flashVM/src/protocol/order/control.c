/* Copyright (c) 2026 bhgv
 * EN: Allocation-free named service codec for directional offers and single-counter renewal.
 * RU: Именованный кодек направленных предложений и сброса одного счётчика без выделения памяти.
 */
#include "flashvm/sordctl.h"
#include <string.h>


/* EN: Validate all scalar ranges and command-specific unused zeros; no mutation.
 * RU: Проверить границы скаляров и обязательные нули неиспользованных полей; без изменений.
 */
static int valid(const FVM_SOrderControl *m)
{
    if (!m || m->command < FVM_SORDER_OFFER || m->command > FVM_SORDER_NACK ||
        !(m->operation.high || m->operation.low) || !m->lane || m->lane > FVM_SRECORD_U32 ||
        m->tag > FVM_SRECORD_U32 || m->operation.high > FVM_SRECORD_U32 || m->operation.low > FVM_SRECORD_U32 ||
        m->window.high > FVM_SRECORD_U32 || m->window.low > FVM_SRECORD_U32 ||
        m->boundary.high > FVM_SRECORD_U32 || m->boundary.low > FVM_SRECORD_U32)
    {
        return 0;
    }
    if (m->command <= FVM_SORDER_REJECT)
    {
        return m->limit && m->limit <= FVM_SRECORD_BODY_MAX && m->window.high < 0x80000000UL &&
            (m->window.high || m->window.low) && !m->tag && !m->boundary.high && !m->boundary.low;
    }
    return !m->limit && !m->window.high && !m->window.low;
}


/* EN: Expand six portable fields in wire order into private integer storage.
 * RU: Разложить шесть полей в порядке передачи во временный массив чисел.
 */
static void fields(const FVM_SOrderControl *m, FVM_SInteger *values)
{
    memset(values, 0, 6 * sizeof(*values));
    values[0].high = m->operation.high;
    values[0].low = m->operation.low;
    values[1].low = m->lane;
    values[2].low = m->limit;
    values[3].high = m->window.high;
    values[3].low = m->window.low;
    values[4].low = m->tag;
    values[5].high = m->boundary.high;
    values[5].low = m->boundary.low;
}


/* EN/RU: See sordctl.h; publish only after both nested records fit and pass all value checks.
 * См. sordctl.h; выдавать результат лишь после проверки значений и размера обеих записей.
 */
FVM_SResult fvm_sorder_control_write(const FVM_SOrderControl *message, void *output, size_t capacity, size_t *used)
{
    unsigned char encoded[FVM_SORDER_CTL_MAX]; /* Atomic temporary named body. */
    unsigned char payload[6 * FVM_SRECORD_INTEGER_MAX]; /* Canonical six unsigned values. */
    FVM_SInteger values[6]; /* Private portable integer values. */
    size_t body = 0; /* Encoded integer extent. */
    size_t first; /* Identity frame extent. */
    size_t second; /* Command frame extent. */
    size_t part; /* Current integer extent. */
    unsigned i; /* Field cursor. */
    if (!valid(message) || !output || !used)
    {
        return FVM_S_INVALID;
    }
    fields(message, values);
    for (i = 0; i < 6; i++)
    {
        if (fvm_sinteger_write(values + i, payload + body, sizeof(payload) - body, &part) != FVM_S_OK)
        {
            return FVM_S_INVALID;
        }
        body += part;
    }
    if (fvm_srecord_write(0, FVM_SORDER_SERVICE, sizeof(FVM_SORDER_SERVICE) - 1, encoded, sizeof(encoded), &first) != FVM_S_OK ||
        fvm_srecord_write(message->command, payload, body, encoded + first, sizeof(encoded) - first, &second) != FVM_S_OK)
    {
        return FVM_S_LIMIT;
    }
    if (capacity < first + second)
    {
        return FVM_S_LIMIT;
    }
    memcpy(output, encoded, first + second);
    *used = first + second;
    return FVM_S_OK;
}


/* EN/RU: See sordctl.h; unrelated root services stay opaque, known identities require exact canonical fields.
 * См. sordctl.h; чужие службы остаются непрозрачными, известная требует точного канонического тела.
 */
FVM_SResult fvm_sorder_control_read(const void *input, size_t size, FVM_SOrderControl *message)
{
    FVM_SRecord identity; /* Borrowed verified identity frame. */
    FVM_SRecord command; /* Borrowed verified command frame. */
    FVM_SInteger values[6]; /* Decoded private unsigned fields. */
    FVM_SOrderControl next; /* Publish only complete validated message. */
    size_t first; /* Identity extent. */
    size_t second; /* Command extent. */
    size_t offset = 0; /* Integer cursor. */
    size_t part; /* One canonical field extent. */
    unsigned i; /* Field cursor. */
    FVM_SResult result; /* Command checksum/framing result. */
    if ((!input && size) || !message)
    {
        return FVM_S_INVALID;
    }
    if (fvm_srecord_read(input, size, FVM_SORDER_CTL_MAX, &identity, &first) != FVM_S_OK || identity.tag ||
        identity.size != sizeof(FVM_SORDER_SERVICE) - 1 || memcmp(identity.payload, FVM_SORDER_SERVICE, identity.size))
    {
        return FVM_S_UNKNOWN;
    }
    result = fvm_srecord_read((const unsigned char *)input + first, size - first, FVM_SORDER_CTL_MAX, &command, &second);
    if (result != FVM_S_OK)
    {
        return result == FVM_S_MORE ? FVM_S_INVALID : result;
    }
    if (first + second != size)
    {
        return FVM_S_INVALID;
    }
    for (i = 0; i < 6; i++)
    {
        if (fvm_sinteger_read(command.payload + offset, command.size - offset, values + i, &part) != FVM_S_OK || values[i].negative)
        {
            return FVM_S_INVALID;
        }
        offset += part;
    }
    if (offset != command.size || values[1].high || values[2].high || values[4].high)
    {
        return FVM_S_INVALID;
    }
    next.command = command.tag;
    next.operation.high = values[0].high;
    next.operation.low = values[0].low;
    next.lane = values[1].low;
    next.limit = values[2].low;
    next.window.high = values[3].high;
    next.window.low = values[3].low;
    next.tag = values[4].low;
    next.boundary.high = values[5].high;
    next.boundary.low = values[5].low;
    if (!valid(&next))
    {
        return FVM_S_INVALID;
    }
    *message = next;
    return FVM_S_OK;
}
