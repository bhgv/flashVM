/* Copyright (c) 2026 bhgv
 * EN: Exercise a wire Int32 payload as an ordinary ActionScript variable.
 * RU: Проверить сетевой дескриптор Int32 как обычную переменную ActionScript.
 */
#include "../../src/styxrpc/player_services.h"
#include "flashvm/execution.h"
#include "flashvm/extensions.h"
#include "flashvm/gc.h"
#include "flashvm/styxrpc_session.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "styxrpc/variable_ref.h"
#include "client.h"
#include "drivers/posix/fd.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>


/* EN/RU: Read one cell from its owner. / Прочитать ячейку владельца. */
static int load(void *state, int32_t *out)
{
    *out = *(int32_t *)state;
    return 1;
}


/* EN/RU: Write one cell on its owner. / Записать ячейку владельца. */
static int store(void *state, int32_t value)
{
    *(int32_t *)state = value;
    return 1;
}


/* EN/RU: Run the issuer until disconnect. / Обслуживать владельца до отключения. */
static void *serve(void *server)
{
    while (styxrpc_server_step(server))
    {
    }
    return NULL;
}


/* EN: Pass an array as a script would, then use the two aliases without member access.
 * RU: Передать массив как из сценария, затем использовать две привязки без членов. */
static int script(FVM_Context *context, StyxClient *client, StyxRpcIntRef *reference, FVM_StyxRpcSession *binding)
{
    FVM_Value object, args, values[STYXRPC_INT_REF_SIZE], ref, result;
    FVM_Value namespace_value, carrier, wrapper;
    unsigned char bytes[STYXRPC_INT_REF_SIZE];
    if (!styxrpc_int_ref_encode(reference, bytes, sizeof(bytes)) ||
        fvm_styxrpc_session_bind(context, binding, client, NULL, &object) != FVM_OK ||
        fvm_set_global(context, "styxrpc", &object) != FVM_OK)
    {
        return 0;
    }
    for (size_t i = 0; i < sizeof(bytes); ++i)
    {
        fvm_value_number(values + i, bytes[i]);
    }
    fvm_value_number(&result, 1);
    int ok = fvm_extensions_install(context) == FVM_OK &&
        fvm_get_global(context, "channel", &namespace_value) == FVM_OK &&
        fvm_object_call(context, namespace_value.object, "create", 1, &result, &carrier) == FVM_OK &&
        fvm_object_call(context, object.object, "intRefChannel", 1, &carrier, &wrapper) == FVM_OK &&
        fvm_set_global(context, "refChannel", &wrapper) == FVM_OK &&
        fvm_exec_literal_array(context, sizeof(bytes), values, &args) == FVM_OK &&
        fvm_set_global(context, "wire", &args) == FVM_OK &&
        fvm_object_call(context, object.object, "acceptIntRef", 1, &args, &ref) == FVM_OK &&
        fvm_set_global(context, "first", &ref) == FVM_OK &&
        fvm_object_call(context, wrapper.object, "send", 1, &ref, &result) == FVM_OK &&
        result.type == FVM_VALUE_BOOLEAN && result.boolean &&
        fvm_object_call(context, wrapper.object, "recv", 0, NULL, &ref) == FVM_OK &&
        fvm_set_global(context, "second", &ref) == FVM_OK &&
        fvm_gc_collect(context, NULL) == FVM_OK &&
        fvm_get_global(context, "first", &result) == FVM_OK &&
        result.type == FVM_VALUE_NUMBER && result.number == 41;
    fvm_value_number(&result, -32);
    ok = ok && fvm_set_global(context, "first", &result) == FVM_OK &&
        fvm_get_global(context, "second", &result) == FVM_OK &&
        result.type == FVM_VALUE_NUMBER && result.number == -32;
    return ok;
}


/* EN: Execute the SWF optionally supplied by the project compiler pipeline in a fresh movie.
 * RU: Выполнить SWF, переданный из штатной цепочки компиляции, в новом фильме. */
