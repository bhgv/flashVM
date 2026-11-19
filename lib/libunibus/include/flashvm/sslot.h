/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: checked fixed-slot geometry and compact-record extents.
 * RU: проверяемая геометрия fixed slots и размеры compact records.
 */
/* Fixed S-record slots preserve the next boundary even when a record length is damaged. */
#ifndef FLASHVM_SSLOT_H
#define FLASHVM_SSLOT_H             /* Guard the standalone C89 slot and batch API. */
#include "srecord.h"
#define FVM_SSLOT_BYTES 1024U       /* Profile 1 fixed slot size, agreed before receiving data. */
#define FVM_SSLOT_MIN_BYTES 256UL   /* Smallest negotiated optional bulk slot. */
#define FVM_SSLOT_MAX_BYTES 65536UL /* Maximum negotiated slot; host must have native capacity. */
#define FVM_SSLOT_BATCH 64U         /* Bounded number of slots processed in one owner call. */
#ifdef __cplusplus
extern "C"
{
#endif


/* One complete record followed by zero padding; input/output/state must not overlap.
     * No allocation, transport, VM or tag binding. Writer publishes exactly SSLOT_BYTES on OK.
     * The slot profile/size must be agreed over a trusted control channel, not inferred from
     * damage. */


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
FVM_API FVM_SResult
fvm_sslot_write(unsigned long tag, const void *payload, size_t size, void *output, size_t capacity, size_t *used);


/* Read exactly one full slot. A shorter input is MORE; longer input INVALID.
     * Check record checksum and all padding before publishing a borrowed view. A MORE from the
     * embedded record in a complete slot becomes INVALID: never consume the next slot as its body.
     * View and used remain unchanged on failure; used is the fixed slot size on success. */


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
FVM_API FVM_SResult fvm_sslot_read(const void *, size_t, FVM_SRecord *, size_t *used);


/* Callback receives a borrowed verified record on OK, otherwise NULL, once per fixed boundary.
     * Owner serializes calls, preserves input throughout callbacks and must not mutate that input.
     * This callback cannot stop a batch or change the next boundary. No managed roots are borrowed.
     */
typedef void (*FVM_SSlotNotify)(size_t index, FVM_SResult, const FVM_SRecord *, void *userdata);

/* Owned result counts for a completely traversed batch, including individually damaged slots.
     */
typedef struct FVM_SSlotReport
{
    size_t slots;   /* Number of fixed slot boundaries visited. */
    size_t valid;   /* Records passing framing, checksum and zero-padding validation. */
    size_t damaged; /* Rejected slots, independently skipped without scanning their length. */
} FVM_SSlotReport;

/* Accept zero through BATCH complete slots; partial trailing slot is INVALID before callbacks.
     * Every structurally bounded batch returns OK even if individual slots are damaged. Inspect
     * report/callback results. Invalid arguments leave report untouched and invoke no callbacks. */


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
FVM_API FVM_SResult fvm_sslot_batch(const void *, size_t, FVM_SSlotNotify, void *, FVM_SSlotReport *);


/* Explicit-size variants use an already agreed slot extent, MIN_BYTES..MAX_BYTES. They have
     * the same ownership/atomicity rules as the default 1024-byte wrappers, and allocate nothing.
     * A size_t unable to represent the negotiated extent must reject it at host negotiation. */


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
FVM_API FVM_SResult fvm_sslot_write_sized(
    unsigned long tag, const void *, size_t, size_t slot_bytes, void *, size_t capacity, size_t *used);


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
FVM_API FVM_SResult fvm_sslot_read_sized(const void *, size_t, size_t slot_bytes, FVM_SRecord *, size_t *used);


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
FVM_API FVM_SResult
fvm_sslot_batch_sized(const void *, size_t, size_t slot_bytes, FVM_SSlotNotify, void *, FVM_SSlotReport *);
#ifdef __cplusplus
}
#endif
#endif
