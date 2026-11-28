/* Copyright (c) 2026 bhgv
 * EN: A bounded duplex mailbox for typed module results and reverse PID grants.
 * RU: Двусторонняя очередь типизированных результатов модуля и полномочий встречного PID.
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


/* EN: Send a typed frame to the peer. MORE if full or no rendezvous receiver.
 * RU: Отправить типизированный кадр партнёру. MORE при заполнении или без получателя rendezvous. */
StyxRpcStatus styxrpc_channel_mailbox_send(StyxRpcChannelMailbox *mailbox, const void *bytes, size_t size);


/* EN: Receive one frame. MORE if idle. Rendezvous requires STYXRPC_MANAGED_LIMIT bytes.
 * RU: Принять кадр. MORE без готового значения. Для rendezvous нужен буфер STYXRPC_MANAGED_LIMIT. */
StyxRpcStatus styxrpc_channel_mailbox_receive(StyxRpcChannelMailbox *mailbox,
    void *bytes, size_t capacity, size_t *size);


/* EN: Create a typed result mailbox with 0..8 slots per direction and replies at offset zero.
 * Return one owner reference independently of the output resource. Zero is rendezvous.
 * RU: Создать очередь результата с 0..8 слотами на направление и ответом от нуля.
 * Вернуть отдельную ссылку владельца. Ноль выбирает rendezvous. */
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
