/* Copyright (c) 2026 bhgv
 * EN: A bounded duplex mailbox for typed module grants on a reverse PID.
 * RU: Ограниченная двусторонняя очередь полномочий модулей на встречном PID.
 */
#ifndef STYXRPC_CHANNEL_MAILBOX_H
#define STYXRPC_CHANNEL_MAILBOX_H
#include "styxrpc/channel_group.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct StyxRpcChannelMailbox StyxRpcChannelMailbox; /* Opaque synchronized queues. */


/* EN: Create two slots-deep FIFOs for shape and return an owned reverse resource.
 * mailbox borrows that resource through output->release. Retire fids before release.
 * RU: Создать два FIFO по slots мест для shape и вернуть собственный встречный ресурс.
 * mailbox заимствует ресурс до output->release. До освобождения отозвать fid. */
StyxRpcStatus styxrpc_channel_mailbox_create(const char *shape, unsigned slots,
    StyxRpcChannelMailbox **mailbox, StyxRpcChannelOutput *output);


/* EN: Copy a complete typed frame into the FIFO for the peer. MORE if full.
 * RU: Скопировать полный типизированный кадр в FIFO партнёра. MORE при заполнении. */
StyxRpcStatus styxrpc_channel_mailbox_send(StyxRpcChannelMailbox *mailbox, const void *bytes, size_t size);


/* EN: Copy and remove one peer frame. MORE if empty; INVALID if capacity is too small.
 * RU: Скопировать и извлечь кадр партнёра. MORE при пустой очереди, INVALID при малом буфере. */
StyxRpcStatus styxrpc_channel_mailbox_receive(StyxRpcChannelMailbox *mailbox,
    void *bytes, size_t capacity, size_t *size);


/* EN: Create a module-result mailbox with PID leaves and replies at offset zero.
 * Return one owner reference in mailbox independently of the output resource.
 * RU: Создать очередь результата модуля с PID и ответами от нулевого смещения.
 * Вернуть ссылку владельца mailbox независимо от ресурса output. */
StyxRpcStatus styxrpc_channel_mailbox_result_create(const char *shape, unsigned slots,
    StyxRpcChannelMailbox **mailbox, StyxRpcChannelOutput *output);


/* EN: Report whether the result resource has retired. Hold the owner reference.
 * RU: Проверить, действует ли ресурс результата. Удерживать ссылку владельца. */
int styxrpc_channel_mailbox_active(StyxRpcChannelMailbox *mailbox);


/* EN: Drop the owner reference from result_create, independently of the resource.
 * RU: Освободить ссылку владельца result_create независимо от ресурса. */
void styxrpc_channel_mailbox_owner_release(StyxRpcChannelMailbox *mailbox);

#ifdef __cplusplus
}
#endif
#endif
