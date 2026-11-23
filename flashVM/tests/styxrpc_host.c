/* Copyright (c) 2026 bhgv
 * EN: Exercise the public FlashVM-StyxRpc adapter against a separate hosted DisVM.
 * RU: Проверка публичного адаптера FlashVM-StyxRpc с отдельной DisVM.
 */
#include "flashvm/avm1.h"
#include "flashvm/exception.h"
#include "flashvm/function.h"
#include "flashvm/styxrpc.h"
#include "styxrpc/program.h"
#include "styxrpc/scalar_client.h"
#include "styxrpc/scalar_module.h"
#include "client.h"
#include "drivers/posix/fd.h"
#include "mux/bidir.h"
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HOST_MSIZE 4096U    /* Maximum negotiated Styx message size. */
#define HOST_TIMEOUT 3000U /* Per-request transport timeout in milliseconds. */
#define HOST_TAGS 8U       /* Maximum concurrent transport requests. */
#define HOST_MUX_LIMIT 65536U /* Bidirectional carrier frame capacity. */

#define ACTION_END 0x00           /* End the top-level action block. */
#define ACTION_ADD 0x0a           /* Add two numbers. */
#define ACTION_GET_VARIABLE 0x1c  /* Resolve the parameter by name. */
#define ACTION_SET_VARIABLE 0x1d  /* Store under the pending result name. */
#define ACTION_THROW 0x2a         /* Raise the top stack value in AVM1. */
#define ACTION_CALL_FUNCTION 0x3d /* Call the registered global function. */
#define ACTION_RETURN 0x3e        /* Return the top stack value. */
#define ACTION_PUSH 0x96          /* Push a record of typed stack operands. */
#define PUSH_STRING 0x00          /* NUL-terminated name operand. */
#define PUSH_INTEGER 0x07         /* Little-endian integer operand. */


/* EN: Define a real AVM1 body and retain it under name in the context globals.
 * RU: Определить тело AVM1 и удерживать его под именем name в глобалах. */
static int define_export(FVM_Context *context, const char *name, const void *body, size_t size)
{
    const char *parameters[] = {"n"};
    FVM_Object *function = NULL;
    FVM_Value value;
    FVM_Result status = fvm_function_create_avm1(context, body, size, 1, parameters, NULL, &function);
    if (status != FVM_OK)
    {
        fprintf(stderr, "Cannot create AVM1 export: %d\n", status);
        return 0;
    }
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = function;
    return fvm_set_global(context, name, &value) == FVM_OK;
}


/* EN: Publish the FlashVM context on an owned raw connected fd and serve on its owner thread.
 * RU: Опубликовать контекст FlashVM на выданном fd и обслуживать его в потоке владельца. */
static int serve_flash(FVM_Context *context, int fd)
{
    static const unsigned char next[] = {
        ACTION_PUSH, 3, 0, PUSH_STRING, 'n', 0, ACTION_GET_VARIABLE,
        ACTION_PUSH, 5, 0, PUSH_INTEGER, 1, 0, 0, 0, ACTION_ADD, ACTION_RETURN, ACTION_END
    };
    static const unsigned char reject[] = {
        ACTION_PUSH, 3, 0, PUSH_STRING, 'x', 0, ACTION_THROW, ACTION_END
    };
    FVM_StyxRpcExport bindings[] = {{context, "next"}, {context, "reject"}};
    StyxRpcExport descriptors[sizeof(bindings) / sizeof(*bindings)];
    StyxRpcScalarModule module;
    if (!define_export(context, "next", next, sizeof(next)) ||
        !define_export(context, "reject", reject, sizeof(reject)) ||
        fvm_styxrpc_export_int(bindings, "next", descriptors) != FVM_OK ||
        fvm_styxrpc_export_int(bindings + 1, "reject", descriptors + 1) != FVM_OK ||
        styxrpc_scalar_module_init(&module, "flash.dis", descriptors, sizeof(descriptors) / sizeof(*descriptors)) != STYXRPC_OK)
    {
        fputs("FlashVM export setup failed\n", stderr);
        return 0;
    }
    RemotePhy *phy = phyposixfd(fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE);
    StyxRpcServer *server = styxrpc_server_create(phy, &styxrpc_scalar_module_tree, &module);
    if (!server)
    {
        return 0;
    }
    while (styxrpc_server_step(server))
    {
    }
    styxrpc_server_destroy(server);
    return 1;
}


/* EN: Call the import through AVM1 and check explicit failures followed by a fresh call.
 * RU: Вызвать импорт из AVM1, проверить ошибки и новый вызов после них. */
