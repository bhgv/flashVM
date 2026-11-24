/* Copyright (c) 2026 bhgv
 * EN: Credited single-frame terminal mailbox over the public UniBus message port.
 * RU: Однокадровая очередь терминала с подтверждением через публичный порт UniBus.
 */
#ifndef STYXRPC_TERMINAL_ADAPTER_H
#define STYXRPC_TERMINAL_ADAPTER_H
#include "styxrpc/messages.h"

#define STYXRPC_TERMINAL_FRAME "_inferno.fvd1"     /* Counted existing terminal envelope. */
#define STYXRPC_TERMINAL_CREDIT "_inferno.credit" /* One empty record permits one frame. */
#define STYXRPC_TERMINAL_MAX 1077U               /* Header 53 plus at most 1024 text bytes. */

typedef struct StyxRpcTerminal
{
    StyxRpcMessagePort *port; /* Exact borrowed port, held by the adapter owner. */
    unsigned char bytes[STYXRPC_TERMINAL_MAX]; /* Copied pending inbound frame. */
    size_t size; /* Zero until one inbound frame is ready. */
    int writable; /* The peer granted one frame of send credit. */
    int granted; /* We granted credit for one inbound frame. */
} StyxRpcTerminal;


/* EN: Describe both direction-local tags for terminal; copy the resulting types on open.
 * RU: Описать два тега для terminal; полученные types копируются при открытии порта. */
void styxrpc_terminal_types(StyxRpcTerminal *terminal, StyxRpcMessageType *types);

/* EN: Grant one frame when the mailbox is empty; MORE means a slot is already occupied or granted.
 * RU: Разрешить один кадр при пустой очереди; MORE означает занятое или выданное место. */
StyxRpcStatus styxrpc_terminal_allow(StyxRpcTerminal *terminal);

/* EN: Copy a frame while there is credit; OK consumes the credit.
 * RU: Скопировать кадр при наличии разрешения; OK расходует разрешение. */
StyxRpcStatus styxrpc_terminal_send(StyxRpcTerminal *terminal, const void *bytes, size_t size);

#endif