static int compiled_movie(FVM_Runtime *runtime, StyxClient *client, const char *path, const StyxRpcIntRef *reference)
{
    unsigned char bytes[65536];
    FILE *file = fopen(path, "rb");
    if (!file)
    {
        return 0;
    }
    size_t size = fread(bytes, 1, sizeof(bytes), file);
    int complete = feof(file) && !ferror(file);
    fclose(file);
    FVM_Context *context = NULL;
    FVM_StyxRpcPlayerServices *services = NULL;
    FVM_Object *root = NULL;
    FVM_Value before, after, bytes_values[STYXRPC_INT_REF_SIZE], wire;
    unsigned char descriptor[STYXRPC_INT_REF_SIZE];
    if (!styxrpc_int_ref_encode(reference, descriptor, sizeof(descriptor)))
    {
        return 0;
    }
    for (size_t i = 0; i < sizeof(descriptor); ++i)
    {
        fvm_value_number(bytes_values + i, descriptor[i]);
    }
    int ok = complete && fvm_context_create(runtime, &context) == FVM_OK &&
        fvm_load_memory(context, bytes, size) == FVM_OK &&
        fvm_standard_install(context) == FVM_OK &&
        fvm_styxrpc_player_services(context, client, NULL, &services) == FVM_OK &&
        fvm_exec_literal_array(context, sizeof(descriptor), bytes_values, &wire) == FVM_OK &&
        fvm_set_global(context, "wireRef", &wire) == FVM_OK &&
        fvm_movieclip_load_swf(context, bytes, size, "_root", &root) == FVM_OK &&
        fvm_gc_pin(context, root) == FVM_OK &&
        fvm_movieclip_advance(context, root, 2) == FVM_OK &&
        fvm_object_get(context, root, "proofBefore", &before) == FVM_OK &&
        before.type == FVM_VALUE_NUMBER && before.number == -32 &&
        fvm_object_get(context, root, "proofAfter", &after) == FVM_OK &&
        after.type == FVM_VALUE_NUMBER && after.number == 111;
    fvm_styxrpc_player_disarm(services);
    fvm_context_destroy(context);
    free(services);
    return ok;
}


/* EN: Run the bound reference on one real carrier and close every owner.
 * RU: Проверить привязку на настоящем потоке и закрыть каждого владельца. */
int main(int argc, char **argv)
{
    if (argc > 2)
    {
        fputs("usage: flashvm_tests_styxrpc_variable_ref [peer-newchan.swf]\n", stderr);
        return 1;
    }
    StyxRpcRegistration nodes[6];
    StyxRpcPrograms programs;
    int sockets[2];
    if (!styxrpc_programs_init(&programs, nodes, 6) || socketpair(AF_UNIX, SOCK_STREAM, 0, sockets))
    {
        return 1;
    }
    StyxRpcServer *server = styxrpc_program_server_create(
        phyposixfd(sockets[1], PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), &programs);
    int32_t cell = 41;
    const StyxIntVariable variable = {&cell, load, store};
    StyxRpcIntRef reference;
    pthread_t worker;
    if (!server || !styxrpc_int_ref_grant(server, &variable, 1, &reference) ||
        pthread_create(&worker, NULL, serve, server))
    {
        close(sockets[0]);
        styxrpc_server_destroy(server);
        return 1;
    }
    StyxClient *client = styxclientmountfd(sockets[0], 4096, "test", "", 1000, 8);
    FVM_Config config;
    FVM_Runtime *runtime = NULL;
    FVM_Context *context = NULL;
    FVM_StyxRpcSession binding = {0};
    int ok = client && fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK &&
        fvm_context_create(runtime, &context) == FVM_OK && script(context, client, &reference, &binding);
    fvm_styxrpc_session_close(&binding);
    fvm_context_destroy(context);
    if (ok && argc == 2)
    {
        ok = compiled_movie(runtime, client, argv[1], &reference);
    }
    fvm_runtime_destroy(runtime);
    styxclientshutdown(client);
    styxclientfree(client);
    pthread_join(worker, NULL);
    styxrpc_server_destroy(server);
    if (!ok || cell != (argc == 2 ? 111 : -32))
    {
        fputs("FlashVM did not round-trip values and live wire references\n", stderr);
        return 1;
    }
    return 0;
}