static int exercise_import(FVM_Context *context, FVM_StyxRpcImport *binding)
{
    static const unsigned char code[] = {
        ACTION_PUSH, 24, 0, PUSH_STRING, 'r', 'p', 'c', 'R', 'e', 's', 'u', 'l', 't', 0,
        PUSH_INTEGER, 41, 0, 0, 0, PUSH_INTEGER, 1, 0, 0, 0,
        PUSH_STRING, 'r', 0, ACTION_CALL_FUNCTION, ACTION_SET_VARIABLE, ACTION_END
    };
    FVM_Value value;
    FVM_Value argument;
    int64_t sentinel = 17;
    FVM_Result registered = fvm_styxrpc_import_int(context, "r", binding);
    FVM_Result executed = registered == FVM_OK ? fvm_avm1_execute(context, code, sizeof(code)) : registered;
    if (executed != FVM_OK &&
        fvm_exception_get(context, &value) == FVM_OK && value.type == FVM_VALUE_STRING)
    {
        fprintf(stderr, "AVM1 RPC import failed: %s\n", value.string);
    }
    if (executed != FVM_OK ||
        fvm_get_global(context, "rpcResult", &value) != FVM_OK ||
        value.type != FVM_VALUE_NUMBER || value.number != 42 ||
        styxrpc_scalar_invoke(binding->session, binding->module, binding->name, STYXRPC_BIG_SIGNATURE,
            41, HOST_TIMEOUT, &sentinel) != STYXRPC_CALL_NOT_SENT || sentinel != 17)
    {
        return 0;
    }
    fvm_value_number(&argument, 0.5);
    if (fvm_call_bytes(context, "r", 1, 1, &argument, &value) != FVM_EXCEPTION ||
        fvm_exception_get(context, &value) != FVM_OK || value.type != FVM_VALUE_STRING ||
        strcmp(value.string, "Styx RPC requires one exact int32 argument") ||
        fvm_exception_clear(context) != FVM_OK)
    {
        return 0;
    }
    fvm_value_number(&argument, 0);
    if (fvm_call_bytes(context, "r", 1, 1, &argument, &value) != FVM_EXCEPTION ||
        fvm_exception_get(context, &value) != FVM_OK || value.type != FVM_VALUE_STRING ||
        strcmp(value.string, "Styx RPC remote function failed") || fvm_exception_clear(context) != FVM_OK)
    {
        return 0;
    }
    fvm_value_number(&argument, -10);
    return fvm_call_bytes(context, "r", 1, 1, &argument, &value) == FVM_OK &&
        value.type == FVM_VALUE_NUMBER && value.number == -9;
}


/* EN: Serve the reverse handshake until the DisVM session closes.
 * RU: Обслуживать встречный поток до закрытия сессии DisVM. */
static void *serve_reverse(void *state)
{
    StyxRpcServer *server = state;
    while (styxrpc_server_step(server))
    {
    }
    styxrpc_server_destroy(server);
    return NULL;
}


/* EN: Consume fd, mount the Dis provider and serve its reverse handshake concurrently.
 * RU: Принять fd, смонтировать провайдера Dis и параллельно обслуживать встречный поток. */
static int call_dis(FVM_Context *context, FVM_StyxRpcImport *binding, int fd)
{
    PhyBidir *mux = phybidircreate(phyposixfd(fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), HOST_MUX_LIMIT);
    if (!mux)
    {
        return 0;
    }
    StyxRpcPrograms programs;
    StyxRpcRegistration storage[2];
    StyxRpcServer *reverse = NULL;
    if (styxrpc_programs_init(&programs, storage, sizeof(storage) / sizeof(*storage)))
    {
        reverse = styxrpc_server_create(phybidirchannel(mux, 1), &styxrpc_program_tree, &programs);
    }
    pthread_t thread;
    if (!reverse || pthread_create(&thread, NULL, serve_reverse, reverse))
    {
        styxrpc_server_destroy(reverse);
        phybidirfree(mux);
        fputs("FlashVM reverse tree failed\n", stderr);
        return 0;
    }
    StyxClient *session = styxclientmountphy(phybidirchannel(mux, 0), HOST_MSIZE, "flash",
        "", HOST_TIMEOUT, HOST_TAGS);
    if (!session)
    {
        fputs("Cannot mount DisVM provider\n", stderr);
    }
    binding->session = session;
    int ok = session && exercise_import(context, binding);
    binding->session = NULL;
    styxclientshutdown(session);
    styxclientfree(session);
    phybidirfree(mux);
    pthread_join(thread, NULL);
    return ok;
}


/* EN: Create a dedicated interpreter context and run the selected test role.
 * RU: Создать выделенный контекст интерпретатора и выполнить выбранную роль проверки. */
int main(int argc, char **argv)
{
    char *end;
    long fd = argc == 3 ? strtol(argv[2], &end, 10) : -1;
    if (fd < 3 || fd > INT_MAX || *end || (strcmp(argv[1], "serve") && strcmp(argv[1], "call")))
    {
        return 2;
    }
    FVM_Config config;
    FVM_Runtime *runtime = NULL;
    FVM_Context *context = NULL;
    if (fvm_config_init(&config) != FVM_OK || fvm_runtime_create(&config, &runtime) != FVM_OK)
    {
        fputs("Cannot create FlashVM runtime\n", stderr);
        return 1;
    }
    int ok = 0;
    FVM_StyxRpcImport binding = {NULL, "mounted/limbo.dis", "next", HOST_TIMEOUT};
    if (fvm_context_create(runtime, &context) == FVM_OK)
    {
        ok = !strcmp(argv[1], "serve") ? serve_flash(context, (int)fd) : call_dis(context, &binding, (int)fd);
        fvm_context_destroy(context);
    }
    fvm_runtime_destroy(runtime);
    if (ok)
    {
        puts("PASS FlashVM Styx RPC host");
    }
    else
    {
        fputs("FlashVM RPC host failed\n", stderr);
    }
    return ok ? 0 : 1;
}
