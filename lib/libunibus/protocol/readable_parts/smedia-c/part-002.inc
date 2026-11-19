/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of smedia.c; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */

    *complete = r->received == media_parts(&r->info, &r->profile);
    return FVM_S_OK;
}

/* Missing-file enumeration is a policy hint, never an implicit network retransmission. */


/* [standalone-contract:fvm_smedia_missing]
 * EN: fvm_smedia_missing: bounded media-fragment framing and caller-owned reassembly.
 * RU: fvm_smedia_missing: ограниченное framing media fragments и сборка в буфере вызывающего.
 * r (const FVM_SMediaReceiver *r): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * start (unsigned long start): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * parts (unsigned long *parts): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * capacity (size_t capacity): Actual caller-owned output capacity; it is not permission to write past the buffer.
 * RU: Фактическая ёмкость буфера вызывающего; выход за неё недопустим.
 * count (size_t *count): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_smedia_missing(
    const FVM_SMediaReceiver *r, unsigned long start, unsigned long *parts, size_t capacity, size_t *count)
{
    unsigned long part, total;
    size_t n = 0;
    if (!media_receiver(r) || !count || (!parts && capacity))
    {
        return FVM_S_INVALID;
    }
    total = media_parts(&r->info, &r->profile);
    if (start > total)
    {
        return FVM_S_INVALID;
    }
    if (r->info.mode == FVM_SMEDIA_FILE)
    {
        for (part = start; part < total && n < capacity; ++part)
        {
            if (!(r->bitmap[part / FVM_SMEDIA_BITS] & (1U << (unsigned int)(part % FVM_SMEDIA_BITS))))
            {
                parts[n++] = part;
            }
        }
    }
    *count = n;
    return FVM_S_OK;
}

/* Default profile wrappers retain the original 1024/768 bulk format. */


/* [standalone-contract:fvm_smedia_write]
 * EN: fvm_smedia_write: bounded media-fragment framing and caller-owned reassembly.
 * RU: fvm_smedia_write: ограниченное framing media fragments и сборка в буфере вызывающего.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * info (const FVM_SMediaInfo *info): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * part (unsigned long part): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * data (const void *data): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * out (void *out): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * capacity (size_t capacity): Actual caller-owned output capacity; it is not permission to write past the buffer.
 * RU: Фактическая ёмкость буфера вызывающего; выход за неё недопустим.
 * used (size_t *used): Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below.
 * RU: Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже.
 * Return: FVM_SResult; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_smedia_write(unsigned long tag,
                             const FVM_SMediaInfo *info,
                             unsigned long part,
                             const void *data,
                             size_t size,
                             void *out,
                             size_t capacity,
                             size_t *used)
{
    return fvm_smedia_write_profile(&media_default, tag, info, part, data, size, out, capacity, used);
}

/* Default profile view still borrows bytes, with no implicit negotiation. */


/* [standalone-contract:fvm_smedia_read]
 * EN: fvm_smedia_read: bounded media-fragment framing and caller-owned reassembly.
 * RU: fvm_smedia_read: ограниченное framing media fragments и сборка в буфере вызывающего.
 * input (const void *input): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * out (FVM_SMediaPiece *out): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: FVM_SResult; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_smedia_read(const void *input, size_t size, unsigned long tag, FVM_SMediaPiece *out)
{
    return fvm_smedia_read_profile(&media_default, input, size, tag, out);
}

/* Default receiver copies the built-in profile exactly as an explicitly agreed profile would. */


/* [standalone-contract:fvm_smedia_receiver_init]
 * EN: fvm_smedia_receiver_init: bounded media-fragment framing and caller-owned reassembly.
 * RU: fvm_smedia_receiver_init: ограниченное framing media fragments и сборка в буфере вызывающего.
 * out (FVM_SMediaReceiver *out): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * info (const FVM_SMediaInfo *info): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * buffer (void *buffer): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * capacity (size_t capacity): Actual caller-owned output capacity; it is not permission to write past the buffer.
 * RU: Фактическая ёмкость буфера вызывающего; выход за неё недопустим.
 * Return: FVM_SResult; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_smedia_receiver_init(
    FVM_SMediaReceiver *out, unsigned long tag, const FVM_SMediaInfo *info, void *buffer, size_t capacity)
{
    return fvm_smedia_receiver_init_profile(out, &media_default, tag, info, buffer, capacity);
}

/* Committed metadata selects the geometry; a late old packet can never select it itself. */


/* [standalone-contract:fvm_smedia_receiver_init_agreed]
 * EN: fvm_smedia_receiver_init_agreed: bounded media-fragment framing and caller-owned reassembly.
 * RU: fvm_smedia_receiver_init_agreed: ограниченное framing media fragments и сборка в буфере вызывающего.
 * out (FVM_SMediaReceiver *out): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * agreement (const FVM_SControlExchange *agreement): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * info (const FVM_SMediaInfo *info): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * buffer (void *buffer): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * capacity (size_t capacity): Actual caller-owned output capacity; it is not permission to write past the buffer.
 * RU: Фактическая ёмкость буфера вызывающего; выход за неё недопустим.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_CONFLICT, FVM_S_INVALID. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_CONFLICT, FVM_S_INVALID. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_smedia_receiver_init_agreed(FVM_SMediaReceiver *out,
                                            const FVM_SControlExchange *agreement,
                                            const FVM_SMediaInfo *info,
                                            void *buffer,
                                            size_t capacity)
{
    FVM_SMediaProfile profile;
    const FVM_SControlBoundary *boundary;
    if (!agreement || agreement->struct_size != sizeof(*agreement) || !agreement->active_id ||
        !agreement->active.slot_bytes || agreement->active.slot_bytes > (size_t)-1 || !media_info(info))
    {
        return FVM_S_INVALID;
    }
    boundary = &agreement->active_boundary;
    if (boundary->stream &&
        (boundary->stream != info->stream || boundary->epoch != info->epoch || info->object < boundary->object))
    {
        return FVM_S_CONFLICT;
    }
    profile.slot_bytes = (size_t)agreement->active.slot_bytes;
    profile.part_bytes = agreement->active.part_bytes;
    return fvm_smedia_receiver_init_profile(out, &profile, agreement->active.tag, info, buffer, capacity);
}
