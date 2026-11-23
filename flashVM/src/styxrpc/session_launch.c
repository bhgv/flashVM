/* Copyright (c) 2026 bhgv
 * EN: Admit one authorized named Dis launch; expose receipt and exact cancellation.
 * RU: Именованный запуск разрешённого Dis, его квитанция и точная отмена.
 */
#include "session_methods.h"


/* EN: Find the exact positive token on a still-owned launch fid.
 * RU: Найти точный положительный токен действующего fid запуска. */
static FVM_StyxRpcTicket *find_ticket(FVM_StyxRpcSession *binding, const FVM_Value *value)
{
    uint64_t token;
    unsigned i;
    if (!binding->client || !fvm_styxrpc_number(value, UINT32_MAX, &token) || !token)
    {
        return NULL;
    }
    for (i = 0; i < FVM_STYXRPC_LAUNCH_SLOTS; ++i)
    {
        if (binding->tickets[i].fid && binding->tickets[i].token == token)
        {
            return binding->tickets + i;
        }
    }
    return NULL;
}


/* EN: Submit the named program once, returning a positive token or an admission outcome.
 * RU: Подать один именованный запуск и вернуть положительный токен либо статус отказа. */
static FVM_Result launch(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *result, void *state)
{
    FVM_StyxRpcSession *binding = state;
    char name[STYXRPC_NAMED_NAME_MAX + 1];
    unsigned char frame[STYXRPC_NAMED_FRAME_MAX];
    unsigned i;
    StyxRpcLaunchStatus status;
    FVM_StyxRpcTicket *ticket;
    (void)context;
    (void)self;
    if (argc != 2 || args[1].type != FVM_VALUE_STRING ||
        !fvm_styxrpc_name(args, name, sizeof(name)))
    {
        return FVM_INVALID_ARGUMENT;
    }
    for (i = 0; i < FVM_STYXRPC_LAUNCH_SLOTS && binding->tickets[i].fid; ++i)
    {
    }
    if (!binding->client || !binding->program.pid || i == FVM_STYXRPC_LAUNCH_SLOTS ||
        binding->last_token == UINT32_MAX)
    {
        fvm_value_number(result, STYXRPC_LAUNCH_NOT_SENT);
        return FVM_OK;
    }
    ticket = binding->tickets + i;
    status = styxrpc_launch_named(binding->client, name, &binding->program, args[1].string,
        fvm_value_string_size(args + 1), frame, sizeof(frame), &ticket->fid);
    if (status != STYXRPC_LAUNCH_PENDING)
    {
        fvm_value_number(result, status);
        return FVM_OK;
    }
    ticket->token = ++binding->last_token;
    ticket->status = status;
    fvm_value_number(result, (double)ticket->token);
    return FVM_OK;
}


/* EN: Read one completion receipt for an existing token; preserve terminal results.
 * RU: Прочитать квитанцию существующего токена; сохранить конечный результат. */
static FVM_Result status(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *result, void *state)
{
    FVM_StyxRpcTicket *ticket = argc == 1 ? find_ticket(state, args) : NULL;
    StyxRpcLaunchStatus current;
    (void)context;
    (void)self;
    if (!ticket)
    {
        return FVM_INVALID_ARGUMENT;
    }
    current = ticket->status;
    if (current == STYXRPC_LAUNCH_PENDING)
    {
        current = styxrpc_launch_named_status(ticket->fid);
        if (current == STYXRPC_LAUNCH_OK || current == STYXRPC_LAUNCH_FAILED)
        {
            ticket->status = current;
        }
    }
    fvm_value_number(result, current);
    return FVM_OK;
}


/* EN: Clunk exactly this ticket. Other fids and their tokens remain independent.
 * RU: Закрыть точный fid этого запуска; остальные fid и токены независимы. */
static FVM_Result close_ticket(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *result, void *state)
{
    FVM_StyxRpcTicket *ticket = argc == 1 ? find_ticket(state, args) : NULL;
    (void)context;
    (void)self;
    if (!ticket)
    {
        return FVM_INVALID_ARGUMENT;
    }
    styxrpc_launch_named_close(&ticket->fid);
    ticket->token = 0;
    fvm_value_boolean(result, 1);
    return FVM_OK;
}


/* EN/RU: See session_methods.h. / См. session_methods.h. */
FVM_Result fvm_styxrpc_launch_members(FVM_Context *context, FVM_Object *object, FVM_StyxRpcSession *binding)
{
    FVM_Result result = fvm_object_define_method_rooted(context, object, "launch", launch, binding, NULL);
    if (result == FVM_OK)
    {
        result = fvm_object_define_method_rooted(context, object, "status", status, binding, NULL);
    }
    if (result == FVM_OK)
    {
        result = fvm_object_define_method_rooted(context, object, "close", close_ticket, binding, NULL);
    }
    return result;
}
