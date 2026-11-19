/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: direction-local numeric tag to schema identity bindings.
 * RU: привязки числовых тегов направления к идентичности схемы.
 */
/* Independent directional tag bindings for binary S-record channels; caller-owned bounded tables.
 */
#ifndef FLASHVM_SBINDINGS_H
#define FLASHVM_SBINDINGS_H        /* Guard the C89 binding registry ABI. */
#include "srecord.h"
#define FVM_SBINDINGS_VERSION 1UL  /* Version of the tag-zero payload schema. */
#define FVM_SBINDINGS_MAX 256U     /* Bound table size and duplicate-check work per declaration. */
#define FVM_SBINDINGS_TYPE_MAX 63U /* Printable ASCII identity bytes, excluding a trailing NUL. */
#ifdef __cplusplus
extern "C"
{
#endif
/* One immutable numeric tag to type identity/version association. */
typedef struct FVM_SBinding
{
    unsigned long tag;                     /* Nonzero unsigned 32-bit local tag. */
    unsigned long version;                 /* Nonzero unsigned 32-bit schema version. */
    unsigned int type_size;                /* Identity bytes, 1..TYPE_MAX. */
    char type[FVM_SBINDINGS_TYPE_MAX + 1]; /* Copied printable ASCII identity; lookup adds NUL. */
} FVM_SBinding;

/* One direction of one session. Keep TX and RX tables separate; do not modify fields directly.
     */
typedef struct FVM_SBindings
{
    size_t struct_size;  /* Exact descriptor size filled by init. */
    FVM_SBinding *slots; /* Caller-owned storage, stable until session disposal/reset. */
    size_t capacity;     /* Number of slots, at most MAX. */
    size_t count;        /* Occupied prefix; registry mutates only after complete validation. */
} FVM_SBindings;

/* Initialize an empty table without modifying slots; caller synchronizes all shared use.
     * Capacity zero/null slots is valid. Reinitializing invalidates prior session associations. */


/* [standalone-contract:fvm_sbindings_init]
 * EN: fvm_sbindings_init: direction-local numeric tag to schema identity bindings.
 * RU: fvm_sbindings_init: привязки числовых тегов направления к идентичности схемы.
 * table (FVM_SBindings *table): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * slots (FVM_SBinding *slots): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * capacity (size_t capacity): Actual caller-owned output capacity; it is not permission to write past the buffer.
 * RU: Фактическая ёмкость буфера вызывающего; выход за неё недопустим.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_API FVM_SResult fvm_sbindings_init(FVM_SBindings *, FVM_SBinding *, size_t capacity);


/* Copy an association to caller output. UNKNOWN for zero/unbound tags; outputs unchanged on
     * error. Lookup copies metadata, not a VM/native object; no allocation or implicit permission
     * grant. */


/* [standalone-contract:fvm_sbindings_lookup]
 * EN: fvm_sbindings_lookup: direction-local numeric tag to schema identity bindings.
 * RU: fvm_sbindings_lookup: привязки числовых тегов направления к идентичности схемы.
 * table (const FVM_SBindings *table): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * out (FVM_SBinding *out): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID, FVM_S_OK, FVM_S_UNKNOWN. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID, FVM_S_OK, FVM_S_UNKNOWN. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_API FVM_SResult fvm_sbindings_lookup(const FVM_SBindings *, unsigned long tag, FVM_SBinding *);


/* Encode only the tag-zero payload, then wrap it with fvm_srecord_write(CONTROL_TAG,...).
     * Payload = schema version, count, repeated(tag, schema version, identity length, identity
     * bytes). Validates the full list/capacity before writing; no duplicate/zero tags, no NUL/space
     * identities. Input associations/output must not overlap; used remains unchanged on failure. */


/* [standalone-contract:fvm_sbindings_payload_write]
 * EN: fvm_sbindings_payload_write: direction-local numeric tag to schema identity bindings.
 * RU: fvm_sbindings_payload_write: привязки числовых тегов направления к идентичности схемы.
 * bindings (const FVM_SBinding *bindings): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * count (size_t count): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
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
FVM_API FVM_SResult
fvm_sbindings_payload_write(const FVM_SBinding *, size_t count, void *output, size_t capacity, size_t *used);


/* Decode/checksum one complete tag-zero record and atomically append all new associations.
     * Exact repeats are idempotent; rebinding a tag returns CONFLICT, duplicates in one record
     * INVALID. Unknown payload version is INVALID. Input/outputs/table/slots must not overlap.
     * Failed/incomplete records leave every slot/count/used unchanged. No VM execution occurs. */


/* [standalone-contract:fvm_sbindings_apply]
 * EN: fvm_sbindings_apply: direction-local numeric tag to schema identity bindings.
 * RU: fvm_sbindings_apply: привязки числовых тегов направления к идентичности схемы.
 * table (FVM_SBindings *table): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * input (const void *input): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * used (size_t *used): Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below.
 * RU: Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_API FVM_SResult fvm_sbindings_apply(FVM_SBindings *, const void *, size_t size, size_t *used);
#ifdef __cplusplus
}
#endif
#endif
