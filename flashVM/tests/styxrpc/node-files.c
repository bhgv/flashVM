/* Copyright (c) 2026 bhgv
 * EN: Exercise default-session callback files over two actual Styx endpoints.
 * RU: Проверка файлов с колбэками штатной сессии через реальные соединения Styx.
 */
#include "movie-assets.h"
#include "../../src/styxrpc/player_services.h"
#include "flashvm/exception.h"
#include "flashvm/execution.h"
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
    unsigned int releases;  /* Exact native release count at withdrawal. */
    unsigned int calls;     /* Exact handler invocation count on the VM owner. */
} Script;

typedef struct Peer
{
    int fd; /* Owned peer socket, consumed when attaching. */
    int ok; /* Result transferred to main at join. */
} Peer;


/* [GC:] EN: Check local argument refusal and failure on an ordinary asset provider.
 * RU: Проверить локальный отказ по аргументу и ошибку на обычном провайдере ресурсов. */
static int reject_exec(FVM_Context *context)
{
    FVM_Value vfs, args[2], out, bad;
    if (fvm_get_global(context, "vfs", &vfs) != FVM_OK || vfs.type != FVM_VALUE_OBJECT)
    {
        return 0;
    }
    if (fvm_object_call(context, vfs.object, "exec", 0, NULL, &out) != FVM_INVALID_ARGUMENT)
    {
        return 0;
    }
    fvm_value_string(args, "/nested/.exec/child");
    if (fvm_object_call(context, vfs.object, "exec", 1, args, &out) != FVM_INVALID_ARGUMENT)
    {
        return 0;
    }
    fvm_value_string(args, "/tools/apply");
    fvm_value_number(&bad, 256);
    if (fvm_exec_literal_array(context, 1, &bad, args + 1) != FVM_OK ||
        fvm_object_call(context, vfs.object, "exec", 2, args, &out) != FVM_INVALID_ARGUMENT)
    {
        return 0;
    }
    FVM_Result status = fvm_object_call(context, vfs.object, "exec", 1, args, &out);
    if (status == FVM_EXCEPTION)
    {
        fvm_exception_clear(context);
    }
    return status == FVM_EXCEPTION;
}


/* EN: Reject uploads in this fixture.
 * RU: Отклонить загрузки в этой проверке. */
static int upload(void *state, const void *bytes, size_t size)
{
    (void)state;
    (void)bytes;
    (void)size;
    return 0;
}


/* EN: Return one identifying byte for a native registration; deny any other transfer.
 * RU: Вернуть отличительный байт нативной регистрации. Отклонить остальные операции. */
static int native_read(void *state, unsigned access, void *bytes, size_t size, size_t *used)
{
    (void)state;
    if (access != STYXRPC_READ || size != 1)
    {
        return 0;
    }
    *(unsigned char *)bytes = 91;
    *used = 1;
    return 1;
}


/* EN: Count one retirement of a native registration using the borrowed test state.
 * RU: Учесть один отзыв нативной регистрации через заимствованное состояние проверки. */
static void native_release(void *state)
{
    Script *script = state;
    ++script->releases;
}


/* EN: Change nodes on the owner thread after receiving the fixture's control command.
 * RU: Изменить узлы в потоке-владельце по управляющей команде проверки. */
