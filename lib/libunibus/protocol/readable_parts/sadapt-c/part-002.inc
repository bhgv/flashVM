/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of sadapt.c; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */
/* [standalone-contract:fvm_sadapt_feedback_write]
 * EN: fvm_sadapt_feedback_write: host-selected link adaptation decisions and hysteresis.
 * RU: fvm_sadapt_feedback_write: выбранные host решения адаптации связи и гистерезис.
 * f (const FVM_SAdaptFeedback *f): Caller-owned protocol value/state; native layout is never a wire representation.
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
FVM_SResult fvm_sadapt_feedback_write(const FVM_SAdaptFeedback *f, void *output, size_t capacity, size_t *used)
{
    unsigned char payload[FVM_SADAPT_FIELDS * FVM_SRECORD_INTEGER_MAX];
    size_t n = 0;
    if (!adapt_feedback(f))
    {
        return FVM_S_INVALID;
    }
    n += adapt_put(payload + n, FVM_SADAPT_SERVICE);
    n += adapt_put(payload + n, FVM_SADAPT_REPORT);
    n += adapt_put(payload + n, f->stream);
    n += adapt_put(payload + n, f->epoch);
    n += adapt_put(payload + n, f->window);
    n += adapt_put(payload + n, f->expected);
    n += adapt_put(payload + n, f->on_time);
    n += adapt_put(payload + n, f->late);
    n += adapt_put(payload + n, f->damaged);
    n += adapt_put(payload + n, f->rtt_known);
    n += adapt_put(payload + n, f->rtt_ms);
    return fvm_srecord_write(FVM_SRECORD_CONTROL_TAG, payload, n, (unsigned char *)output, capacity, used);
}

/* Unknown services leave routing to the host, preserving legacy bindings and size agreement. */


/* [standalone-contract:fvm_sadapt_feedback_read]
 * EN: fvm_sadapt_feedback_read: host-selected link adaptation decisions and hysteresis.
 * RU: fvm_sadapt_feedback_read: выбранные host решения адаптации связи и гистерезис.
 * input (const void *input): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * out (FVM_SAdaptFeedback *out): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * used (size_t *used): Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below.
 * RU: Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID, FVM_S_OK, FVM_S_UNKNOWN. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID, FVM_S_OK, FVM_S_UNKNOWN. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_sadapt_feedback_read(const void *input, size_t size, FVM_SAdaptFeedback *out, size_t *used)
{
    FVM_SRecord record;
    FVM_SAdaptFeedback f;
    FVM_SResult result;
    unsigned long service, command;
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
    if (!adapt_get(&p, &left, &service))
    {
        return FVM_S_INVALID;
    }
    if (service != FVM_SADAPT_SERVICE)
    {
        return FVM_S_UNKNOWN;
    }
    if (!adapt_get(&p, &left, &command))
    {
        return FVM_S_INVALID;
    }
    if (command != FVM_SADAPT_REPORT)
    {
        return FVM_S_UNKNOWN;
    }
    memset(&f, 0, sizeof(f));
    if (!adapt_get(&p, &left, &f.stream) || !adapt_get(&p, &left, &f.epoch) || !adapt_get(&p, &left, &f.window) ||
        !adapt_get(&p, &left, &f.expected) || !adapt_get(&p, &left, &f.on_time) || !adapt_get(&p, &left, &f.late) ||
        !adapt_get(&p, &left, &f.damaged) || !adapt_get(&p, &left, &f.rtt_known) || !adapt_get(&p, &left, &f.rtt_ms) ||
        left || !adapt_feedback(&f))
    {
        return FVM_S_INVALID;
    }
    *out = f;
    *used = consumed;
    return FVM_S_OK;
}
