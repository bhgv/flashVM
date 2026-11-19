/* Copyright (c) 2026 bhgv
 * EN: Optional sequence prefix outside exactly one checked original S-record, no generation in data packets.
 * RU: Необязательный префикс номера перед одной проверенной S-записью; без поколения в пакетах данных.
 */
#include "flashvm/sorder.h"
#include <string.h>

/* EN/RU: See sorder.h; validate prefix into temporary, original record writer is atomic before prefix publication.
 * См. sorder.h; prefix в temporary, original writer атомарен до publication prefix.
 */


/* [standalone-contract:fvm_sordered_write]
 * EN: fvm_sordered_write: checked ordinal prefixes around unchanged S-record bytes.
 * RU: fvm_sordered_write: проверяемые префиксы номеров вокруг неизменённых S-record байт.
 * ordinal (const FVM_SOrdinal *ordinal): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * bytes (const void *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * output (void *output): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * capacity (size_t capacity): Actual caller-owned output capacity; it is not permission to write past the buffer.
 * RU: Фактическая ёмкость буфера вызывающего; выход за неё недопустим.
 * used (size_t *used): Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below.
 * RU: Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID, FVM_S_LIMIT. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID, FVM_S_LIMIT. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_sordered_write(const FVM_SOrdinal *ordinal,
                               unsigned long tag,
                               const void *bytes,
                               size_t size,
                               void *output,
                               size_t capacity,
                               size_t *used)
{
    unsigned char prefix[FVM_SORDER_PREFIX_MAX]; /* Temporary exact prefix. */
    size_t header;                               /* Exact prefix extent. */
    size_t frame;                                /* Exact original S-record extent. */
    FVM_SResult result;                          /* Original codec result, no guessed wire status. */
    if (!output || !used)
    {
        return FVM_S_INVALID;
    }
    result = fvm_sordinal_write(ordinal, prefix, sizeof(prefix), &header);
    if (result != FVM_S_OK)
    {
        return result;
    }
    if (capacity < header)
    {
        return FVM_S_LIMIT;
    }
    result = fvm_srecord_write(tag, bytes, size, (unsigned char *)output + header, capacity - header, &frame);
    if (result == FVM_S_OK)
    {
        memcpy(output, prefix, header);
        *used = header + frame;
    }
    return result;
}

/* EN/RU: See sorder.h; both prefix and frame must pass before a borrowed combined view is visible.
 * См. sorder.h; prefix и frame должны пройти проверку до publication combined view.
 */


/* [standalone-contract:fvm_sordered_read]
 * EN: fvm_sordered_read: checked ordinal prefixes around unchanged S-record bytes.
 * RU: fvm_sordered_read: проверяемые префиксы номеров вокруг неизменённых S-record байт.
 * input (const void *input): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * limit (size_t limit): Explicit local quota; permitted ranges and wire/body units are given in the description.
 * RU: Явная локальная квота; диапазон и единицы wire/body указаны в описании.
 * view (FVM_SOrderedRecord *view): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * used (size_t *used): Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below.
 * RU: Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_sordered_read(const void *input, size_t size, size_t limit, FVM_SOrderedRecord *view, size_t *used)
{
    FVM_SOrderedRecord next; /* Private complete borrowed view. */
    size_t header;           /* Exact prefix extent. */
    size_t frame;            /* Exact original S-record extent. */
    FVM_SResult result;      /* Original codec status. */
    if ((!input && size) || !view || !used || !limit || limit > FVM_SRECORD_BODY_MAX)
    {
        return FVM_S_INVALID;
    }
    result = fvm_sordinal_read(input, size, &next.ordinal, &header);
    if (result != FVM_S_OK)
    {
        return result;
    }
    result = fvm_srecord_read((const unsigned char *)input + header, size - header, limit, &next.record, &frame);
    if (result == FVM_S_OK)
    {
        next.encoded_size = header + frame;
        *view = next;
        *used = next.encoded_size;
    }
    return result;
}
