/* Copyright (c) 2026 bhgv
 * EN: Verify restricted reverse progress on an active module-slot server.
 * RU: Проверить ограниченное встречное обслуживание активного сервера слота.
 */
#include "styxrpc/control.h"
#include "styxrpc/module_slot.h"
#include <time.h>

typedef struct ProgramWaitProbe
{
    StyxRpcServer *server;  /* Borrowed dispatcher through the test join. */
    int requested;          /* Desired restricted phase, published by the client. */
    int observed;           /* Phase acknowledged by the dispatcher. */
    int commands;           /* Calls accepted by the ordinary application callback. */
} ProgramWaitProbe;


/* EN: Refuse any unexpected complete upload.
 * RU: Отклонить неожиданную завершённую загрузку. */
static int refuse_upload(void *context, const void *bytes, size_t size)
{
    (void)context;
    (void)bytes;
    (void)size;
    return 0;
}


/* EN: Accept exactly one normal pause and verify recursive pumping is refused.
 * RU: Принять обычную паузу и проверить отказ рекурсивного обслуживания. */
static int pause_probe(void *context, StyxRpcControlCommand command)
{
    ProgramWaitProbe *probe = context;
    __atomic_add_fetch(&probe->commands, 1, __ATOMIC_RELAXED);
    return command == STYXRPC_CONTROL_PAUSE && !styxrpc_server_program_step_for(probe->server, 0);
}


/* EN: Publish an empty state snapshot for an ordinary opened control file.
 * RU: Выдать пустой снимок состояния обычному открытому файлу управления. */
static int empty_status(void *context, char *bytes, size_t capacity, size_t *size)
{
    (void)context;
    (void)bytes;
    (void)capacity;
    *size = 0;
    return 1;
}


/* EN: Alternate ordinary and restricted turns on one owner through transport close.
 * RU: Чередовать обычные и ограниченные такты у одного владельца до обрыва. */
static void *dispatch_wait_probe(void *state)
{
    ProgramWaitProbe *probe = state;
    for (;;)
    {
        int restricted = __atomic_load_n(&probe->requested, __ATOMIC_ACQUIRE);
        __atomic_store_n(&probe->observed, restricted, __ATOMIC_RELEASE);
        if (!(restricted ? styxrpc_server_program_step_for(probe->server, 5) :
            styxrpc_server_step_for(probe->server, 5)))
        {
            return NULL;
        }
    }
}


/* EN: Wait at most one second for dispatch to leave the previous phase.
 * RU: Ждать выхода диспетчера из предыдущей фазы до одной секунды. */
static int request_phase(ProgramWaitProbe *probe, int restricted)
{
    __atomic_store_n(&probe->requested, restricted, __ATOMIC_RELEASE);
    const struct timespec delay = {0, 1000000};
    for (unsigned attempt = 0; attempt < 1000; ++attempt)
    {
        if (__atomic_load_n(&probe->observed, __ATOMIC_ACQUIRE) == restricted)
        {
            return 1;
        }
        nanosleep(&delay, NULL);
    }
    return 0;
}


/* EN: Verify native reads, held-fid refusal, and normal recovery on one client.
 * RU: Проверить нативное чтение, отказ удержанному fid и восстановление клиента. */
static int exercise_wait(ProgramWaitProbe *probe, StyxClient *client)
{
    StyxFid *state = styxclientwalk(client, ".state");
    StyxFid *upload = styxclientwalk(client, ".upload");
    int good = state && upload && styxclientopen(state, 2) && styxclientopen(upload, 1) &&
        request_phase(probe, 1);
    if (good)
    {
        StyxFid *blocked = styxclientwalk(client, ".state");
        StyxFid *native = walk_pid(client, 17);
        char byte = 0;
        StyxTransfer control, resource;
        styxclientwrite(state, 0, "pause", sizeof("pause") - 1, &control);
        good = !blocked && !control.success && !__atomic_load_n(&probe->commands, __ATOMIC_RELAXED) &&
            native && styxclientopen(native, 0);
        if (good)
        {
            styxclientread(native, 0, &byte, 1, &resource);
            good = resource.success && resource.count == 1 && byte == 'r';
        }
        styxclientclunk(blocked);
        styxclientclunk(native);
        styxclientclunk(upload);
        upload = NULL;
    }
    if (request_phase(probe, 0) && good)
    {
        styxclientclunk(state);
        state = styxclientwalk(client, ".state");
        upload = styxclientwalk(client, ".upload");
        good = state && styxclientopen(state, 2) && upload && styxclientopen(upload, 1);
        if (good)
        {
            StyxTransfer resumed;
            styxclientwrite(state, 0, "pause", sizeof("pause") - 1, &resumed);
            good = resumed.success && __atomic_load_n(&probe->commands, __ATOMIC_RELAXED) == 1;
        }
    }
    else
    {
        good = 0;
    }
    styxclientclunk(upload);
    styxclientclunk(state);
    return good;
}


/* EN: Start a module slot with a native program bus and an unrelated control tree.
 * RU: Запустить слот с нативной шиной программы и отдельным деревом управления. */
static int check_program_wait(void)
{
    StyxRpcRegistration nodes[8];
    StyxRpcPrograms programs;
    ProgramWaitProbe probe = {0};
    Probe native = {'r', 0};
    const StyxRpcResource resource = {&native, NULL, read_byte, refuse_write, close_probe};
    const StyxRpcControlSpec spec = {&probe, 1024, refuse_upload, pause_probe, empty_status};
    StyxRpcControl *control = NULL;
    if (!styxrpc_programs_init(&programs, nodes, sizeof(nodes) / sizeof(nodes[0])) ||
        !styxrpc_program_add(&programs, 17) || !styxrpc_program_unibus(&programs, 17, &resource) ||
        styxrpc_control_create(&spec, &control) != STYXRPC_OK)
    {
        return 0;
    }
    StyxRpcModuleSlot *slot = styxrpc_module_slot_create(control);
    int sockets[2];
    int good = 0;
    if (slot && styxrpc_module_slot_programs(slot, &programs) &&
        !socketpair(AF_UNIX, SOCK_STREAM, 0, sockets))
    {
        probe.server = styxrpc_server_create(
            phyposixfd(sockets[1], PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), &styxrpc_module_slot_tree, slot);
        pthread_t thread;
        if (probe.server && !pthread_create(&thread, NULL, dispatch_wait_probe, &probe))
        {
            StyxClient *client = styxclientmountfd(sockets[0], 4096, "test", "", 1000, 8);
            good = client && exercise_wait(&probe, client);
            styxclientshutdown(client);
            styxclientfree(client);
            pthread_join(thread, NULL);
        }
        else
        {
            close(sockets[0]);
        }
        styxrpc_server_destroy(probe.server);
    }
    styxrpc_module_slot_destroy(slot);
    styxrpc_control_destroy(control);
    return good && __atomic_load_n(&native.closes, __ATOMIC_RELAXED) == 1;
}
