/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of scontrol.c; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */

                                    unsigned long id,
                                    const FVM_SControlLimits *limits,
                                    const FVM_SControlBoundary *boundary,
                                    FVM_SControlMessage *out)
{
    FVM_SControlMessage offer;
    if (!control_exchange(x) || !out || !id || id > FVM_SRECORD_U32 || !control_limits(limits) ||
        !control_boundary(boundary) || (boundary->stream && !limits->slot_bytes))
    {
        return FVM_S_INVALID;
    }
    if (x->pending_id || id <= x->last_id)
    {
        return FVM_S_CONFLICT;
    }
    /* A fixed-capable ceiling also permits a compact offer within its record quota. */
    if (!(control_within(limits, &x->ceiling) ||
          (!limits->slot_bytes && limits->tag == x->ceiling.tag && limits->record_limit <= x->ceiling.record_limit)))
    {
        return FVM_S_LIMIT;
    }
    offer.command = boundary->stream ? FVM_SCONTROL_OFFER_AT : FVM_SCONTROL_OFFER;
    offer.boundary = *boundary;
    x->pending_boundary = *boundary;
    offer.id = id;
    offer.limits = *limits;
    x->pending = *limits;
    x->last_id = x->pending_id = id;
    x->role = offer.command;
    *out = offer;
    return FVM_S_OK;
}

/* Select the smaller portable capacity; negotiation only reduces, never expands local storage. */


/* [standalone-contract:fvm_scontrol_respond]
 * EN: fvm_scontrol_respond: correlated per-tag size/profile negotiation state.
 * RU: fvm_scontrol_respond: коррелированное согласование размера/профиля отдельного тега.
 * x (FVM_SControlExchange *x): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * offer (const FVM_SControlMessage *offer): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * out (FVM_SControlMessage *out): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK, FVM_S_UNKNOWN. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK, FVM_S_UNKNOWN. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_scontrol_respond(FVM_SControlExchange *x, const FVM_SControlMessage *offer, FVM_SControlMessage *out)
{
    FVM_SControlMessage reply;
    FVM_SControlLimits *p = &reply.limits;
    if (!control_exchange(x) || !out || !control_message(offer) || !control_offer(offer->command))
    {
        return FVM_S_INVALID;
    }
    if (x->pending_id || offer->id <= x->last_id || offer->limits.tag != x->ceiling.tag)
    {
        return FVM_S_CONFLICT;
    }
    reply = *offer;
    reply.command = offer->boundary.stream ? FVM_SCONTROL_ACCEPT_AT : FVM_SCONTROL_ACCEPT;
    if (p->record_limit > x->ceiling.record_limit)
    {
        p->record_limit = x->ceiling.record_limit;
    }
    if (p->slot_bytes)
    {
        if (!x->ceiling.slot_bytes)
        {
            return FVM_S_UNKNOWN;
        }
        if (p->slot_bytes > x->ceiling.slot_bytes)
        {
            p->slot_bytes = x->ceiling.slot_bytes;
        }
        if (p->record_limit > p->slot_bytes - FVM_SCONTROL_FRAMING)
        {
            p->record_limit = p->slot_bytes - FVM_SCONTROL_FRAMING;
        }
        if (p->part_bytes > x->ceiling.part_bytes)
        {
            p->part_bytes = x->ceiling.part_bytes;
        }
        if (p->part_bytes > p->record_limit - FVM_SCONTROL_METADATA)
        {
            p->part_bytes = p->record_limit - FVM_SCONTROL_METADATA;
        }
    }
    if (!control_limits(p))
    {
        return FVM_S_LIMIT;
    }
    x->pending = *p;
    x->pending_boundary = offer->boundary;
    x->last_id = x->pending_id = reply.id;
    x->role = reply.command;
    *out = reply;
    return FVM_S_OK;
}

/* Commit correlation/quotas atomically; duplicate committed ACKs are safe and idempotent. */


