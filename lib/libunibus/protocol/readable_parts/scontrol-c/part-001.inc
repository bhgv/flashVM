/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of scontrol.c; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */
/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: correlated per-tag size/profile negotiation state.
 * RU: коррелированное согласование размера/профиля отдельного тега.
 */
/* Tag-zero size agreement is separate from immutable legacy bindings and from data transport. */
#include "flashvm/scontrol.h"
#include <string.h>

/* Enforce conservative fixed-slot reserves and native representability before any activation. */


/* [standalone-contract:control_limits]
 * EN: control_limits: correlated per-tag size/profile negotiation state.
 * RU: control_limits: коррелированное согласование размера/профиля отдельного тега.
 * p (const FVM_SControlLimits *p): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int control_limits(const FVM_SControlLimits *p)
{
    if (!p || !p->tag || p->tag > FVM_SRECORD_U32 || !p->record_limit || p->record_limit > FVM_SRECORD_BODY_MAX ||
        p->record_limit > (size_t)-1)
    {
        return 0;
    }
    if (!p->slot_bytes)
    {
        return !p->part_bytes;
    }
    return p->slot_bytes >= FVM_SSLOT_MIN_BYTES && p->slot_bytes <= FVM_SSLOT_MAX_BYTES &&
           p->slot_bytes <= (size_t)-1 && p->record_limit <= p->slot_bytes - FVM_SCONTROL_FRAMING &&
           p->record_limit > FVM_SCONTROL_METADATA && p->part_bytes &&
           p->part_bytes <= p->record_limit - FVM_SCONTROL_METADATA;
}

/* Equal semantic fields, with no dependence on native padding bytes. */


/* [standalone-contract:control_same]
 * EN: control_same: correlated per-tag size/profile negotiation state.
 * RU: control_same: коррелированное согласование размера/профиля отдельного тега.
 * a (const FVM_SControlLimits *a): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * b (const FVM_SControlLimits *b): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int control_same(const FVM_SControlLimits *a, const FVM_SControlLimits *b)
{
    return a->tag == b->tag && a->record_limit == b->record_limit && a->slot_bytes == b->slot_bytes &&
           a->part_bytes == b->part_bytes;
}

/* An acknowledgement cannot change direction/tag, mode or enlarge any proposed quota. */


/* [standalone-contract:control_within]
 * EN: control_within: correlated per-tag size/profile negotiation state.
 * RU: control_within: коррелированное согласование размера/профиля отдельного тега.
 * p (const FVM_SControlLimits *p): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * ceiling (const FVM_SControlLimits *ceiling): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int control_within(const FVM_SControlLimits *p, const FVM_SControlLimits *ceiling)
{
    return control_limits(p) && p->tag == ceiling->tag && p->record_limit <= ceiling->record_limit &&
           ((!p->slot_bytes && !ceiling->slot_bytes) ||
            (p->slot_bytes && ceiling->slot_bytes && p->slot_bytes <= ceiling->slot_bytes &&
             p->part_bytes <= ceiling->part_bytes));
}

/* A native exchange owns no allocations; callers must not edit its private state. */


/* [standalone-contract:control_exchange]
 * EN: control_exchange: correlated per-tag size/profile negotiation state.
 * RU: control_exchange: коррелированное согласование размера/профиля отдельного тега.
 * x (const FVM_SControlExchange *x): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int control_exchange(const FVM_SControlExchange *x)
{
    return x && x->struct_size == sizeof(*x) && control_limits(&x->ceiling);
}

/* Boundaries are either explicitly absent or complete portable media identities. */


/* [standalone-contract:control_boundary]
 * EN: control_boundary: correlated per-tag size/profile negotiation state.
 * RU: control_boundary: коррелированное согласование размера/профиля отдельного тега.
 * b (const FVM_SControlBoundary *b): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int control_boundary(const FVM_SControlBoundary *b)
{
    return b && ((!b->stream && !b->epoch && !b->object) ||
                 (b->stream && b->stream <= FVM_SRECORD_U32 && b->epoch && b->epoch <= FVM_SRECORD_U32 && b->object &&
                  b->object <= FVM_SRECORD_U32));
}

/* ACKs cannot silently move a media profile change to a different object or epoch. */


/* [standalone-contract:control_boundary_same]
 * EN: control_boundary_same: correlated per-tag size/profile negotiation state.
 * RU: control_boundary_same: коррелированное согласование размера/профиля отдельного тега.
 * a (const FVM_SControlBoundary *a): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * b (const FVM_SControlBoundary *b): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int control_boundary_same(const FVM_SControlBoundary *a, const FVM_SControlBoundary *b)
{
    return a->stream == b->stream && a->epoch == b->epoch && a->object == b->object;
}

/* Distinguish offer and ACK variants without interpreting unknown service commands. */


