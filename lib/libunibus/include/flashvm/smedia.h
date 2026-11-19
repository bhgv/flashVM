/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: bounded media-fragment framing and caller-owned reassembly.
 * RU: ограниченное framing media fragments и сборка в буфере вызывающего.
 */
/* Bounded media/file fragments carried by checked S-record recovery slots; no socket ownership. */
#ifndef FLASHVM_SMEDIA_H
#define FLASHVM_SMEDIA_H                         /* Guard the standalone C89 fragment/reassembly API. */
#include "scontrol.h"
#define FVM_SMEDIA_VERSION 1UL                   /* Payload schema within the direction-bound fragment tag. */
#define FVM_SMEDIA_TYPE "flashvm.media.fragment" /* Negotiate the tag separately per direction. */
#define FVM_SMEDIA_FILE 1UL                      /* Enumerate missing parts for reliable control-channel requests. */
#define FVM_SMEDIA_LIVE 2UL                      /* Never request missing parts; host owns playback deadlines. */
#define FVM_SMEDIA_PAYLOAD 768UL                 /* All nonfinal parts carry this many raw bytes. */
#define FVM_SMEDIA_PARTS 4096UL                  /* Per-object part/bitmap bound; larger files use more objects. */
#define FVM_SMEDIA_MAX_BYTES (FVM_SMEDIA_PAYLOAD * FVM_SMEDIA_PARTS) /* Three MiB per object. */
#define FVM_SMEDIA_BITMAP (FVM_SMEDIA_PARTS / 8UL)                   /* One bit per accepted part. */
#define FVM_SMEDIA_RESERVE 64UL /* Maximum conservative record/tag/fragment header reserve. */
#define FVM_SMEDIA_FIELDS 8U    /* Version, mode, stream, epoch, object, time_ms, total, part. */
#define FVM_SMEDIA_BITS 8U      /* Wire profile requires eight-bit octets. */
#ifdef __cplusplus
extern "C"
{
#endif
/* Optional negotiated bulk geometry; small ordinary S-records do not use this structure. */
typedef struct FVM_SMediaProfile
{
    size_t slot_bytes;        /* Agreed slot extent, SSLOT_MIN_BYTES..SSLOT_MAX_BYTES. */
    unsigned long part_bytes; /* Positive raw bytes per nonfinal part, fitting metadata reserve. */
} FVM_SMediaProfile;

/* Agreed immutable identity for one object: host must authenticate it before receiver init. */
typedef struct FVM_SMediaInfo
{
    unsigned long mode;    /* FILE or LIVE; receiver rejects a peer's mode change. */
    unsigned long stream;  /* Nonzero Uint32 stream identifier assigned by the host. */
    unsigned long epoch;   /* Nonzero Uint32 session incarnation; never reused for old traffic. */
    unsigned long object;  /* Nonzero Uint32 object/segment identity within that incarnation. */
    unsigned long time_ms; /* Host-defined relative presentation time; zero is valid for files. */
    unsigned long total;   /* Exact object byte count, 1..MAX_BYTES. */
} FVM_SMediaInfo;

/* Verified borrowed fragment. Raw data is never interpreted as a codec or native structure. */
typedef struct FVM_SMediaPiece
{
    FVM_SMediaInfo info;       /* Copied identity and total extent. */
    unsigned long part;        /* Zero-based part index; offset is part*profile.part_bytes. */
    const unsigned char *data; /* Borrowed input until the containing slot is changed. */
    size_t size;               /* Agreed part_bytes, except the exact final remainder. */
} FVM_SMediaPiece;

/* Caller-owned, single-owner reassembly state; fields are private after initialization. */
typedef struct FVM_SMediaReceiver
{
    size_t struct_size;     /* Exact ABI size initialized by receiver_init. */
    FVM_SMediaInfo info;    /* Expected identity; no automatic adoption of network metadata. */
    unsigned long tag;      /* Direction-local fragment tag agreed on authenticated control channel. */
    unsigned char *buffer;  /* Borrowed writable object buffer; no VM roots or hidden allocations. */
    size_t capacity;        /* Accessible extent, at least info.total; stable until reinitialization. */
    unsigned long received; /* Number of distinct accepted parts, not number of packets. */
    unsigned char bitmap[FVM_SMEDIA_BITMAP]; /* Accepted parts; unknown holes are never read. */
    FVM_SMediaProfile profile;               /* Copied already agreed geometry; never inferred from data. */
} FVM_SMediaReceiver;

/* Encode one exact part to one padded slot. tag must be nonzero Uint32. Atomic output/used on
     * failure; payload must have precisely the extent implied by total/part. No allocation or I/O.
     */


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
FVM_API FVM_SResult fvm_smedia_write(unsigned long tag,
                                     const FVM_SMediaInfo *,
                                     unsigned long part,
                                     const void *data,
                                     size_t size,
                                     void *output,
                                     size_t capacity,
                                     size_t *used);


/* Verify slot, tag, canonical fields and exact part extent before publishing borrowed piece.
     * UNKNOWN means another direction-bound tag. Output unchanged on every error. */


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
FVM_API FVM_SResult fvm_smedia_read(const void *, size_t, unsigned long tag, FVM_SMediaPiece *);


/* Validate expected identity and caller capacity before replacing native state; buffer bytes
     * are untouched. Reinitialization explicitly abandons the previous object at a host-chosen
     * deadline. All descriptors, buffers and slot input/output must be disjoint. No cleanup
     * allocation exists. */


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
FVM_API FVM_SResult fvm_smedia_receiver_init(
    FVM_SMediaReceiver *, unsigned long tag, const FVM_SMediaInfo *, void *buffer, size_t capacity);


/* Verify before copying; reject mismatched identity CONFLICT without changing
     * state/buffer/complete. Exact duplicate OK without increasing received; same part with
     * different valid bytes CONFLICT. Copy only verified new part. complete is true once every part
     * arrived, not a playback decision. Caller authenticates packet/session before entry; CRC alone
     * cannot establish trust. */


/* [standalone-contract:fvm_smedia_accept]
 * EN: fvm_smedia_accept: bounded media-fragment framing and caller-owned reassembly.
 * RU: fvm_smedia_accept: ограниченное framing media fragments и сборка в буфере вызывающего.
 * r (FVM_SMediaReceiver *r): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * input (const void *input): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * complete (int *complete): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_API FVM_SResult fvm_smedia_accept(FVM_SMediaReceiver *, const void *, size_t, int *complete);


/* Enumerate absent part indices >=start, at most capacity, in ascending order. Continue with
     * the last returned index+1. FILE reports holes; LIVE always returns count zero. No timer,
     * network request, retry storm or transfer acknowledgement is generated. Invalid args leave
     * output/count unchanged. A host bounds/rate-limits requests and retains sender data through
     * file completion. */


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
FVM_API FVM_SResult fvm_smedia_missing(
    const FVM_SMediaReceiver *, unsigned long start, unsigned long *parts, size_t capacity, size_t *count);


/* Explicit-profile variants use already agreed geometry, preserve the default wrappers and
     * require no object-sized scratch allocation. Total remains bounded by MAX_BYTES and PARTS.
     * Profile changes require a quiesced/new object; a data packet cannot change the profile. */


/* [standalone-contract:fvm_smedia_write_profile]
 * EN: fvm_smedia_write_profile: bounded media-fragment framing and caller-owned reassembly.
 * RU: fvm_smedia_write_profile: ограниченное framing media fragments и сборка в буфере вызывающего.
 * profile (const FVM_SMediaProfile *profile): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
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
FVM_API FVM_SResult fvm_smedia_write_profile(const FVM_SMediaProfile *,
                                             unsigned long tag,
                                             const FVM_SMediaInfo *,
                                             unsigned long part,
                                             const void *,
                                             size_t,
                                             void *,
                                             size_t capacity,
                                             size_t *used);


/* [standalone-contract:fvm_smedia_read_profile]
 * EN: fvm_smedia_read_profile: bounded media-fragment framing and caller-owned reassembly.
 * RU: fvm_smedia_read_profile: ограниченное framing media fragments и сборка в буфере вызывающего.
 * profile (const FVM_SMediaProfile *profile): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * input (const void *input): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * out (FVM_SMediaPiece *out): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID, FVM_S_OK, FVM_S_UNKNOWN. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID, FVM_S_OK, FVM_S_UNKNOWN. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_API FVM_SResult
fvm_smedia_read_profile(const FVM_SMediaProfile *, const void *, size_t, unsigned long tag, FVM_SMediaPiece *);


/* [standalone-contract:fvm_smedia_receiver_init_profile]
 * EN: fvm_smedia_receiver_init_profile: bounded media-fragment framing and caller-owned reassembly.
 * RU: fvm_smedia_receiver_init_profile: ограниченное framing media fragments и сборка в буфере вызывающего.
 * out (FVM_SMediaReceiver *out): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * profile (const FVM_SMediaProfile *profile): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * info (const FVM_SMediaInfo *info): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * buffer (void *buffer): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * capacity (size_t capacity): Actual caller-owned output capacity; it is not permission to write past the buffer.
 * RU: Фактическая ёмкость буфера вызывающего; выход за неё недопустим.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_API FVM_SResult fvm_smedia_receiver_init_profile(FVM_SMediaReceiver *,
                                                     const FVM_SMediaProfile *,
                                                     unsigned long tag,
                                                     const FVM_SMediaInfo *,
                                                     void *,
                                                     size_t capacity);


/* Initialize from committed limits and activation boundary; rejects objects before the agreed
     * first object or from another stream/epoch. Host retires old receive buffers at the segment
     * boundary; no timeline stop or timer is introduced. Generic agreements require host
     * quiescence. */


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
FVM_API FVM_SResult fvm_smedia_receiver_init_agreed(
    FVM_SMediaReceiver *, const FVM_SControlExchange *, const FVM_SMediaInfo *, void *, size_t capacity);
#ifdef __cplusplus
}
#endif
#endif
