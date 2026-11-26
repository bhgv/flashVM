/* Copyright (c) 2026 bhgv
 * EN: Exercise default-session callback files over two actual Styx endpoints.
 * RU: Проверка файлов с колбэками штатной сессии через реальные соединения Styx.
 */
#include "movie-assets.h"
#include "../../src/styxrpc/player_services.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "styxrpc/nodes.h"
#include "styxrpc/vfs_client.h"
#include "drivers/posix/fd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct Script
{
    StyxRpcModuleSlot *slot; /* Owner slot retired via the control request. */
    unsigned char value;    /* Application state shared across separate requests. */
    unsigned int calls;     /* Exact handler invocation count on the VM owner. */
} Script;

typedef struct Peer
{
    int fd; /* Owned peer socket, consumed when attaching. */
    int ok; /* Result transferred to main at join. */
} Peer;


/* EN: Reject uploads in this fixture.
 * RU: Отклонить загрузки в этой проверке. */
static int upload(void *state, const void *bytes, size_t size)
{
    (void)state;
    (void)bytes;
    (void)size;
    return 0;
}


/* EN: Emulate a movie retirement when the peer sends restart.
 * RU: Смоделировать отзыв фильма по команде restart. */
static int command(void *state, StyxRpcControlCommand mode)
{
    if (mode != STYXRPC_CONTROL_RESTART)
    {
        return 0;
    }
    styxrpc_module_slot_clear_nodes(((Script *)state)->slot);
    return 1;
}


/* EN: Return an empty control status.
 * RU: Вернуть пустое состояние управления. */
static int status(void *state, char *bytes, size_t size, size_t *used)
{
    (void)state;
    (void)bytes;
    (void)size;
    *used = 0;
    return 1;
}


/* [GC:] EN: Treat the first input byte as state and generate a one-byte read reply.
 * RU: Сохранить первый входной байт и сформировать однобайтовый ответ чтения. */
static FVM_Result callback(FVM_Context *context, FVM_Object *receiver, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    Script *script = state;
    FVM_Value value;
    (void)receiver;
    if (argc != 3 || args[0].type != FVM_VALUE_STRING || fvm_value_string_size(args) != 1 ||
        args[1].type != FVM_VALUE_OBJECT || args[2].type != FVM_VALUE_NUMBER || args[2].number != 1)
    {
        return FVM_INVALID_ARGUMENT;
    }
    ++script->calls;
    if (*args[0].string == 'w')
    {
        if (fvm_object_get(context, args[1].object, "0", &value) != FVM_OK ||
            value.type != FVM_VALUE_NUMBER || value.number != 42)
        {
            return FVM_INVALID_ARGUMENT;
        }
        script->value = (unsigned char)value.number;
    }
    else if (*args[0].string == 'r')
    {
        fvm_value_number(&value, script->value);
        if (fvm_object_set(context, args[1].object, "0", &value) != FVM_OK)
        {
            return FVM_INVALID_ARGUMENT;
        }
    }
    else
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_number(out, 1);
    return FVM_OK;
}


/* EN: Use the public session API and verify registration, duplicate and reserved paths.
 * RU: Проверить публикацию, дубликат и служебный путь через API сессии. */
static int publish(FVM_Context *context, Script *script)
{
    FVM_Value rpc, args[3], out;
    fvm_value_undefined(args + 2);
    if (fvm_get_global(context, "rpc", &rpc) != FVM_OK || rpc.type != FVM_VALUE_OBJECT ||
        fvm_function_create_native(context, callback, script, NULL, &args[2].object) != FVM_OK)
    {
        return 0;
    }
    args[2].type = FVM_VALUE_OBJECT;
    fvm_value_string(args, "/menu/data");
    fvm_value_string(args + 1, "rw");
    if (fvm_object_call(context, rpc.object, "newnode", 3, args, &out) != FVM_OK ||
        out.type != FVM_VALUE_BOOLEAN || !out.boolean ||
        fvm_object_call(context, rpc.object, "newnode", 3, args, &out) != FVM_INVALID_ARGUMENT)
    {
        return 0;
    }
    fvm_value_string(args, "/menu/.prog/blocked");
    if (fvm_object_call(context, rpc.object, "newnode", 3, args, &out) != FVM_INVALID_ARGUMENT)
    {
        return 0;
    }
    fvm_value_string(args, "/menu/input");
    fvm_value_string(args + 1, "r");
    if (fvm_object_call(context, rpc.object, "newnode", 3, args, &out) != FVM_OK)
    {
        return 0;
    }
    fvm_value_string(args, "tcp!1234");
    return fvm_object_call(context, rpc.object, "publishchan", 2, (FVM_Value[]){args[0], args[2]}, &out) == FVM_OK &&
        out.type == FVM_VALUE_UNDEFINED &&
        fvm_object_call(context, rpc.object, "newchan", 1, args, &out) == FVM_OK &&
        out.type == FVM_VALUE_UNDEFINED && fvm_gc_collect(context, NULL) == FVM_OK;
}