/* [standalone-contract:control_offer]
 * EN: control_offer: correlated per-tag size/profile negotiation state.
 * RU: control_offer: коррелированное согласование размера/профиля отдельного тега.
 * command (unsigned long command): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int control_offer(unsigned long command)
{
    return command == FVM_SCONTROL_OFFER || command == FVM_SCONTROL_OFFER_AT;
}

/* Only these commands may commit active data limits. */


/* [standalone-contract:control_ack]
 * EN: control_ack: correlated per-tag size/profile negotiation state.
 * RU: control_ack: коррелированное согласование размера/профиля отдельного тега.
 * command (unsigned long command): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int control_ack(unsigned long command)
{
    return command == FVM_SCONTROL_ACCEPT || command == FVM_SCONTROL_ACCEPT_AT;
}

/* Native messages must contain a recognized command and exact Uint32 correlation. */


/* [standalone-contract:control_message]
 * EN: control_message: correlated per-tag size/profile negotiation state.
 * RU: control_message: коррелированное согласование размера/профиля отдельного тега.
 * m (const FVM_SControlMessage *m): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int control_message(const FVM_SControlMessage *m)
{
    return m && (control_offer(m->command) || control_ack(m->command)) && m->id && m->id <= FVM_SRECORD_U32 &&
           control_limits(&m->limits) && control_boundary(&m->boundary) &&
           ((m->command == FVM_SCONTROL_OFFER_AT || m->command == FVM_SCONTROL_ACCEPT_AT)
                ? m->boundary.stream != 0 && m->limits.slot_bytes != 0
                : m->boundary.stream == 0);
}

/* Append a canonical field into bounded local control-message scratch storage. */


/* [standalone-contract:control_put]
 * EN: control_put: correlated per-tag size/profile negotiation state.
 * RU: control_put: коррелированное согласование размера/профиля отдельного тега.
 * out (unsigned char *out): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * value (unsigned long value): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: size_t; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: size_t; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static size_t control_put(unsigned char *out, unsigned long value)
{
    FVM_SInteger n;
    size_t used = 0;
    n.high = 0;
    n.low = value;
    n.negative = 0;
    fvm_sinteger_write(&n, out, FVM_SRECORD_INTEGER_MAX, &used);
    return used;
}

/* Read exactly a canonical nonnegative Uint32 service field. */


/* [standalone-contract:control_get]
 * EN: control_get: correlated per-tag size/profile negotiation state.
 * RU: control_get: коррелированное согласование размера/профиля отдельного тега.
 * p (const unsigned char **p): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * left (size_t *left): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * value (unsigned long *value): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int control_get(const unsigned char **p, size_t *left, unsigned long *value)
{
    FVM_SInteger n;
    size_t used;
    if (fvm_sinteger_read(*p, *left, &n, &used) != FVM_S_OK || n.high || n.negative)
    {
        return 0;
    }
    *p += used;
    *left -= used;
    *value = n.low;
    return 1;
}

/* Small service messages remain ordinary variable-length S-records, never padded media slots. */


/* [standalone-contract:fvm_scontrol_write]
 * EN: fvm_scontrol_write: correlated per-tag size/profile negotiation state.
 * RU: fvm_scontrol_write: коррелированное согласование размера/профиля отдельного тега.
 * m (const FVM_SControlMessage *m): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * output (void *output): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * capacity (size_t capacity): Actual caller-owned output capacity; it is not permission to write past the buffer.
 * RU: Фактическая ёмкость буфера вызывающего; выход за неё недопустим.
 * used (size_t *used): Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below.
 * RU: Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_scontrol_write(const FVM_SControlMessage *m, void *output, size_t capacity, size_t *used)
{
    unsigned char payload[FVM_SCONTROL_FIELDS * FVM_SRECORD_INTEGER_MAX];
    size_t n = 0;
    if (!control_message(m))
    {
        return FVM_S_INVALID;
    }
    n += control_put(payload + n, FVM_SCONTROL_VERSION);
    n += control_put(payload + n, m->command);
    n += control_put(payload + n, m->id);
    n += control_put(payload + n, m->limits.tag);
    n += control_put(payload + n, m->limits.record_limit);
    n += control_put(payload + n, m->limits.slot_bytes);
    n += control_put(payload + n, m->limits.part_bytes);
    n += control_put(payload + n, 0);
    if (m->boundary.stream)
    {
        n += control_put(payload + n, m->boundary.stream);
        n += control_put(payload + n, m->boundary.epoch);
        n += control_put(payload + n, m->boundary.object);
    }
    return fvm_srecord_write(FVM_SRECORD_CONTROL_TAG, payload, n, (unsigned char *)output, capacity, used);
}

/* Unknown tag-zero subservices remain distinguishable from a malformed known size message. */