static int command(void *state, StyxRpcControlCommand mode)
{
    Script *script = state;
    if (mode == STYXRPC_CONTROL_PAUSE)
    {
        const StyxRpcNodeCallbacks replacement = {native_read, native_release, script};
        return styxrpc_module_slot_remove_node(script->slot, "menu/single") == STYXRPC_OK &&
            script->releases == 1 &&
            styxrpc_module_slot_remove_node(script->slot, "/menu/single") == STYXRPC_INVALID &&
            styxrpc_module_slot_remove_node(script->slot, "menu/data") == STYXRPC_OK &&
            styxrpc_module_slot_new_node(script->slot, "menu/data", 4, &replacement) == STYXRPC_OK;
    }
    if (mode != STYXRPC_CONTROL_RESTART)
    {
        return 0;
    }
    styxrpc_module_slot_clear_nodes(script->slot);
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
    else if (*args[0].string == 'x')
    {
        if (fvm_object_get(context, args[1].object, "0", &value) != FVM_OK ||
            value.type != FVM_VALUE_NUMBER || value.number != 38)
        {
            return FVM_INVALID_ARGUMENT;
        }
        fvm_value_number(&value, 42);
        if (fvm_object_set(context, args[1].object, "0", &value) != FVM_OK)
        {
            return FVM_INVALID_ARGUMENT;
        }
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
    FVM_Value vfs, args[3], out;
    fvm_value_undefined(args + 2);
    if (fvm_get_global(context, "vfs", &vfs) != FVM_OK || vfs.type != FVM_VALUE_OBJECT ||
        fvm_function_create_native(context, callback, script, NULL, &args[2].object) != FVM_OK)
    {
        return 0;
    }
    args[2].type = FVM_VALUE_OBJECT;
    fvm_value_string(args, "/menu/data");
    fvm_value_string(args + 1, "rw");
    if (fvm_object_call(context, vfs.object, "newnode", 3, args, &out) != FVM_OK ||
        out.type != FVM_VALUE_BOOLEAN || !out.boolean ||
        fvm_object_call(context, vfs.object, "newnode", 3, args, &out) != FVM_INVALID_ARGUMENT)
    {
        return 0;
    }
    fvm_value_string(args, "/rollback/deep/.prog/blocked");
    if (fvm_object_call(context, vfs.object, "newnode", 3, args, &out) != FVM_INVALID_ARGUMENT)
    {
        return 0;
    }
    fvm_value_string(args, "/menu/input");
    fvm_value_string(args + 1, "r");
    if (fvm_object_call(context, vfs.object, "newnode", 3, args, &out) != FVM_OK)
    {
        return 0;
    }
    fvm_value_string(args, "/menu/invoke");
    fvm_value_string(args + 1, "x");
    if (fvm_object_call(context, vfs.object, "newnode", 3, args, &out) != FVM_OK)
    {
        return 0;
    }
    const StyxRpcNodeCallbacks native = {native_read, native_release, script};
    if (styxrpc_module_slot_new_node(script->slot, "menu/single", 4, &native) != STYXRPC_OK)
    {
        return 0;
    }
    fvm_value_string(args, "tcp!1234");
    return fvm_object_call(context, vfs.object, "publishchan", 2, (FVM_Value[]){args[0], args[2]}, &out) == FVM_OK &&
        out.type == FVM_VALUE_UNDEFINED &&
        fvm_object_call(context, vfs.object, "newchan", 1, args, &out) == FVM_OK &&
        out.type == FVM_VALUE_UNDEFINED && fvm_gc_collect(context, NULL) == FVM_OK;
}


/* EN: Keep walked and open fids across withdrawal of one exact registration.
 * RU: Удержать пройденный и открытый fid во время отзыва точной регистрации. */
static int check_withdrawal(StyxClient *session, StyxRpcVfsFile *opened, StyxRpcVfsFile *control)
{
    StyxRpcVfsFile *walked = styxclientwalk(session, "menu/data");
    StyxRpcVfsFile *replacement = NULL;
    StyxRpcFileInfo info;
    size_t used;
    unsigned char byte;
    int ok = walked && styxrpc_vfs_file_write(control, 0, "pause", 5, &used) == STYXRPC_IO_OK &&
        used == 5 && styxrpc_vfs_stat(session, "menu/single", &info) != STYXRPC_IO_OK &&
        styxrpc_vfs_stat(session, "menu/input", &info) == STYXRPC_IO_OK &&
        !styxclientopen(walked, STYXRPC_FILE_READ) &&
        styxrpc_vfs_file_read(opened, 0, &byte, 1, &used) != STYXRPC_IO_OK &&
        styxrpc_vfs_open(session, "menu/data", STYXRPC_FILE_READ, &replacement) == STYXRPC_IO_OK &&
        styxrpc_vfs_file_read(replacement, 0, &byte, 1, &used) == STYXRPC_IO_OK && used == 1 && byte == 91;
    styxrpc_vfs_close(walked, 0);
    styxrpc_vfs_close(replacement, 0);
    return ok;
}


/* EN: Read/write independently opened files and verify revocation of a still-open fid.
 * RU: Проверить независимые открытия, чтение и запись, затем отзыв живого fid. */
static void *client(void *data)
{
    Peer *peer = data;
    StyxClient *session = styxclientmountfd(peer->fd, 4096, "node-test", "", 1000, 8);
    StyxRpcVfsFile *file = NULL, *control = NULL, *pending = NULL;
    StyxRpcFileInfo info;
    unsigned char byte = 42;
    size_t used;
    int ok = session &&
        styxrpc_vfs_stat(session, "rollback", &info) != STYXRPC_IO_OK &&
        styxrpc_vfs_stat(session, "menu", &info) == STYXRPC_IO_OK && info.directory &&
        styxrpc_vfs_stat(session, "menu/data", &info) == STYXRPC_IO_OK && info.mode == 0666 &&
        styxrpc_vfs_open(session, "menu/input", STYXRPC_FILE_WRITE, &file) != STYXRPC_IO_OK &&
        styxrpc_vfs_open(session, "menu/data", STYXRPC_FILE_RDWR, &file) == STYXRPC_IO_OK &&
        styxrpc_vfs_file_write(file, 0, &byte, 1, &used) == STYXRPC_IO_OK && used == 1;
    byte = 0;
    ok = ok && styxrpc_vfs_file_read(file, 0, &byte, 1, &used) == STYXRPC_IO_OK && used == 1 && byte == 42;
    byte = 38;
    ok = ok && styxrpc_vfs_stat(session, STYXRPC_NODE_EXEC_SERVICE, &info) == STYXRPC_IO_NOT_OPEN &&
        styxrpc_node_exec(session, "menu/input", &byte, 1, &used) != STYXRPC_IO_OK && !used && byte == 38 &&
        styxrpc_node_exec(session, "menu/invoke", &byte, 1, &used) == STYXRPC_IO_OK && used == 1 && byte == 42;
    if (ok)
    {
        control = styxclientwalk(session, ".state");
        ok = control && styxclientopen(control, STYXRPC_FILE_WRITE) &&
            check_withdrawal(session, file, control);
        styxrpc_vfs_close(control, 0);
        control = NULL;
    }
    if (ok)
    {
        /* One counted path and byte, with execution still pending when the provider retires. */
        static const unsigned char header[] = {0, 0, 0, sizeof("menu/invoke"), 0, 0, 0, 1};
        pending = styxclientwalk(session, STYXRPC_NODE_EXEC_SERVICE);
        control = styxclientwalk(session, ".state");
        ok = pending && styxclientopen(pending, STYXRPC_FILE_RDWR) &&
            styxrpc_vfs_file_write(pending, 0, header, sizeof(header), &used) == STYXRPC_IO_OK &&
            used == sizeof(header) &&
            control && styxclientopen(control, STYXRPC_FILE_WRITE) &&
            styxrpc_vfs_file_write(control, 0, "restart", 7, &used) == STYXRPC_IO_OK && used == 7 &&
            styxrpc_vfs_stat(session, "menu/data", &info) != STYXRPC_IO_OK &&
            styxrpc_vfs_file_read(file, 0, &byte, 1, &used) != STYXRPC_IO_OK &&
            styxrpc_vfs_file_write(pending, sizeof(header), "menu/invoke", sizeof("menu/invoke"), &used) != STYXRPC_IO_OK;
    }
    styxrpc_vfs_close(pending, 0);
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
        fvm_styxrpc_player_activate(script.slot, services) == FVM_OK && reject_exec(context) &&
        publish(context, &script) &&
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
            ok = peer.ok && script.calls == 3 && script.releases == 2;
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
