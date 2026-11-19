/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of smedia.c; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */
/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: bounded media-fragment framing and caller-owned reassembly.
 * RU: ограниченное framing media fragments и сборка в буфере вызывающего.
 */
/* Original bounded object reassembly over fixed checked slots; transport and codecs stay separate.
 */
#include "flashvm/smedia.h"
#include <string.h>

/* Default geometry is opt-in and remains stable for existing convenience calls. */
static const FVM_SMediaProfile media_default = {FVM_SSLOT_BYTES, FVM_SMEDIA_PAYLOAD};

/* Profile comes from authenticated control agreement, never from a damaged part header. */


/* [standalone-contract:media_profile]
 * EN: media_profile: bounded media-fragment framing and caller-owned reassembly.
 * RU: media_profile: ограниченное framing media fragments и сборка в буфере вызывающего.
 * p (const FVM_SMediaProfile *p): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int media_profile(const FVM_SMediaProfile *p)
{
    return p && p->slot_bytes >= FVM_SSLOT_MIN_BYTES && p->slot_bytes <= FVM_SSLOT_MAX_BYTES && p->part_bytes &&
           p->part_bytes <= p->slot_bytes - FVM_SMEDIA_RESERVE;
}

/* Validate portable metadata before arithmetic, buffer access or session adoption. */


/* [standalone-contract:media_info]
 * EN: media_info: bounded media-fragment framing and caller-owned reassembly.
 * RU: media_info: ограниченное framing media fragments и сборка в буфере вызывающего.
 * info (const FVM_SMediaInfo *info): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int media_info(const FVM_SMediaInfo *info)
{
    return info && (info->mode == FVM_SMEDIA_FILE || info->mode == FVM_SMEDIA_LIVE) && info->stream &&
           info->stream <= FVM_SRECORD_U32 && info->epoch && info->epoch <= FVM_SRECORD_U32 && info->object &&
           info->object <= FVM_SRECORD_U32 && info->time_ms <= FVM_SRECORD_U32 && info->total &&
           info->total <= FVM_SMEDIA_MAX_BYTES;
}

/* Ceiling division is safe after the strict object-size validation. */


/* [standalone-contract:media_parts]
 * EN: media_parts: bounded media-fragment framing and caller-owned reassembly.
 * RU: media_parts: ограниченное framing media fragments и сборка в буфере вызывающего.
 * info (const FVM_SMediaInfo *info): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * p (const FVM_SMediaProfile *p): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: unsigned long; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: unsigned long; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static unsigned long media_parts(const FVM_SMediaInfo *info, const FVM_SMediaProfile *p)
{
    return (info->total - 1) / p->part_bytes + 1;
}

/* A final fragment has an exact short extent; no overlapping or sparse peer-selected offsets. */


/* [standalone-contract:media_size]
 * EN: media_size: bounded media-fragment framing and caller-owned reassembly.
 * RU: media_size: ограниченное framing media fragments и сборка в буфере вызывающего.
 * info (const FVM_SMediaInfo *info): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * part (unsigned long part): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * p (const FVM_SMediaProfile *p): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: size_t; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: size_t; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static size_t media_size(const FVM_SMediaInfo *info, unsigned long part, const FVM_SMediaProfile *p)
{
    unsigned long remaining = info->total - part * p->part_bytes;
    return (size_t)(remaining < p->part_bytes ? remaining : p->part_bytes);
}

/* Append one canonical nonnegative Uint32 without native-endian field stores. */


/* [standalone-contract:media_put]
 * EN: media_put: bounded media-fragment framing and caller-owned reassembly.
 * RU: media_put: ограниченное framing media fragments и сборка в буфере вызывающего.
 * out (unsigned char *out): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * value (unsigned long value): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: size_t; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: size_t; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static size_t media_put(unsigned char *out, unsigned long value)
{
    FVM_SInteger number;
    size_t used = 0;
    number.high = 0;
    number.low = value;
    number.negative = 0;
    fvm_sinteger_write(&number, out, FVM_SRECORD_INTEGER_MAX, &used);
    return used;
}

/* Consume an exact canonical Uint32, leaving incomplete bodies unpublished. */


