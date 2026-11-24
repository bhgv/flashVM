/* Copyright (c) 2026 bhgv
 * EN: Copy AVM1 terminal strings over one hosted UniBus endpoint.
 * RU: Передача копий строк терминала AVM1 через один узел UniBus.
 */
#include "player_terminal.h"
#include "flashvm/gc_roots.h"
#include <string.h>


/* EN: Claim the single UI port and advertise one receive slot.
 * RU: Занять единственный UI-порт и объявить одно место приёма. */
static void opened(StyxRpcMessagePort *port, void *context)
{
    StyxRpcTerminal *terminal = context;
    if (!terminal->port)
    {
        terminal->port = port;
        styxrpc_terminal_allow(terminal);
    }
}


/* EN: Forget only the exact port that owns this terminal.
 * RU: Очистить терминал только при закрытии его собственного порта. */
static void closed(StyxRpcMessagePort *port, void *context)
{
    StyxRpcTerminal *terminal = context;
    if (terminal->port == port)
    {
        memset(terminal, 0, sizeof(*terminal));
    }
}


/* EN/RU: See player_terminal.h. / См. player_terminal.h. */
int fvm_styxrpc_player_publish_terminal(StyxRpcServer *server, StyxRpcTerminal *terminal)
{
    StyxRpcMessageType types[2];
    StyxRpcMessageSpec spec;
    styxrpc_terminal_types(terminal, types);
    spec.types = types;
    spec.count = 2;
    spec.opened = opened;
    spec.closed = closed;
    spec.context = terminal;
    return styxrpc_messages_publish(server, NULL, &spec) == STYXRPC_OK;
}


/* EN: Accept one counted script frame if the peer's slot has credit.
 * RU: Принять одну строку кадра AS при наличии места на другой стороне. */
static FVM_Result send_frame(FVM_Context *context, FVM_Object *object, unsigned argc,
    const FVM_Value *args, FVM_Value *result, void *state)
{
    StyxRpcTerminal *terminal = state;
    (void)context;
    (void)object;
    if (argc != 1 || args->type != FVM_VALUE_STRING)
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_boolean(result, styxrpc_terminal_send(terminal, args->string, fvm_value_string_size(args)) == STYXRPC_OK);
    return FVM_OK;
}


/* EN: Materialize one counted frame into context-owned string storage.
 * RU: Скопировать один кадр с длиной в хранилище строки контекста. */
static FVM_Result receive_frame(FVM_Context *context, FVM_Object *object, unsigned argc,
    const FVM_Value *args, FVM_Value *result, void *state)
{
    StyxRpcTerminal *terminal = state;
    FVM_Result status;
    FVM_Value bytes;
    unsigned short units[STYXRPC_TERMINAL_MAX];
    size_t count;
    (void)object;
    (void)args;
    if (argc)
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_undefined(result);
    if (!terminal->size)
    {
        return FVM_OK;
    }
    status = fvm_value_string_bytes(&bytes, (const char *)terminal->bytes, terminal->size);
    if (status == FVM_OK)
    {
        status = fvm_value_get_utf16(&bytes, units, sizeof(units) / sizeof(units[0]), &count);
    }
    if (status == FVM_OK)
    {
        status = fvm_value_string_utf16(context, units, count, result);
    }
    if (status == FVM_OK)
    {
        terminal->size = 0;
        styxrpc_terminal_allow(terminal);
    }
    return status;
}


/* EN/RU: See player_terminal.h. / См. player_terminal.h. */
FVM_Result fvm_styxrpc_player_terminal(FVM_Context *context, StyxRpcTerminal *terminal)
{
    FVM_GCRootScope *roots;
    FVM_Value object, bidir;
    FVM_Result status = fvm_gc_roots_begin(context, &roots);
    if (status != FVM_OK)
    {
        return status;
    }
    fvm_value_undefined(&object);
    status = fvm_object_create(context, &object.object);
    if (status == FVM_OK)
    {
        object.type = FVM_VALUE_OBJECT;
        status = fvm_gc_roots_add(roots, &object);
    }
    if (status == FVM_OK)
    {
        fvm_value_boolean(&bidir, 1);
        status = fvm_object_set(context, object.object, "bidir", &bidir);
    }
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(context, object.object, "trySendFrame", send_frame, terminal, NULL);
    }
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(context, object.object, "tryReceiveFrame", receive_frame, terminal, NULL);
    }
    if (status == FVM_OK)
    {
        status = fvm_set_global(context, "styxTerminal", &object);
    }
    fvm_gc_roots_end(roots);
    return status;
}
