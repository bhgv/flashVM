/* Copyright (c) 2026 bhgv
 * EN: Common script message descriptors and payload conversion for both roles.
 * RU: Общие описатели сообщений AS и преобразование данных в обоих направлениях.
 */
#ifndef FLASHVM_STYXRPC_SESSION_MESSAGE_TYPES_H
#define FLASHVM_STYXRPC_SESSION_MESSAGE_TYPES_H
#include "flashvm/value.h"
#include "styxrpc/messages.h"

typedef struct ScriptMessageNames
{
    char type[STYXRPC_MESSAGE_NAME_MAX + 1];  /* Wire schema identity. */
    char method[STYXRPC_TREE_NAME + 1];       /* Empty for send-only types. */
} ScriptMessageNames;


/* [GC:] EN: Parse value into names and the SDK registration with borrowed callback state.
 * RU: Разобрать value в names и регистрацию SDK с заимствованным состоянием колбэка. */
FVM_Result fvm_styxrpc_message_descriptor(FVM_Context *context, const FVM_Value *value,
    ScriptMessageNames *names, StyxRpcMessageHandler receive, void *cookie, StyxRpcMessageType *type);


/* [GC:] EN: Convert method arguments and queue one payload if *port is still live.
 * Return the SDK queue status as a script Number after accepted arguments.
 * RU: Преобразовать аргументы метода и поставить payload в очередь, если *port жив.
 * При допустимых аргументах вернуть статус очереди SDK как Number. */
FVM_Result fvm_styxrpc_message_queue(FVM_Context *context, StyxRpcMessagePort **port,
    unsigned argc, const FVM_Value *args, FVM_Value *out);


/* [GC:] EN: Invoke receiver.method with a copied byte array and optional reply object.
 * RU: Вызвать receiver.method с копией массива байтов и необязательным объектом ответа. */
FVM_Result fvm_styxrpc_message_dispatch(FVM_Context *context, const FVM_Value *receiver,
    const char *method, const unsigned char *bytes, size_t size, const FVM_Value *reply);

#endif