/* EN: Read/write independently opened files and verify revocation of a still-open fid.
 * RU: Проверить независимые открытия, чтение и запись, затем отзыв живого fid. */
static void *client(void *data)
{
    Peer *peer = data;
    StyxClient *session = styxclientmountfd(peer->fd, 4096, "node-test", "", 1000, 8);
    StyxRpcVfsFile *file = NULL, *control = NULL;
    StyxRpcFileInfo info;
    unsigned char byte = 42;
    size_t used;
    int ok = session &&
        styxrpc_vfs_stat(session, "menu", &info) == STYXRPC_IO_OK && info.directory &&
        styxrpc_vfs_stat(session, "menu/data", &info) == STYXRPC_IO_OK && info.mode == 0666 &&
        styxrpc_vfs_open(session, "menu/input", STYXRPC_FILE_WRITE, &file) != STYXRPC_IO_OK &&
        styxrpc_vfs_open(session, "menu/data", STYXRPC_FILE_RDWR, &file) == STYXRPC_IO_OK &&
        styxrpc_vfs_file_write(file, 0, &byte, 1, &used) == STYXRPC_IO_OK && used == 1;
    byte = 0;
    ok = ok && styxrpc_vfs_file_read(file, 0, &byte, 1, &used) == STYXRPC_IO_OK && used == 1 && byte == 42;
    if (ok)
    {
        control = styxclientwalk(session, ".state");
        ok = control && styxclientopen(control, STYXRPC_FILE_WRITE) &&
            styxrpc_vfs_file_write(control, 0, "restart", 7, &used) == STYXRPC_IO_OK && used == 7 &&
            styxrpc_vfs_stat(session, "menu/data", &info) != STYXRPC_IO_OK &&
            styxrpc_vfs_file_read(file, 0, &byte, 1, &used) != STYXRPC_IO_OK;
    }
    styxrpc_vfs_close(control, 0);
    styxrpc_vfs_close(file, 0);
    styxclientshutdown(session);
    styxclientfree(session);
    peer->ok = ok;
    return NULL;
}


/* EN: Own the FlashVM and hosted server on the same native thread.
 * RU: Владеть FlashVM и сервером в одном нативном потоке. */
int main(void)
{
    Script script = {0};
    MovieAssets assets = {0};
    StyxRpcControl *control = NULL;
    FVM_Runtime *runtime = NULL;
    FVM_Context *context = NULL;
    FVM_StyxRpcPlayerServices *services = NULL;
    StyxRpcServer *server = NULL;
    const StyxRpcControlSpec spec = {&script, 4096, upload, command, status};
    FVM_Config config;
    int sockets[2] = {-1, -1};
    int ok = test_movie_assets_start(&assets) && styxrpc_control_create(&spec, &control) == STYXRPC_OK &&
        (script.slot = styxrpc_module_slot_create(control)) != NULL &&
        fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK &&
        fvm_context_create(runtime, &context) == FVM_OK &&
        fvm_styxrpc_player_services(context, assets.client, script.slot, &services) == FVM_OK &&
        fvm_styxrpc_player_activate(script.slot, services) == FVM_OK && publish(context, &script) &&
        !socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
    if (ok)
    {
        server = styxrpc_server_create(phyposixfd(sockets[1], PHY_STREAM | PHY_ORDERED | PHY_RELIABLE),
            &styxrpc_module_slot_tree, script.slot);
        Peer peer = {sockets[0], 0};
        pthread_t worker;
        if (server && !pthread_create(&worker, NULL, client, &peer))
        {
            while (styxrpc_server_step(server))
            {
            }
            pthread_join(worker, NULL);
            ok = peer.ok && script.calls == 2;
        }
        else
        {
            close(sockets[0]);
            ok = 0;
        }
    }
    styxrpc_server_destroy(server);
    fvm_styxrpc_player_disarm(services);
    fvm_context_destroy(context);
    free(services);
    fvm_runtime_destroy(runtime);
    styxrpc_module_slot_destroy(script.slot);
    styxrpc_control_destroy(control);
    test_movie_assets_stop(&assets);
    if (!ok)
    {
        fputs("default RPC callback file check failed\n", stderr);
    }
    return ok ? 0 : 1;
}
