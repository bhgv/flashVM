/* Copyright (c) 2026 bhgv
 * EN: Mounted launch's owner-local mixed UniBus port leases.
 * RU: Жетоны портов смешанных сообщений UniBus у владельца смонтированного запуска.
 */
#ifndef DISVM_PAIRED_EXCHANGE_H
#define DISVM_PAIRED_EXCHANGE_H
#include "styxrpc/exchange.h"

#define DVM_EXCHANGE_SLOTS 8U /* Outstanding ports for one mounted guest. */

typedef struct DvmPairedPort
{
    StyxRpcExchange *exchange; /* Owned native mailbox. */
    StyxRpcMessagePort *wire;  /* Owned client fid and stream buffers. */
    int lease;                /* Positive non-reusable guest identifier. */
} DvmPairedPort;

typedef struct DvmPairedExchange
{
    DvmPairedPort ports[DVM_EXCHANGE_SLOTS]; /* Operated by the launch root. */
    int last;                              /* Most recently granted lease. */
} DvmPairedExchange;


/* EN: Dispatch one generated typed builtin frame while its original caller is pinned.
 * RU: Выполнить один сгенерированный builtin-вызов, пока удерживается вызывающий. */
int dvm_paired_exchange_invoke(DvmPairedExchange *exchange, struct StyxClient *client,
    unsigned method, void *frame);


/* EN: Close all ports after the launch and before the borrowed client is released.
 * RU: Закрыть все порты после выполнения запуска, до освобождения клиента. */
void dvm_paired_exchange_finish(DvmPairedExchange *exchange);
#endif
