/* Copyright (c) 2026 bhgv
 * EN: Enforce one outstanding frame per direction on the public UniBus message port.
 * RU: Поддержка одного ожидающего кадра на направление через публичный порт UniBus.
 */
#include "styxrpc/unibus_terminal.h"
#include <string.h>


/* EN: Receive one checked transport frame, bounded by previously issued credit.
 * RU: Принять один проверенный кадр в пределах ранее выданного разрешения. */
static int receive_frame(StyxRpcMessagePort *port, void *context, const void *bytes, size_t size)
{
    StyxRpcTerminal *terminal = context;
    if (terminal->port != port || !terminal->granted || terminal->size ||
        size < 53 || size > sizeof(terminal->bytes) || memcmp(bytes, "fvd1:", 5))
    {
        return 0;
    }
    memcpy(terminal->bytes, bytes, size);
    terminal->size = size;
    terminal->granted = 0;
    return 1;
}


/* EN: Accept one empty credit from the exact peer. Duplicate credits retire that port.
 * RU: Принять одно пустое разрешение от точной стороны. Повтор разрешения закрывает порт. */
static int receive_credit(StyxRpcMessagePort *port, void *context, const void *bytes, size_t size)
{
    StyxRpcTerminal *terminal = context;
    (void)bytes;
    if (terminal->port != port || size || terminal->writable)
    {
        return 0;
    }
    terminal->writable = 1;
    return 1;
}


/* EN: Describe both direction-local tags for this terminal's exact port.
 * RU: Описать оба тега направления для точного порта терминала. */
void styxrpc_terminal_types(StyxRpcTerminal *terminal, StyxRpcMessageType *types)
{
    types[0].name = STYXRPC_TERMINAL_FRAME;
    types[0].version = 1;
    types[0].send_tag = 1;
    types[0].receive = receive_frame;
    types[0].context = terminal;
    types[1].name = STYXRPC_TERMINAL_CREDIT;
    types[1].version = 1;
    types[1].send_tag = 2;
    types[1].receive = receive_credit;
    types[1].context = terminal;
}


/* EN: Grant one frame when the local mailbox is empty. Preserve credit on retry/failure.
 * RU: Разрешить один кадр, когда локальная очередь пуста. Сохранить состояние при отказе. */
StyxRpcStatus styxrpc_terminal_allow(StyxRpcTerminal *terminal)
{
    StyxRpcStatus result;
    if (!terminal->port || terminal->size || terminal->granted)
    {
        return STYXRPC_MORE;
    }
    result = styxrpc_message_send(terminal->port, STYXRPC_TERMINAL_CREDIT, 1, NULL, 0);
    terminal->granted = result == STYXRPC_OK;
    return result;
}


/* EN: Copy one frame only if the peer has room. Queue success uses that credit.
 * RU: Скопировать кадр только при наличии места у получателя. Успех расходует разрешение. */
StyxRpcStatus styxrpc_terminal_send(StyxRpcTerminal *terminal, const void *bytes, size_t size)
{
    StyxRpcStatus result;
    if (!terminal->port || !terminal->writable)
    {
        return STYXRPC_MORE;
    }
    if (size < 53 || size > STYXRPC_TERMINAL_MAX || !bytes || memcmp(bytes, "fvd1:", 5))
    {
        return STYXRPC_INVALID;
    }
    result = styxrpc_message_send(terminal->port, STYXRPC_TERMINAL_FRAME, 1, bytes, size);
    if (result == STYXRPC_OK)
    {
        terminal->writable = 0;
    }
    return result;
}