/* [standalone-contract:fvm_scontrol_read]
 * EN: fvm_scontrol_read: correlated per-tag size/profile negotiation state.
 * RU: fvm_scontrol_read: коррелированное согласование размера/профиля отдельного тега.
 * input (const void *input): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * out (FVM_SControlMessage *out): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * used (size_t *used): Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below.
 * RU: Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID, FVM_S_OK, FVM_S_UNKNOWN. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID, FVM_S_OK, FVM_S_UNKNOWN. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_scontrol_read(const void *input, size_t size, FVM_SControlMessage *out, size_t *used)
{
    FVM_SRecord record;
    FVM_SControlMessage m;
    FVM_SResult result;
    unsigned long version, reserved;
    const unsigned char *p;
    size_t left, consumed;
    if (!out || !used)
    {
        return FVM_S_INVALID;
    }
    result = fvm_srecord_read(input, size, FVM_SRECORD_BODY_MAX, &record, &consumed);
    if (result != FVM_S_OK)
    {
        return result;
    }
    if (record.tag != FVM_SRECORD_CONTROL_TAG)
    {
        return FVM_S_UNKNOWN;
    }
    p = record.payload;
    left = record.size;
    if (!control_get(&p, &left, &version))
    {
        return FVM_S_INVALID;
    }
    if (version != FVM_SCONTROL_VERSION)
    {
        return FVM_S_UNKNOWN;
    }
    memset(&m, 0, sizeof(m));
    if (!control_get(&p, &left, &m.command))
    {
        return FVM_S_INVALID;
    }
    if (!control_offer(m.command) && !control_ack(m.command))
    {
        return FVM_S_UNKNOWN;
    }
    if (!control_get(&p, &left, &m.id) || !control_get(&p, &left, &m.limits.tag) ||
        !control_get(&p, &left, &m.limits.record_limit) || !control_get(&p, &left, &m.limits.slot_bytes) ||
        !control_get(&p, &left, &m.limits.part_bytes) || !control_get(&p, &left, &reserved) || reserved)
    {
        return FVM_S_INVALID;
    }
    if ((m.command == FVM_SCONTROL_OFFER_AT || m.command == FVM_SCONTROL_ACCEPT_AT) &&
        (!control_get(&p, &left, &m.boundary.stream) || !control_get(&p, &left, &m.boundary.epoch) ||
         !control_get(&p, &left, &m.boundary.object)))
    {
        return FVM_S_INVALID;
    }
    if (left || !control_message(&m))
    {
        return FVM_S_INVALID;
    }
    *out = m;
    *used = consumed;
    return FVM_S_OK;
}

/* Initialize no active agreement; a local capacity declaration alone never authorizes data. */


/* [standalone-contract:fvm_scontrol_init]
 * EN: fvm_scontrol_init: correlated per-tag size/profile negotiation state.
 * RU: fvm_scontrol_init: коррелированное согласование размера/профиля отдельного тега.
 * out (FVM_SControlExchange *out): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * ceiling (const FVM_SControlLimits *ceiling): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_scontrol_init(FVM_SControlExchange *out, const FVM_SControlLimits *ceiling)
{
    FVM_SControlExchange x;
    if (!out || !control_limits(ceiling))
    {
        return FVM_S_INVALID;
    }
    memset(&x, 0, sizeof(x));
    x.struct_size = sizeof(x);
    x.ceiling = *ceiling;
    *out = x;
    return FVM_S_OK;
}

/* Starting a pending proposal does not replace an existing committed configuration. */


/* [standalone-contract:fvm_scontrol_propose_at]
 * EN: fvm_scontrol_propose_at: correlated per-tag size/profile negotiation state.
 * RU: fvm_scontrol_propose_at: коррелированное согласование размера/профиля отдельного тега.
 * x (FVM_SControlExchange *x): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * id (unsigned long id): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * limits (const FVM_SControlLimits *limits): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * boundary (const FVM_SControlBoundary *boundary): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * out (FVM_SControlMessage *out): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_scontrol_propose_at(FVM_SControlExchange *x,
