/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: per-stream measurements and explicit adaptation inputs.
 * RU: измерения потока и явные входы адаптации.
 */
/* Bounded unique-part observations for one host-timed receive window; no clocks or I/O. */
#ifndef FLASHVM_SMEASURE_H
#define FLASHVM_SMEASURE_H /* Guard the standalone C89 measurement ABI. */
#include "sadapt.h"
#include "smedia.h"
#define FVM_SMEASURE_PARTS 4096UL /* Maximum explicitly expected distinct parts per window. */
#define FVM_SMEASURE_BITS 2U      /* Missing, damaged, late or on-time category for each part. */
#define FVM_SMEASURE_PER_BYTE 4U  /* Four two-bit categories per eight-bit wire octet. */
#define FVM_SMEASURE_BYTES (FVM_SMEASURE_PARTS / FVM_SMEASURE_PER_BYTE) /* Fixed native bitmap. */
#define FVM_SMEASURE_MISSING 0U /* No classified arrival; inferred absent at window close. */
#define FVM_SMEASURE_DAMAGED 1U /* Trusted part identity, but unusable data. */
#define FVM_SMEASURE_LATE 2U    /* Verified usable data received after the deadline. */
#define FVM_SMEASURE_ON_TIME 3U /* Verified usable data received on or before the deadline. */
#define FVM_SMEASURE_MASK 3U    /* Select one complete two-bit category. */
#define FVM_SMEASURE_VALID 1U   /* The host verified usable data before recording the arrival. */
#define FVM_SMEASURE_INVALID 0U /* Only use with an independently trusted part identity. */
#ifdef __cplusplus
extern "C"
{
#endif
/* Caller-owned serial state. Private fields after init; no VM roots or borrowed storage. */
typedef struct FVM_SMeasure
{
    size_t struct_size;        /* Exact native ABI, initialized by init. */
    FVM_SAdaptFeedback report; /* Exclusive current counts; RTT supplied only at close. */
    unsigned long deadline_ms; /* Positive host-relative deadline, not absolute wall time. */
    unsigned long last_ms;     /* Greatest accepted arrival timestamp; requires monotonic host input. */
    int closed;                /* A closed observation cannot accept further packets or produce a changed
                        report. */
    unsigned char categories[FVM_SMEASURE_BYTES]; /* Fixed two-bit category for every expected part. */
} FVM_SMeasure;

/* Copy nonzero Uint32 stream/epoch/window, expected 1..PARTS and deadline 1..Uint32.
     * Failure preserves state. No allocation, socket, timer or implicit session adoption. */


/* [standalone-contract:fvm_smeasure_init]
 * EN: fvm_smeasure_init: per-stream measurements and explicit adaptation inputs.
 * RU: fvm_smeasure_init: измерения потока и явные входы адаптации.
 * out (FVM_SMeasure *out): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * stream (unsigned long stream): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * epoch (unsigned long epoch): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * window (unsigned long window): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * expected (unsigned long expected): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * deadline_ms (unsigned long deadline_ms): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_API FVM_SResult fvm_smeasure_init(FVM_SMeasure *,
                                      unsigned long stream,
                                      unsigned long epoch,
                                      unsigned long window,
                                      unsigned long expected,
                                      unsigned long deadline_ms);


/* Record one trusted zero-based part identity and host-relative monotonic elapsed_ms.
     * valid must be VALID or INVALID. A damaged packet cannot supply its own trusted identity:
     * use an authenticated outer envelope or leave it missing. Repeats never increase distinct
     * counts. Usable on-time wins over late/damaged; late wins over damaged; repair can upgrade.
     * Never infer clock time from packet presentation timestamps. Invalid args INVALID,
     * regressing time or a closed window CONFLICT; every failure preserves state. */


/* [standalone-contract:fvm_smeasure_record]
 * EN: fvm_smeasure_record: per-stream measurements and explicit adaptation inputs.
 * RU: fvm_smeasure_record: измерения потока и явные входы адаптации.
 * s (FVM_SMeasure *s): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * part (unsigned long part): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * valid (unsigned int valid): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * elapsed_ms (unsigned long elapsed_ms): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_API FVM_SResult fvm_smeasure_record(FVM_SMeasure *,
                                        unsigned long part,
                                        unsigned int valid,
                                        unsigned long elapsed_ms);


/* Close once, at/after deadline and last arrival; fill owned copied report atomically.
     * RTT known is 0/1; unknown requires zero RTT. No automatic probe or RTT inference.
     * Premature/regressing/already closed CONFLICT; invalid RTT/args INVALID. Later repair
     * cannot improve this historical report. Missing = expected-on_time-late-damaged. */


/* [standalone-contract:fvm_smeasure_finish]
 * EN: fvm_smeasure_finish: per-stream measurements and explicit adaptation inputs.
 * RU: fvm_smeasure_finish: измерения потока и явные входы адаптации.
 * s (FVM_SMeasure *s): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * elapsed_ms (unsigned long elapsed_ms): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * rtt_known (unsigned long rtt_known): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * rtt_ms (unsigned long rtt_ms): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * out (FVM_SAdaptFeedback *out): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_API FVM_SResult fvm_smeasure_finish(
    FVM_SMeasure *, unsigned long elapsed_ms, unsigned long rtt_known, unsigned long rtt_ms, FVM_SAdaptFeedback *);

/* Bound native adapter: receiver/storage remain caller-owned and stable through finish.
     * Snapshot identity/profile rejects receiver reinitialization for another object or geometry.
     */
typedef struct FVM_SMeasureMedia
{
    size_t struct_size;           /* Exact adapter ABI, initialized by media_init. */
    FVM_SMeasure measurement;     /* Private window; host may record independently identified damage. */
    FVM_SMediaReceiver *receiver; /* Borrowed initialized pristine receiver, never a VM object. */
    FVM_SMediaInfo info;          /* Copied agreed object identity, including mode/time/total. */
    FVM_SMediaProfile profile;    /* Copied committed geometry, immutable through the window. */
    unsigned long tag;            /* Copied direction-bound media tag. */
} FVM_SMeasureMedia;

/* Bind an already initialized receiver with no accepted parts; derive expected from agreed
     * total/profile. Copies metadata, borrows receiver and its existing data storage until finish.
     * INVALID for bad descriptors, CONFLICT for a non-pristine receiver; output unchanged. */


/* [standalone-contract:fvm_smeasure_media_init]
 * EN: fvm_smeasure_media_init: per-stream measurements and explicit adaptation inputs.
 * RU: fvm_smeasure_media_init: измерения потока и явные входы адаптации.
 * out (FVM_SMeasureMedia *out): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * receiver (FVM_SMediaReceiver *receiver): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * window (unsigned long window): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * deadline_ms (unsigned long deadline_ms): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_API FVM_SResult fvm_smeasure_media_init(FVM_SMeasureMedia *,
                                            FVM_SMediaReceiver *,
                                            unsigned long window,
                                            unsigned long deadline_ms);


/* Verify slot/identity and accept actual bytes before crediting the decoded part once.
     * Host supplies monotonic relative arrival time and authenticates the channel. Bad packets
     * preserve adapter, receiver, image and complete; unidentified damage remains missing.
     * Late valid packets may complete FILE data before window close without improving on_time.
     * Closed/regressing/reinitialized-object/profile CONFLICT; codec errors propagate. */


/* [standalone-contract:fvm_smeasure_media_accept]
 * EN: fvm_smeasure_media_accept: per-stream measurements and explicit adaptation inputs.
 * RU: fvm_smeasure_media_accept: измерения потока и явные входы адаптации.
 * s (FVM_SMeasureMedia *s): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * input (const void *input): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * elapsed_ms (unsigned long elapsed_ms): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * complete (int *complete): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_API FVM_SResult
fvm_smeasure_media_accept(FVM_SMeasureMedia *, const void *, size_t, unsigned long elapsed_ms, int *complete);


/* Freeze the bound window; identical contract to finish. Afterwards release adapter's borrow
     * and use ordinary media_accept for any later FILE repair, leaving report history unchanged. */


/* [standalone-contract:fvm_smeasure_media_finish]
 * EN: fvm_smeasure_media_finish: per-stream measurements and explicit adaptation inputs.
 * RU: fvm_smeasure_media_finish: измерения потока и явные входы адаптации.
 * s (FVM_SMeasureMedia *s): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * elapsed_ms (unsigned long elapsed_ms): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * rtt_known (unsigned long rtt_known): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * rtt_ms (unsigned long rtt_ms): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * out (FVM_SAdaptFeedback *out): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_API FVM_SResult fvm_smeasure_media_finish(
    FVM_SMeasureMedia *, unsigned long elapsed_ms, unsigned long rtt_known, unsigned long rtt_ms, FVM_SAdaptFeedback *);
#ifdef __cplusplus
}
#endif
#endif
