/* Copyright (c) 2026 bhgv
 * EN: Call a named export on the existing script session.
 * RU: Вызов именованного экспорта через существующую сессию AS.
 */
#include "session_methods.h"
#include "module_import.h"
#include "flashvm/styxrpc.h"
#include "styxrpc/serve.h"
#include "styxrpc/vfs_client.h"
#include <string.h>

#define FVM_SESSION_CALL_WAIT_MS 1000U /* Reply polling budget after submission. */


/* [ASINC] EN: args are module path, export name and exact int32. Use one
 * synchronous request on the borrowed session and propagate its exception.
 * RU: args содержит путь модуля, имя экспорта и точный int32. Выполнить один
 * синхронный запрос на заимствованной сессии, передать исключение вызывающему. */
static FVM_Result call_int(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    FVM_StyxRpcSession *session = state;
    char path[STYXRPC_VFS_PATH_MAX + 1];
    char name[STYXRPC_TREE_NAME + 1];
    FVM_StyxRpcImport target;
    if (argc != 3 || !session->client || !fvm_styxrpc_name(args, path, sizeof(path)) ||
        !fvm_styxrpc_name(args + 1, name, sizeof(name)) || strchr(name, '/'))
    {
        return FVM_INVALID_ARGUMENT;
    }
    target.session = session->client;
    target.module = path;
    target.name = name;
    target.wait_ms = FVM_SESSION_CALL_WAIT_MS;
    return fvm_styxrpc_invoke_int(context, self, 1, args + 2, out, &target);
}


/* EN/RU: See session_methods.h. / См. session_methods.h. */
FVM_Result fvm_styxrpc_call_members(FVM_Context *context, FVM_Object *object, FVM_StyxRpcSession *binding)
{
    return fvm_object_define_method_rooted(context, object, "callInt", call_int, binding, NULL);
}
