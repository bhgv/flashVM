/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: canonical integers, bounded S-record framing and checksum validation.
 * RU: канонические целые, framing S-записей и проверка checksum.
 */
/* Binary S-record profile 1: canonical signed-magnitude integers and bounded checked records. */
#ifndef FLASHVM_SRECORD_H
#define FLASHVM_SRECORD_H /* Guard the standalone C89 wire-format interface. */
#include "wire_export.h"
#include <limits.h>
#define FVM_SRECORD_OCTET_BITS 8U /* Wire octets require eight-bit C bytes. */
#if CHAR_BIT != FVM_SRECORD_OCTET_BITS
#error Binary S-record profile 1 requires eight-bit bytes
#endif
#define FVM_SRECORD_PROFILE 1U            /* Out-of-band profile; no implicit wire negotiation. */
#define FVM_SRECORD_FINAL 0x80U           /* Set on the last integer octet, not a continuation octet. */
#define FVM_SRECORD_SIGN 0x40U            /* Sign in the first octet only. */
#define FVM_SRECORD_FIRST_MASK 0x3fU      /* Six magnitude bits in the first integer octet. */
#define FVM_SRECORD_NEXT_MASK 0x7fU       /* Seven magnitude bits in subsequent octets. */
#define FVM_SRECORD_U32 0xffffffffUL      /* Bound a portable limb and the CRC arithmetic. */
#define FVM_SRECORD_SHIFT_LIMIT                                                                                        \
    0x01ffffffUL                          /* Largest high limb safe before a seven-bit shift.      \
                                           */
