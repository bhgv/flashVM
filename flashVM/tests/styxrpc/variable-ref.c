/* Copyright (c) 2026 bhgv
 * EN: Exercise a wire Int32 payload as an ordinary ActionScript variable.
 * RU: Проверить сетевой дескриптор Int32 как обычную переменную ActionScript.
 */
#include "../../src/styxrpc/player_services.h"
#include "flashvm/execution.h"
#include "flashvm/extensions.h"
#include "flashvm/gc.h"
#include "flashvm/gc_roots.h"
#include "flashvm/styxrpc_session.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "styxrpc/variable_ref.h"
#include "styxrpc/channel_messages.h"
#include "client.h"
#include "drivers/posix/fd.h"
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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


/* EN: Echo each payload under the same named schema, with the server's own tag.
 * RU: Вернуть каждый payload под той же схемой, с собственным тегом сервера. */
static int echo_record(StyxRpcMessagePort *port, void *name, const void *bytes, size_t size)
{
    return styxrpc_message_send(port, name, STYXRPC_CHANNEL_MESSAGE_VERSION, bytes, size) == STYXRPC_OK;
}


/* EN: Queue the owner's existing live cell as the first message on each open.
 * RU: Поставить живую ячейку владельца первым сообщением при каждом открытии. */
static void announce_ref(StyxRpcMessagePort *port, void *state)
{
    unsigned char bytes[STYXRPC_INT_REF_SIZE];
    if (styxrpc_int_ref_encode(state, bytes, sizeof(bytes)))
    {
        styxrpc_message_send(port, STYXRPC_CHANNEL_MESSAGE_REF, STYXRPC_CHANNEL_MESSAGE_VERSION,
            bytes, sizeof(bytes));
    }
}


/* EN: Publish a mixed exchange root; all server tags differ from the AS tags.
 * RU: Опубликовать корень обмена разных типов; все теги сервера отличаются от AS. */
static int publish_mixed(StyxRpcServer *server, StyxRpcIntRef *reference)
{
    const char *names[] = {STYXRPC_CHANNEL_MESSAGE_INT, STYXRPC_CHANNEL_MESSAGE_REAL,
        STYXRPC_CHANNEL_MESSAGE_BOOL, STYXRPC_CHANNEL_MESSAGE_TEXT, STYXRPC_CHANNEL_MESSAGE_NULL,
        STYXRPC_CHANNEL_MESSAGE_REF};
    StyxRpcMessageType types[sizeof(names) / sizeof(*names)];
    for (size_t i = 0; i < sizeof(types) / sizeof(*types); ++i)
    {
        types[i] = (StyxRpcMessageType){names[i], STYXRPC_CHANNEL_MESSAGE_VERSION,
            40U + (uint32_t)i, echo_record, (void *)names[i]};
    }
    const StyxRpcMessageSpec spec = {types, sizeof(types) / sizeof(*types), announce_ref, NULL, reference};
    return styxrpc_messages_publish(server, "mixed", &spec) == STYXRPC_OK;
}


/* EN: Send value on channel and require acknowledgement of the Styx write.
 * RU: Отправить value по channel и проверить подтверждение записи Styx. */
static int send_value(FVM_Context *context, FVM_Object *channel, const FVM_Value *value)
{
    FVM_Value result;
    return fvm_object_call(context, channel, "send", 1, value, &result) == FVM_OK &&
        result.type == FVM_VALUE_BOOLEAN && result.boolean;
}


/* EN: Read one ready result using the public AS method on channel.
 * RU: Прочитать один готовый результат публичным методом AS канала channel. */
static int recv_value(FVM_Context *context, FVM_Object *channel, FVM_Value *result)
{
    return fvm_object_call(context, channel, "recv", 0, NULL, result) == FVM_OK;
}


/* EN: Submit five different value kinds and check their FIFO round trip. Keep the long string fragmented.
 * RU: Отправить пять типов и проверить возврат в порядке FIFO. Длинная строка идёт фрагментами. */
static int mixed_values(FVM_Context *context, FVM_Object *channel)
{
    FVM_Value args, result;
    char text[9000];
    memset(text, 'x', sizeof(text));
    text[2] = 0;
    fvm_value_number(&args, -32);
    int ok = send_value(context, channel, &args);
    fvm_value_number(&args, -0.0);
    ok = ok && send_value(context, channel, &args);
    fvm_value_boolean(&args, 1);
    ok = ok && send_value(context, channel, &args);
    fvm_value_undefined(&args);
    args.type = FVM_VALUE_NULL;
    ok = ok && send_value(context, channel, &args) &&
        fvm_value_string_bytes(&args, text, sizeof(text)) == FVM_OK && send_value(context, channel, &args);
    ok = ok && recv_value(context, channel, &result) && result.type == FVM_VALUE_NUMBER && result.number == -32 &&
        recv_value(context, channel, &result) && result.type == FVM_VALUE_NUMBER &&
        result.number == 0 && signbit(result.number) &&
        recv_value(context, channel, &result) && result.type == FVM_VALUE_BOOLEAN && result.boolean &&
        recv_value(context, channel, &result) && result.type == FVM_VALUE_NULL &&
        recv_value(context, channel, &result) && result.type == FVM_VALUE_UNDEFINED;
    return ok && recv_value(context, channel, &result) && result.type == FVM_VALUE_STRING &&
        fvm_value_string_size(&result) == sizeof(text) && !memcmp(result.string, text, sizeof(text)) &&
        recv_value(context, channel, &result) && result.type == FVM_VALUE_UNDEFINED;
}


