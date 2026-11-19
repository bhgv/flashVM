/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: checked fixed-slot geometry and compact-record extents.
 * RU: проверяемая геометрия fixed slots и размеры compact records.
 */
/* Fixed-size recovery boundaries for batches of independently checked binary S-records. */
#include "flashvm/sslot.h"
#include <string.h>

/* The ordinary writer preflights before touching output; padding cannot subsequently fail. */


/* [standalone-contract:fvm_sslot_write_sized]
 * EN: fvm_sslot_write_sized: checked fixed-slot geometry and compact-record extents.
 * RU: fvm_sslot_write_sized: проверяемая геометрия fixed slots и размеры compact records.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * payload (const void *payload): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * slot_bytes (size_t slot_bytes): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * output (void *output): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * capacity (size_t capacity): Actual caller-owned output capacity; it is not permission to write past the buffer.
 * RU: Фактическая ёмкость буфера вызывающего; выход за неё недопустим.
 * used (size_t *used): Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below.
 * RU: Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_sslot_write_sized(
    unsigned long tag, const void *payload, size_t size, size_t slot_bytes, void *output, size_t capacity, size_t *used)
{
    size_t n;
    FVM_SResult result;
    if (!output || !used || (!payload && size) || slot_bytes < FVM_SSLOT_MIN_BYTES || slot_bytes > FVM_SSLOT_MAX_BYTES)
    {
        return FVM_S_INVALID;
    }
    if (capacity < slot_bytes)
    {
        return FVM_S_LIMIT;
    }
    result = fvm_srecord_write(tag, payload, size, (unsigned char *)output, slot_bytes, &n);
    if (result != FVM_S_OK)
    {
        return result;
    }
    memset((unsigned char *)output + n, 0, slot_bytes - n);
    *used = slot_bytes;
    return FVM_S_OK;
}

/* Preserve the original profile-1 convenience call without making it a channel default. */


/* [standalone-contract:fvm_sslot_write]
 * EN: fvm_sslot_write: checked fixed-slot geometry and compact-record extents.
 * RU: fvm_sslot_write: проверяемая геометрия fixed slots и размеры compact records.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * payload (const void *payload): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * output (void *output): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
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
FVM_SResult
fvm_sslot_write(unsigned long tag, const void *payload, size_t size, void *output, size_t capacity, size_t *used)
{
    return fvm_sslot_write_sized(tag, payload, size, FVM_SSLOT_BYTES, output, capacity, used);
}

/* Never let corrupted embedded framing cross the externally fixed boundary. */


/* [standalone-contract:fvm_sslot_read_sized]
 * EN: fvm_sslot_read_sized: checked fixed-slot geometry and compact-record extents.
 * RU: fvm_sslot_read_sized: проверяемая геометрия fixed slots и размеры compact records.
 * input (const void *input): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * slot_bytes (size_t slot_bytes): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * view (FVM_SRecord *view): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * used (size_t *used): Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below.
 * RU: Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID, FVM_S_MORE, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID, FVM_S_MORE, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_sslot_read_sized(const void *input, size_t size, size_t slot_bytes, FVM_SRecord *view, size_t *used)
{
    FVM_SRecord record;
    FVM_SResult result;
    const unsigned char *bytes = (const unsigned char *)input;
    size_t n, i;
    if ((!input && size) || !view || !used || slot_bytes < FVM_SSLOT_MIN_BYTES || slot_bytes > FVM_SSLOT_MAX_BYTES)
    {
        return FVM_S_INVALID;
    }
    if (size < slot_bytes)
    {
        return FVM_S_MORE;
    }
    if (size != slot_bytes)
    {
        return FVM_S_INVALID;
    }
    result = fvm_srecord_read(input, size, slot_bytes, &record, &n);
    if (result != FVM_S_OK)
    {
        return result == FVM_S_MORE ? FVM_S_INVALID : result;
    }
    for (i = n; i < size; ++i)
    {
        if (bytes[i])
        {
            return FVM_S_INVALID;
        }
    }
    *view = record;
    *used = size;
    return FVM_S_OK;
}

/* Default-size reader remains available to fixed profile-1 users. */


/* [standalone-contract:fvm_sslot_read]
 * EN: fvm_sslot_read: checked fixed-slot geometry and compact-record extents.
 * RU: fvm_sslot_read: проверяемая геометрия fixed slots и размеры compact records.
 * input (const void *input): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * view (FVM_SRecord *view): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * used (size_t *used): Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below.
 * RU: Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже.
 * Return: FVM_SResult; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_sslot_read(const void *input, size_t size, FVM_SRecord *view, size_t *used)
{
    return fvm_sslot_read_sized(input, size, FVM_SSLOT_BYTES, view, used);
}

/* Corrupt slots are reported independently; even damaged lengths cannot redirect iteration. */


/* [standalone-contract:fvm_sslot_batch_sized]
 * EN: fvm_sslot_batch_sized: checked fixed-slot geometry and compact-record extents.
 * RU: fvm_sslot_batch_sized: проверяемая геометрия fixed slots и размеры compact records.
 * input (const void *input): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * slot_bytes (size_t slot_bytes): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * notify (FVM_SSlotNotify notify): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * userdata (void *userdata): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * report (FVM_SSlotReport *report): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_sslot_batch_sized(
    const void *input, size_t size, size_t slot_bytes, FVM_SSlotNotify notify, void *userdata, FVM_SSlotReport *report)
{
    FVM_SSlotReport result;
    FVM_SRecord record;
    FVM_SResult status;
    size_t i, used;
    const unsigned char *bytes = (const unsigned char *)input;
    if ((!input && size) || !report || slot_bytes < FVM_SSLOT_MIN_BYTES || slot_bytes > FVM_SSLOT_MAX_BYTES ||
        size % slot_bytes)
    {
        return FVM_S_INVALID;
    }
    result.slots = size / slot_bytes;
    if (result.slots > FVM_SSLOT_BATCH)
    {
        return FVM_S_LIMIT;
    }
    result.valid = result.damaged = 0;
    for (i = 0; i < result.slots; ++i)
    {
        status = fvm_sslot_read_sized(bytes + i * slot_bytes, slot_bytes, slot_bytes, &record, &used);
        if (status == FVM_S_OK)
        {
            ++result.valid;
        }
        else
        {
            ++result.damaged;
        }
        if (notify)
        {
            notify(i, status, status == FVM_S_OK ? &record : 0, userdata);
        }
    }
    *report = result;
    return FVM_S_OK;
}

/* Profile-1 batch wrapper uses the same independently bounded traversal. */


/* [standalone-contract:fvm_sslot_batch]
 * EN: fvm_sslot_batch: checked fixed-slot geometry and compact-record extents.
 * RU: fvm_sslot_batch: проверяемая геометрия fixed slots и размеры compact records.
 * input (const void *input): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * notify (FVM_SSlotNotify notify): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * userdata (void *userdata): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * report (FVM_SSlotReport *report): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: FVM_SResult; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult
fvm_sslot_batch(const void *input, size_t size, FVM_SSlotNotify notify, void *userdata, FVM_SSlotReport *report)
{
    return fvm_sslot_batch_sized(input, size, FVM_SSLOT_BYTES, notify, userdata, report);
}