/* [standalone-contract:media_get]
 * EN: media_get: bounded media-fragment framing and caller-owned reassembly.
 * RU: media_get: ограниченное framing media fragments и сборка в буфере вызывающего.
 * p (const unsigned char **p): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * left (size_t *left): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * value (unsigned long *value): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int media_get(const unsigned char **p, size_t *left, unsigned long *value)
{
    FVM_SInteger number;
    size_t used;
    if (fvm_sinteger_read(*p, *left, &number, &used) != FVM_S_OK || number.high || number.negative)
    {
        return 0;
    }
    *p += used;
    *left -= used;
    *value = number.low;
    return 1;
}

/* Encode a fragment in local storage before the slot writer commits any destination bytes. */


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
FVM_SResult fvm_smedia_write_profile(const FVM_SMediaProfile *profile,
                                     unsigned long tag,
                                     const FVM_SMediaInfo *info,
                                     unsigned long part,
                                     const void *data,
                                     size_t size,
                                     void *output,
                                     size_t capacity,
                                     size_t *used)
{
    unsigned char payload[FVM_SMEDIA_FIELDS * FVM_SRECORD_INTEGER_MAX];
    size_t n = 0, encoded;
    FVM_SResult result;
    if (!media_profile(profile) || !output || !used || !tag || tag > FVM_SRECORD_U32 || !media_info(info) || !data ||
        media_parts(info, profile) > FVM_SMEDIA_PARTS || part >= media_parts(info, profile) ||
        size != media_size(info, part, profile))
    {
        return FVM_S_INVALID;
    }
    if (capacity < profile->slot_bytes)
    {
        return FVM_S_LIMIT;
    }
    n += media_put(payload + n, FVM_SMEDIA_VERSION);
    n += media_put(payload + n, info->mode);
    n += media_put(payload + n, info->stream);
    n += media_put(payload + n, info->epoch);
    n += media_put(payload + n, info->object);
    n += media_put(payload + n, info->time_ms);
    n += media_put(payload + n, info->total);
    n += media_put(payload + n, part);
    result =
        fvm_srecord_write_pair(tag, payload, n, data, size, (unsigned char *)output, profile->slot_bytes, &encoded);
    if (result != FVM_S_OK)
    {
        return result;
    }
    memset((unsigned char *)output + encoded, 0, profile->slot_bytes - encoded);
    *used = profile->slot_bytes;
    return FVM_S_OK;
}

/* Decode without accepting tag declarations from a lossy/untrusted media datagram. */


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
FVM_SResult fvm_smedia_read_profile(
    const FVM_SMediaProfile *profile, const void *input, size_t size, unsigned long tag, FVM_SMediaPiece *out)
{
    FVM_SRecord record;
    FVM_SMediaPiece piece;
    FVM_SResult result;
    unsigned long version;
    const unsigned char *p;
    size_t used, left;
    if (!media_profile(profile) || !out || !tag || tag > FVM_SRECORD_U32)
    {
        return FVM_S_INVALID;
    }
    result = fvm_sslot_read_sized(input, size, profile->slot_bytes, &record, &used);
    if (result != FVM_S_OK)
    {
        return result;
    }
    if (record.tag != tag)
    {
        return FVM_S_UNKNOWN;
    }
    memset(&piece, 0, sizeof(piece));
    p = record.payload;
    left = record.size;
    if (!media_get(&p, &left, &version) || version != FVM_SMEDIA_VERSION || !media_get(&p, &left, &piece.info.mode) ||
        !media_get(&p, &left, &piece.info.stream) || !media_get(&p, &left, &piece.info.epoch) ||
        !media_get(&p, &left, &piece.info.object) || !media_get(&p, &left, &piece.info.time_ms) ||
        !media_get(&p, &left, &piece.info.total) || !media_get(&p, &left, &piece.part) || !media_info(&piece.info) ||
        media_parts(&piece.info, profile) > FVM_SMEDIA_PARTS || piece.part >= media_parts(&piece.info, profile) ||
        left != media_size(&piece.info, piece.part, profile))
    {
        return FVM_S_INVALID;
    }
    piece.data = p;
    piece.size = left;
    *out = piece;
    return FVM_S_OK;
}

