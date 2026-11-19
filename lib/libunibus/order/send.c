/* Copyright (c) 2026 bhgv
 * EN: Packet spool assigns per-tag unsigned64 ordinals only on successful encoding, with no implicit wrap.
 * RU: 64-битные номера по тегам назначаются только при успешном кодировании; автоматического перехода через максимум нет.
 */
#include "internal.h"

/* EN/RU: Check original body bound before writing any caller output; tag encoding uses existing canonical codec.
 * Проверить original body bound до записи caller output, tag кодируется existing canonical codec.
 */


/* [standalone-contract:fits]
 * EN: fits: independent per-tag send state.
 * RU: fits: независимое состояние тегов send.
 * order (StyxOrder *order): Native per-tag queue engine; keep it alive until all concurrent calls retire.
 * RU: Нативный механизм очередей тегов; удерживать до завершения параллельных вызовов.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int fits(StyxOrder *order, unsigned long tag, size_t size)
{
    FVM_SInteger number = {0, tag, 0};              /* Original unsigned tag representation only. */
    unsigned char encoded[FVM_SRECORD_INTEGER_MAX]; /* Temporary exact tag bytes. */
    size_t used;                                    /* Tag cost charged to record body. */
    return fvm_sinteger_write(&number, encoded, sizeof(encoded), &used) == FVM_S_OK && used <= order->limit &&
           size <= order->limit - used;
}

/* EN/RU: See order.h; caller owns the accepted encoded packet and must not allocate a new number to retry it.
 * См. order.h; caller владеет encoded packet, retry не должен выдавать новый номер.
 */


/* [standalone-contract:styxorderencode]
 * EN: styxorderencode: independent per-tag send state.
 * RU: styxorderencode: независимое состояние тегов send.
 * order (StyxOrder *order): Native per-tag queue engine; keep it alive until all concurrent calls retire.
 * RU: Нативный механизм очередей тегов; удерживать до завершения параллельных вызовов.
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
FVM_SResult styxorderencode(
    StyxOrder *order, unsigned long tag, const void *bytes, size_t size, void *output, size_t capacity, size_t *used)
{
    if (!order || tag > FVM_SRECORD_U32 || (size && !bytes) || !output || !used)
    {
        return FVM_S_INVALID;
    }
    if (!fits(order, tag, size))
    {
        return FVM_S_LIMIT;
    }
    pthread_mutex_lock(&order->lock);
    OrderTag *entry = order->closed ? NULL : ordertag(order, tag, 1); /* Allocate counter before accepting a number. */
    FVM_SResult result = order->closed                                                      ? FVM_S_INVALID
                         : !entry                                                           ? FVM_S_LIMIT
                         : entry->pending[STYX_ORDER_TX] || entry->exhausted[STYX_ORDER_TX] ? FVM_S_CONFLICT
                                                                                            : FVM_S_OK;
    if (result == FVM_S_OK)
    {
        FVM_SOrdinal ordinal; /* Explicit portable limbs, no native uint64 wire layout. */
        ordinal.high = (unsigned long)(entry->transmit >> 32);
        ordinal.low = (unsigned long)(entry->transmit & 0xffffffffULL);
        result = fvm_sordered_write(&ordinal, tag, bytes, size, output, capacity, used);
        if (result == FVM_S_OK)
        {
            entry->used[STYX_ORDER_TX] = 1;
            entry->tx_spooled = 1;
            entry->tx_last = entry->transmit;
            if (entry->transmit == ~0ULL)
            {
                entry->exhausted[STYX_ORDER_TX] = 1;
            }
            else
            {
                entry->transmit++;
            }
        }
    }
    pthread_mutex_unlock(&order->lock);
    return result;
}
