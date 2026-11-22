/* Copyright (c) 2026 bhgv
 * EN: Construct receiver-local Dis values from validated portable Styx records.
 * RU: Создание локальных значений Dis из проверенных переносимых записей Styx.
 */
#ifndef DVM_STYX_MANAGED_VALUE_H
#define DVM_STYX_MANAGED_VALUE_H
#include "flashvm/srecord.h"
#include "../../native/terminal/call_record.h"
#include "../../native/terminal/load_profile.h"


/* [GC:] EN: Decode one leaf of type s, a[i], a[b], a[f] or a[y]. Borrow type and record, allocate
 * on the current ACTIVE resident, and return one owned reference or H for nil.
 * Return zero and preserve result on malformed data. Allocator errors propagate.
 * The recipient must root the returned object before leaving GC admission.
 * RU: Декодировать s или a[y]. Тип и запись заимствуются; выделить объект в
 * текущем ACTIVE resident. Вернуть владеющую ссылку либо H для nil. При
 * неверных данных вернуть 0, сохранив result. Ошибки выделения передаются
 * вызывающему. Вызывающий укореняет объект до выхода из допуска GC. */
int dvm_styx_managed_new(const char *type, const FVM_SRecord *record, void **result);


/* [GC:] EN: Encode a rooted s or primitive array value into disjoint native storage.
 * Observe a stable String owner or borrowed array with immutable header.
 * Payload is snapped before encoding. On failure bytes and used are preserved.
 * RU: Кодировать укоренённые s или a[y] в отдельный нативный буфер.
 * Строка удерживается стабильным владельцем, заголовок массива неизменяем.
 * Данные копируются до кодирования. При отказе bytes и used сохраняются. */
int dvm_styx_managed_write(const char *type, void *value, void *bytes, size_t capacity, size_t *used);


/* EN: Validate one bounded value shape whose managed leaves have a codec.
 * RU: Проверить схему с поддерживаемыми кодеками для управляемых значений.
 */
int dvm_styx_value_shape_supported(const char *shape);

typedef struct DVM_StyxValueFrame
{
    StyxScalarLayout layout; /* NC64 offsets and managed ownership slots. */
    unsigned roots; /* Registered temporary heap roots by leaf index. */
    uint64_t words[STYX_CALL_VALUES]; /* Aligned private argument or result bytes. */
} DVM_StyxValueFrame;


/* [GC:] EN: Decode a checked list into an empty output frame on the ACTIVE resident.
 * On success the frame owns rooted references. Clear it under GC admission,
 * after copying references to managed slots if needed. Failure preserves output.
 * RU: Декодировать проверенный список в пустой кадр на ACTIVE resident.
 * Кадр владеет укоренёнными ссылками. Освободить его при допуске GC, при
 * необходимости после переноса в управляемые слоты. Отказ сохраняет выход. */
int dvm_styx_value_frame_read(unsigned long tag, const char *shape, const void *bytes, size_t size,
    DVM_StyxValueFrame *frame);


/* [GC:] EN: Release all decoded owners and clear the private frame.
 * RU: Освободить все декодированные ссылки и очистить частный кадр. */
void dvm_styx_value_frame_clear(DVM_StyxValueFrame *frame);


/* [GC:] EN: Move a decoded frame into a live mapped return span and transfer
 * temporary roots. Failure leaves both owners and destination unchanged.
 * RU: Переместить кадр в живую mapped область возврата, передав корни.
 * При отказе сохранить кадр и область назначения.
 */
int dvm_styx_value_frame_move(DVM_StyxValueFrame *frame, void *destination, size_t size);

/* [GC:] EN: Encode an NC64 value list from borrowed slots with pinned managed
 * owners. Writes one complete native S-record or preserves bytes and used.
 * RU: Закодировать список NC64 из заимствованных слотов с удержанными
 * владельцами. Выдать одну S-запись либо сохранить bytes и used при отказе. */
int dvm_styx_value_frame_write(unsigned long tag, const char *shape, const void *slots,
    size_t slot_size, void *bytes, size_t capacity, size_t *used);


/* [GC:] EN: Send the return from rooted slots through the active private route.
 * RU: Отправить результат из укоренённых слотов через текущий приватный маршрут. */
void dvm_styx_owner_finish(const char *shape, const void *slots, size_t size, int success);

/* [GC:] EN: Initialize the private mapped frame of a generated gateway.
 * RU: Инициализировать частный mapped кадр сгенерированного шлюза. */
struct Modlink;
int dvm_styx_owner_frame_init(DVM_StyxValueFrame *arguments, struct Modlink *target,
    void *frame, size_t extent);


/* [GC:] EN: Bind a generated bridge to an original rooted export and return an owned link or H.
 * RU: Связать шлюз с удерживаемым исходным экспортом и вернуть собственную ссылку либо H. */
struct Modlink *dvm_styx_managed_gateway(struct Modlink *target, const StyxCallShape *shape);
#endif