/* Validate the receiver's native storage contract; callers must not modify private state. */


/* [standalone-contract:media_receiver]
 * EN: media_receiver: bounded media-fragment framing and caller-owned reassembly.
 * RU: media_receiver: ограниченное framing media fragments и сборка в буфере вызывающего.
 * r (const FVM_SMediaReceiver *r): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int media_receiver(const FVM_SMediaReceiver *r)
{
    return r && r->struct_size == sizeof(*r) && media_info(&r->info) && media_profile(&r->profile) && r->tag &&
           r->tag <= FVM_SRECORD_U32 && r->buffer && r->capacity >= r->info.total &&
           media_parts(&r->info, &r->profile) <= FVM_SMEDIA_PARTS && r->received <= media_parts(&r->info, &r->profile);
}

/* Replace identity/bitmap only after all caller-owned buffer and capacity checks pass. */


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
FVM_SResult fvm_smedia_receiver_init_profile(FVM_SMediaReceiver *out,
                                             const FVM_SMediaProfile *profile,
                                             unsigned long tag,
                                             const FVM_SMediaInfo *info,
                                             void *buffer,
                                             size_t capacity)
{
    FVM_SMediaReceiver r;
    if (!out || !media_profile(profile) || !tag || tag > FVM_SRECORD_U32 || !media_info(info) ||
        media_parts(info, profile) > FVM_SMEDIA_PARTS || !buffer)
    {
        return FVM_S_INVALID;
    }
    if (capacity < info->total)
    {
        return FVM_S_LIMIT;
    }
    memset(&r, 0, sizeof(r));
    r.struct_size = sizeof(r);
    r.info = *info;
    r.profile = *profile;
    r.tag = tag;
    r.buffer = (unsigned char *)buffer;
    r.capacity = capacity;
    *out = r;
    return FVM_S_OK;
}

/* Compare individual fields, never native struct padding or addresses. */


/* [standalone-contract:media_same]
 * EN: media_same: bounded media-fragment framing and caller-owned reassembly.
 * RU: media_same: ограниченное framing media fragments и сборка в буфере вызывающего.
 * a (const FVM_SMediaInfo *a): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * b (const FVM_SMediaInfo *b): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int media_same(const FVM_SMediaInfo *a, const FVM_SMediaInfo *b)
{
    return a->mode == b->mode && a->stream == b->stream && a->epoch == b->epoch && a->object == b->object &&
           a->time_ms == b->time_ms && a->total == b->total;
}

/* Duplicate and out-of-order packets cannot overwrite an already accepted distinct fragment. */


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
FVM_SResult fvm_smedia_accept(FVM_SMediaReceiver *r, const void *input, size_t size, int *complete)
{
    FVM_SMediaPiece piece;
    FVM_SResult result;
    size_t offset, byte;
    unsigned int mask;
    if (!media_receiver(r) || !complete)
    {
        return FVM_S_INVALID;
    }
    result = fvm_smedia_read_profile(&r->profile, input, size, r->tag, &piece);
    if (result != FVM_S_OK)
    {
        return result;
    }
    if (!media_same(&r->info, &piece.info))
    {
        return FVM_S_CONFLICT;
    }
    offset = (size_t)(piece.part * r->profile.part_bytes);
    byte = (size_t)(piece.part / FVM_SMEDIA_BITS);
    mask = 1U << (unsigned int)(piece.part % FVM_SMEDIA_BITS);
    if (r->bitmap[byte] & mask)
    {
        if (memcmp(r->buffer + offset, piece.data, piece.size))
        {
            return FVM_S_CONFLICT;
        }
    }
    else
    {
        memcpy(r->buffer + offset, piece.data, piece.size);
        r->bitmap[byte] |= (unsigned char)mask;
        ++r->received;
    }
