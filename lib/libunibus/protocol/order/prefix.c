/* Copyright (c) 2026 bhgv
 * EN: Canonical one-to-eight-byte unsigned sequence with one combined CRC-5/length byte.
 * RU: Беззнаковый номер в 1..8 байтах с одним общим байтом длины и CRC-5.
 */
#include "flashvm/sorder.h"
#include <string.h>

/* EN/RU: Feed one MSB-first bit into CRC5 x^5+x^2+1, no native-width dependence.
 * Подать один MSB-first bit в CRC5 x^5+x^2+1 независимо от native width.
 */


/* [standalone-contract:bit]
 * EN: bit: unsigned64 ordinal widths and CRC5 validation.
 * RU: bit: ширины unsigned64 номеров и проверка CRC5.
 * crc (unsigned crc): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * value (unsigned value): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: unsigned; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: unsigned; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static unsigned bit(unsigned crc, unsigned value)
{
    unsigned feedback = ((crc >> 4) ^ value) & 1U; /* Highest-register bit XOR input bit. */
    crc = (crc << 1) & 31U;
    if (feedback)
    {
        crc ^= 5U;
    }
    return crc;
}

/* EN/RU: Protect exact3 length bits followed by unsigned sequence bytes, no implicit padding/reflection.
 * Защитить ровно3 length bits и sequence bytes без padding/reflection.
 */


/* [standalone-contract:checksum]
 * EN: checksum: unsigned64 ordinal widths and CRC5 validation.
 * RU: checksum: ширины unsigned64 номеров и проверка CRC5.
 * length (unsigned length): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * bytes (const unsigned char *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * count (size_t count): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: unsigned; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: unsigned; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static unsigned checksum(unsigned length, const unsigned char *bytes, size_t count)
{
    unsigned crc = 31U; /* Explicit initial register. */
    size_t i;           /* Byte cursor. */
    unsigned shift;     /* MSB-first bit cursor. */
    for (shift = 3; shift; shift--)
    {
        crc = bit(crc, (length >> (shift - 1)) & 1U);
    }
    for (i = 0; i < count; i++)
    {
        for (shift = 8; shift; shift--)
        {
            crc = bit(crc, (bytes[i] >> (shift - 1)) & 1U);
        }
    }
    return crc ^ 31U;
}

/* EN/RU: See sorder.h; construct temporary bytes before publishing to preserve failure atomicity.
 * См. sorder.h; temporary bytes до publication сохраняют failure atomicity.
 */


/* [standalone-contract:fvm_sordinal_write]
 * EN: fvm_sordinal_write: unsigned64 ordinal widths and CRC5 validation.
 * RU: fvm_sordinal_write: ширины unsigned64 номеров и проверка CRC5.
 * value (const FVM_SOrdinal *value): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
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
FVM_SResult fvm_sordinal_write(const FVM_SOrdinal *value, void *output, size_t capacity, size_t *used)
{
    unsigned char bytes[FVM_SORDER_PREFIX_MAX]; /* Temporary canonical prefix. */
    unsigned long high;                         /* Portable high limb while shifting whole bytes. */
    unsigned long low;                          /* Portable low limb while shifting whole bytes. */
    size_t count = 0;                           /* Number of nonredundant data bytes. */
    size_t i;                                   /* Output cursor. */
    if (!value || !output || !used || value->high > FVM_SRECORD_U32 || value->low > FVM_SRECORD_U32)
    {
        return FVM_S_INVALID;
    }
    high = value->high;
    low = value->low;
    do
    {
        count++;
        low = ((low >> 8) | (high << 24)) & FVM_SRECORD_U32;
        high >>= 8;
    } while (high || low);
    if (capacity < count + 1)
    {
        return FVM_S_LIMIT;
    }
    high = value->high;
    low = value->low;
    for (i = count; i; i--)
    {
        bytes[i] = (unsigned char)(low & 255UL);
        low = ((low >> 8) | (high << 24)) & FVM_SRECORD_U32;
        high >>= 8;
    }
    bytes[0] = (unsigned char)((checksum((unsigned)(count - 1), bytes + 1, count) << 3) | (count - 1));
    memcpy(output, bytes, count + 1);
    *used = count + 1;
    return FVM_S_OK;
}

/* EN/RU: See sorder.h; validate exact required prefix before publishing limbs/extent.
 * См. sorder.h; проверить required prefix до publication limbs/extent.
 */


/* [standalone-contract:fvm_sordinal_read]
 * EN: fvm_sordinal_read: unsigned64 ordinal widths and CRC5 validation.
 * RU: fvm_sordinal_read: ширины unsigned64 номеров и проверка CRC5.
 * input (const void *input): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * value (FVM_SOrdinal *value): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * used (size_t *used): Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below.
 * RU: Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_CHECKSUM, FVM_S_INVALID, FVM_S_MORE, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_CHECKSUM, FVM_S_INVALID, FVM_S_MORE, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_sordinal_read(const void *input, size_t size, FVM_SOrdinal *value, size_t *used)
{
    const unsigned char *bytes = (const unsigned char *)input; /* Borrowed prefix bytes. */
    FVM_SOrdinal next;                                         /* Private result until checksum/canonicality passed. */
    size_t count;                                              /* Data-byte count from low3 bits. */
    size_t i;                                                  /* Data cursor. */
    if ((!input && size) || !value || !used)
    {
        return FVM_S_INVALID;
    }
    if (!size)
    {
        return FVM_S_MORE;
    }
    count = (bytes[0] & 7U) + 1U;
    if (size < count + 1)
    {
        return FVM_S_MORE;
    }
    if (checksum((unsigned)(count - 1), bytes + 1, count) != (bytes[0] >> 3))
    {
        return FVM_S_CHECKSUM;
    }
    if (count > 1 && !bytes[1])
    {
        return FVM_S_INVALID;
    }
    next.high = 0;
    next.low = 0;
    for (i = 1; i <= count; i++)
    {
        next.high = ((next.high << 8) | (next.low >> 24)) & FVM_SRECORD_U32;
        next.low = ((next.low << 8) | bytes[i]) & FVM_SRECORD_U32;
    }
    *value = next;
    *used = count + 1;
    return FVM_S_OK;
}