/* EN: Use the stock player global and share one transport fid for values and live aliases.
 * RU: Использовать штатную глобаль плеера и один fid для значений и живых ссылок. */
static int mixed_script(FVM_Context *context, StyxClient *client, FVM_StyxRpcPlayerServices **services)
{
    FVM_Value rpc, legacy, address, channel, ref, result;
    if (fvm_styxrpc_player_services(context, client, NULL, services) != FVM_OK ||
        fvm_get_global(context, "rpc", &rpc) != FVM_OK || rpc.type != FVM_VALUE_OBJECT ||
        fvm_get_global(context, "styxrpc", &legacy) != FVM_OK || rpc.object != legacy.object ||
        fvm_value_string(&address, "/mixed") != FVM_OK ||
        fvm_object_call(context, rpc.object, "openExchange", 1, &address, &channel) != FVM_OK ||
        fvm_set_global(context, "mixed", &channel) != FVM_OK)
    {
        return 0;
    }
    FVM_GCRootScope *roots;
    if (fvm_gc_roots_begin(context, &roots) != FVM_OK)
    {
        return 0;
    }
    fvm_value_undefined(&ref);
    /* The initial announcement precedes all snapshots. Forward the exact imported operand. */
    int ok = fvm_gc_roots_add(roots, &channel) == FVM_OK && fvm_gc_roots_add(roots, &ref) == FVM_OK &&
        recv_value(context, channel.object, &ref) &&
        fvm_set_global(context, "mixedFirst", &ref) == FVM_OK &&
        fvm_get_global(context, "mixedFirst", &result) == FVM_OK &&
        result.type == FVM_VALUE_NUMBER && result.number == -32 &&
        send_value(context, channel.object, &result) &&
        send_value(context, channel.object, &ref) &&
        recv_value(context, channel.object, &result) &&
        result.type == FVM_VALUE_NUMBER && result.number == -32 &&
        recv_value(context, channel.object, &ref) &&
        fvm_set_global(context, "mixedSecond", &ref) == FVM_OK &&
        mixed_values(context, channel.object) &&
        fvm_gc_collect(context, NULL) == FVM_OK;
    fvm_value_number(&result, 77);
    ok = ok && fvm_set_global(context, "mixedSecond", &result) == FVM_OK &&
        fvm_get_global(context, "mixedFirst", &result) == FVM_OK &&
        result.type == FVM_VALUE_NUMBER && result.number == 77 &&
        fvm_object_call(context, channel.object, "close", 0, NULL, &result) == FVM_OK &&
        fvm_object_call(context, channel.object, "close", 0, NULL, &result) == FVM_OK &&
        fvm_object_call(context, channel.object, "recv", 0, NULL, &result) == FVM_INVALID_ARGUMENT;
    fvm_gc_roots_end(roots);
    return ok;
}


/* EN: Execute the SWF optionally supplied by the project compiler pipeline in a fresh movie.
 * RU: Выполнить SWF, переданный из штатной цепочки компиляции, в новом фильме. */
static int compiled_movie(FVM_Runtime *runtime, StyxClient *client, const char *path)
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
    FVM_Value before, after;
    int ok = complete && fvm_context_create(runtime, &context) == FVM_OK &&
        fvm_load_memory(context, bytes, size) == FVM_OK &&
        fvm_standard_install(context) == FVM_OK &&
        fvm_styxrpc_player_services(context, client, NULL, &services) == FVM_OK &&
        fvm_movieclip_load_swf(context, bytes, size, "_root", &root) == FVM_OK &&
        fvm_gc_pin(context, root) == FVM_OK &&
        fvm_movieclip_advance(context, root, 2) == FVM_OK &&
        fvm_object_get(context, root, "proofBefore", &before) == FVM_OK &&
        before.type == FVM_VALUE_NUMBER && before.number == 77 &&
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
        !publish_mixed(server, &reference) ||
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
    FVM_StyxRpcPlayerServices *services = NULL;
    int ok = client && fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK &&
        fvm_context_create(runtime, &context) == FVM_OK && script(context, client, &reference, &binding) &&
        mixed_script(context, client, &services);
    fvm_styxrpc_session_close(&binding);
    fvm_styxrpc_player_disarm(services);
    fvm_context_destroy(context);
    free(services);
    if (ok && argc == 2)
    {
        ok = compiled_movie(runtime, client, argv[1]);
    }
    fvm_runtime_destroy(runtime);
    styxclientshutdown(client);
    styxclientfree(client);
    pthread_join(worker, NULL);
    styxrpc_server_destroy(server);
    if (!ok || cell != (argc == 2 ? 111 : 77))
    {
        fputs("FlashVM did not round-trip values and live wire references\n", stderr);
        return 1;
    }
    return 0;
}