/* [standalone-contract:fvm_scontrol_commit]
 * EN: fvm_scontrol_commit: correlated per-tag size/profile negotiation state.
 * RU: fvm_scontrol_commit: коррелированное согласование размера/профиля отдельного тега.
 * x (FVM_SControlExchange *x): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * ack (const FVM_SControlMessage *ack): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_scontrol_commit(FVM_SControlExchange *x, const FVM_SControlMessage *ack)
{
    if (!control_exchange(x) || !control_message(ack) || !control_ack(ack->command))
    {
        return FVM_S_INVALID;
    }
    if (!x->pending_id)
    {
        return x->active_id == ack->id && control_same(&x->active, &ack->limits) &&
                       control_boundary_same(&x->active_boundary, &ack->boundary)
                   ? FVM_S_OK
                   : FVM_S_CONFLICT;
    }
    if (ack->id != x->pending_id || !control_boundary_same(&x->pending_boundary, &ack->boundary) ||
        (control_offer(x->role) && !control_within(&ack->limits, &x->pending)) ||
        (control_ack(x->role) && !control_same(&ack->limits, &x->pending)))
    {
        return FVM_S_CONFLICT;
    }
    x->active = ack->limits;
    x->active_boundary = ack->boundary;
    x->active_id = ack->id;
    x->pending_id = x->role = 0;
    memset(&x->pending, 0, sizeof(x->pending));
    memset(&x->pending_boundary, 0, sizeof(x->pending_boundary));
    return FVM_S_OK;
}

/* Cancel preserves the old active configuration and does not recycle request IDs. */


/* [standalone-contract:fvm_scontrol_cancel]
 * EN: fvm_scontrol_cancel: correlated per-tag size/profile negotiation state.
 * RU: fvm_scontrol_cancel: коррелированное согласование размера/профиля отдельного тега.
 * x (FVM_SControlExchange *x): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_scontrol_cancel(FVM_SControlExchange *x)
{
    if (!control_exchange(x))
    {
        return FVM_S_INVALID;
    }
    x->pending_id = x->role = 0;
    memset(&x->pending, 0, sizeof(x->pending));
    memset(&x->pending_boundary, 0, sizeof(x->pending_boundary));
    return FVM_S_OK;
}

/* Generic tag negotiation has no media activation boundary and requires host quiescence. */


/* [standalone-contract:fvm_scontrol_propose]
 * EN: fvm_scontrol_propose: correlated per-tag size/profile negotiation state.
 * RU: fvm_scontrol_propose: коррелированное согласование размера/профиля отдельного тега.
 * x (FVM_SControlExchange *x): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * id (unsigned long id): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * limits (const FVM_SControlLimits *limits): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * out (FVM_SControlMessage *out): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: FVM_SResult; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_scontrol_propose(FVM_SControlExchange *x,
                                 unsigned long id,
                                 const FVM_SControlLimits *limits,
                                 FVM_SControlMessage *out)
{
    static const FVM_SControlBoundary none = {0, 0, 0};
    return fvm_scontrol_propose_at(x, id, limits, &none, out);
}

/* Adapters read only a copied, fully validated committed profile, never an in-flight offer. */


/* [standalone-contract:fvm_scontrol_active]
 * EN: fvm_scontrol_active: correlated per-tag size/profile negotiation state.
 * RU: fvm_scontrol_active: коррелированное согласование размера/профиля отдельного тега.
 * x (const FVM_SControlExchange *x): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * limits (FVM_SControlLimits *limits): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * boundary (FVM_SControlBoundary *boundary): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID, FVM_S_OK, FVM_S_UNKNOWN. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID, FVM_S_OK, FVM_S_UNKNOWN. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult
fvm_scontrol_active(const FVM_SControlExchange *x, FVM_SControlLimits *limits, FVM_SControlBoundary *boundary)
{
    FVM_SControlMessage message;
    if (!control_exchange(x) || !limits || !boundary)
    {
        return FVM_S_INVALID;
    }
    if (!x->active_id)
    {
        return FVM_S_UNKNOWN;
    }
    message.command = x->active_boundary.stream ? FVM_SCONTROL_ACCEPT_AT : FVM_SCONTROL_ACCEPT;
    message.id = x->active_id;
    message.limits = x->active;
    message.boundary = x->active_boundary;
    if (!control_message(&message))
    {
        return FVM_S_INVALID;
    }
    *limits = message.limits;
    *boundary = message.boundary;
    return FVM_S_OK;
}
