/* Copyright (c) 2026 bhgv
 * EN: Run two named Dis programs on one bidirectional connection.
 * RU: Запустить две именованные Dis-программы по одному двустороннему соединению.
 */
#define _POSIX_C_SOURCE 200809L
#include "styxrpc/launch.h"
#include "client.h"
#include "drivers/posix/fd.h"
#include "mux/bidir.h"
#include "reverse.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>


/* EN: Submit one selected program and keep its fid until terminal status.
 * RU: Подать выбранную программу и держать её fid до конечного статуса. */
static StyxRpcLaunchStatus run_program(StyxClient *client, const char *name, const char *arg)
{
    const StyxRpcProgram program = {42, 0, 0};
    unsigned char scratch[64];
    StyxFid *request = NULL;
    StyxRpcLaunchStatus status = styxrpc_launch_named(client, name, &program, arg, strlen(arg),
        scratch, sizeof(scratch), &request);
    const struct timespec pause = {0, 10000000};
    for (unsigned tries = 0; status == STYXRPC_LAUNCH_PENDING && tries < 300; ++tries)
    {
        status = styxrpc_launch_named_status(request);
        if (status == STYXRPC_LAUNCH_PENDING)
        {
            nanosleep(&pause, NULL);
        }
    }
    styxrpc_launch_named_close(&request);
    return status;
}


/* EN: Wait for the checkpoint, finish another launch, then clunk the still-pending fid.
 * RU: Дождаться отметки, завершить соседний запуск и закрыть ожидающий fid. */
static int cancel_program(StyxClient *client, ReverseState *state)
{
    const StyxRpcProgram program = {42, 0, 0};
    unsigned char scratch[64];
    StyxFid *request = NULL;
    if (styxrpc_launch_named(client, "device.dis", &program, "park", 4,
        scratch, sizeof(scratch), &request) != STYXRPC_LAUNCH_PENDING)
    {
        return 0;
    }
    const struct timespec pause = {0, 10000000};
    for (unsigned tries = 0; !__atomic_load_n(&state->parked, __ATOMIC_ACQUIRE) && tries < 300; ++tries)
    {
        nanosleep(&pause, NULL);
    }
    int active = __atomic_load_n(&state->parked, __ATOMIC_ACQUIRE) &&
        styxrpc_launch_named_status(request) == STYXRPC_LAUNCH_PENDING &&
        run_program(client, "other.dis", "native-c") == STYXRPC_LAUNCH_OK &&
        styxrpc_launch_named_status(request) == STYXRPC_LAUNCH_PENDING;
    styxrpc_launch_named_close(&request);
    return active;
}


/* EN: Take the connected socket, serve reverse callbacks on a native dispatcher thread,
 * then verify the guest's writeback after joining the thread.
 * RU: Принять сокет, обслужить встречные колбэки в нативном потоке, проверить запись после join. */
int main(int argc, char **argv)
{
    char *end;
    long fd = argc == 2 ? strtol(argv[1], &end, 10) : -1;
    if (fd < 3 || fd > INT_MAX || *end)
    {
        return 2;
    }
    PhyBidir *mux = phybidircreate(phyposixfd((int)fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), 65536);
    if (!mux)
    {
        return 1;
    }
    ReverseState state = {.value = 41};
    if (!register_reverse(&state))
    {
        phybidirfree(mux);
        return 1;
    }
    state.server = styxrpc_server_create(phybidirchannel(mux, 1), &styxrpc_program_tree, &state.tree);
    pthread_t reverse;
    if (!state.server || pthread_create(&reverse, NULL, run_reverse, &state))
    {
        styxrpc_server_destroy(state.server);
        phybidirfree(mux);
        return 1;
    }
    StyxClient *client = styxclientmountphy(phybidirchannel(mux, 0), 4096, "caller", "", 1000, 8);
    StyxRpcLaunchStatus first = run_program(client, "device.dis", "anything");
    int cancelled = cancel_program(client, &state);
    StyxRpcLaunchStatus second = run_program(client, "other.dis", "native-c");
    StyxRpcLaunchStatus failed = run_program(client, "other.dis", "wrong");
    StyxRpcLaunchStatus absent = run_program(client, "../other.dis", "");
    styxclientshutdown(client);
    styxclientfree(client);
    phybidirfree(mux);
    pthread_join(reverse, NULL);
    if (!cancelled || first != STYXRPC_LAUNCH_OK || second != STYXRPC_LAUNCH_OK ||
        failed != STYXRPC_LAUNCH_FAILED || absent != STYXRPC_LAUNCH_NOT_SENT ||
        state.value != 42 || state.byte != 'D' || state.output != 'O' ||
        state.vector.length != 3 || state.values[1] != 12 || state.values[2] != 23 ||
        state.keyed.length != 1 || state.keys[0] != 9 || state.mapped[0] != 77 ||
        state.hybrid.length != 3 || state.hybrid.count != 3 || state.hybrid.dense[1] != 17 ||
        state.hybrid.sparse[0].value != 91)
    {
        fprintf(stderr, "launch results: %d %d %d %d, cancelled=%d count=%d fd=%u stdout=%u\n",
            first, second, failed, absent, cancelled, (int)state.value, (unsigned)state.byte, (unsigned)state.output);
        return 1;
    }
    puts("PASS one socket, two named Dis modules, reverse .var/.fd/.vfs and collections");
    return 0;
}