#define FVM_SRECORD_INTEGER_MAX 10U       /* Maximum canonical encoding of a 64-bit magnitude. */
#define FVM_SRECORD_BODY_MAX 16777216UL   /* Absolute profile body bound, including encoded tag. */
#define FVM_SRECORD_CRC_POLY 0xedb88320UL /* Reflected CRC-32/ISO-HDLC polynomial. */
#define FVM_SRECORD_CONTROL_TAG 0UL       /* Direction-local type binding records. */
#ifdef __cplusplus
extern "C"
{
#endif
/* Protocol results are separate from VM execution results. */
typedef enum FVM_SResult
{
    FVM_S_OK = 0,       /* Complete valid record/value or successful write. */
    FVM_S_MORE = 1,     /* Valid prefix needs more bytes; no output published. */
    FVM_S_INVALID = 2,  /* Invalid argument, integer, geometry or body. */
    FVM_S_LIMIT = 3,    /* Capacity or configured record bound exceeded. */
    FVM_S_CHECKSUM = 4, /* Complete record checksum mismatch. */
    FVM_S_CONFLICT = 5, /* A direction-local tag has another immutable binding. */
    FVM_S_UNKNOWN = 6   /* Requested type binding does not exist. */
} FVM_SResult;

/* Host-endian limbs represent a 64-bit magnitude without requiring C99 long long. */
typedef struct FVM_SInteger
{
    unsigned long high; /* Most significant 32 magnitude bits, never a pointer. */
    unsigned long low;  /* Least significant 32 magnitude bits. */
    int negative;       /* Exactly 0 or 1; negative zero is rejected. */
} FVM_SInteger;

/* Verified view borrows bytes; preserve the complete input through use of this view. */
typedef struct FVM_SRecord
{
    unsigned long tag;            /* Unsigned 32-bit direction-local tag; zero is reserved. */
    const unsigned char *payload; /* Borrowed opaque body after the encoded tag. */
    size_t size;                  /* Payload bytes, including NULs/nested records if supplied. */
    size_t encoded_size;          /* Complete record extent including checksum and length. */
} FVM_SRecord;

/* Canonical big-endian signed magnitude. Nonoverlapping outputs required. No allocation/VM.
     * Write/read value and used only on OK; null input is valid only when size is zero (MORE).
     * Write needs valid value/output/used. Read needs value/used. Maximum magnitude is 2^64-1. */


/* [standalone-contract:fvm_sinteger_write]
 * EN: fvm_sinteger_write: canonical integers, bounded S-record framing and checksum validation.
 * RU: fvm_sinteger_write: канонические целые, framing S-записей и проверка checksum.
 * value (const FVM_SInteger *value): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * out (unsigned char *out): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
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
FVM_API FVM_SResult fvm_sinteger_write(const FVM_SInteger *, unsigned char *, size_t, size_t *used);


/* [standalone-contract:fvm_sinteger_read]
 * EN: fvm_sinteger_read: canonical integers, bounded S-record framing and checksum validation.
 * RU: fvm_sinteger_read: канонические целые, framing S-записей и проверка checksum.
 * in (const unsigned char *in): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * value (FVM_SInteger *value): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * used (size_t *used): Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below.
 * RU: Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID, FVM_S_MORE, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID, FVM_S_MORE, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_API FVM_SResult fvm_sinteger_read(const unsigned char *, size_t, FVM_SInteger *, size_t *used);


/* Record = unsigned checksum, unsigned length, unsigned tag, payload. Length counts
     * tag+payload. CRC covers exact length/tag/payload bytes; profile 1 retains
     * 6+7*(length-varint-size-1) bits, capped at 32. Integers are canonical, so checksum may use
     * fewer bytes if its value is small. Write validates all sizes before output. Payload/output
     * must not overlap; no VM/GC/SDL. Outputs and used remain unchanged on failure. Tag <= U32,
     * body <= BODY_MAX. */


/* [standalone-contract:fvm_srecord_write]
 * EN: fvm_srecord_write: canonical integers, bounded S-record framing and checksum validation.
 * RU: fvm_srecord_write: канонические целые, framing S-записей и проверка checksum.
 * tag (unsigned long tag): Unsigned direction-local tag, 0..0xffffffff; transport tag 0 is service-owned.
 * RU: Беззнаковый тег направления 0..0xffffffff; transport tag 0 принадлежит обслуживанию.
 * payload (const void *payload): Borrowed opaque bytes, possibly nested records; disjoint from output/state.
 * RU: Заимствованные непрозрачные байты, возможно вложенные записи; отдельно от output/state.
 * size (size_t size): Payload bytes; tag+payload must fit the body profile bound.
 * RU: Байты payload; tag+payload должны помещаться в предел body-профиля.
 * out (unsigned char *out): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * capacity (size_t capacity): Accessible destination bytes; insufficient space reports LIMIT.
 * RU: Доступные выходные байты; нехватка места даёт LIMIT.
 * used (size_t *used): Complete encoded byte count on OK; unchanged on failure.
 * RU: Полная кодированная длина при OK; при отказе не меняется.
 * Return: FVM_SResult; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_API FVM_SResult fvm_srecord_write(
    unsigned long tag, const void *payload, size_t size, unsigned char *output, size_t capacity, size_t *used);


/* Scatter-input equivalent of write: concatenate two disjoint borrowed byte spans without
     * an object-sized temporary buffer. Each span and all outputs must not overlap. All size,
     * checksum and capacity checks precede output; no allocation or native structure serialization.
     */


/* [standalone-contract:fvm_srecord_write_pair]
 * EN: fvm_srecord_write_pair: canonical integers, bounded S-record framing and checksum validation.
 * RU: fvm_srecord_write_pair: канонические целые, framing S-записей и проверка checksum.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * first (const void *first): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * first_size (size_t first_size): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * second (const void *second): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * second_size (size_t second_size): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * out (unsigned char *out): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
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
FVM_API FVM_SResult fvm_srecord_write_pair(unsigned long tag,
                                           const void *first,
                                           size_t first_size,
                                           const void *second,
                                           size_t second_size,
                                           unsigned char *,
                                           size_t capacity,
                                           size_t *used);


/* limit is 1..BODY_MAX and counts tag+payload; pass BODY_MAX explicitly for the full profile.
     * MORE means retain/retry the prefix. Checksum is verified before tag interpretation.
     * OK publishes one record and consumed extent; trailing records remain unconsumed.
     * Caller must initialize neither view nor used; both remain unchanged on non-OK. */


/* [standalone-contract:fvm_srecord_read]
 * EN: fvm_srecord_read: canonical integers, bounded S-record framing and checksum validation.
 * RU: fvm_srecord_read: канонические целые, framing S-записей и проверка checksum.
 * input (const void *input): Borrowed input bytes; preserve them throughout use of the returned record view.
 * RU: Заимствованный вход; сохраняется всё время использования record view.
 * size (size_t size): Accessible input bytes; trailing records may remain unconsumed.
 * RU: Доступные входные байты; последующие записи могут остаться непоглощёнными.
 * limit (size_t limit): Allowed body bytes, including encoded tag: 1..FVM_SRECORD_BODY_MAX.
 * RU: Допустимые body-байты, включая encoded tag: 1..FVM_SRECORD_BODY_MAX.
 * view (FVM_SRecord *view): Verified borrowed record, published only on OK.
 * RU: Проверенная заимствованная запись, публикуемая только при OK.
 * used (size_t *used): Bytes of exactly one record on OK; unchanged on any other result.
 * RU: Байты ровно одной записи при OK; иначе не меняется.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_CHECKSUM, FVM_S_INVALID, FVM_S_LIMIT, FVM_S_MORE, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_CHECKSUM, FVM_S_INVALID, FVM_S_LIMIT, FVM_S_MORE, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_API FVM_SResult fvm_srecord_read(const void *input, size_t size, size_t limit, FVM_SRecord *view, size_t *used);
#ifdef __cplusplus
}
#endif
#endif
