/* Copyright (c) 2026 bhgv
 * EN: Run the paired headless playback owner on a delegated bidirectional Styx connection.
 * RU: Запуск парного плеера без окна на выданном двустороннем соединении Styx.
 */
#include "playback_clock.h"
#include "../../ports/monotonic_port.h"
#include "flashvm/swf.h"
#include "client.h"
#include "drivers/posix/fd.h"
#include "mux/bidir.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#define PLAYER_MIN_FD 3         /* Standard streams remain available for diagnostics. */
#define PLAYER_MUX_LIMIT 65536U /* Maximum frame on the bidirectional carrier. */
#define PLAYER_MSIZE 4096U     /* Requested message size for the remote filesystem. */
#define PLAYER_WAIT_MS 5000U   /* Per-operation mount and file timeout. */
#define PLAYER_TAGS 8U         /* Pending reverse Styx tags. */
#define PLAYER_TICK 1U         /* Timed operation advances one frame with no backlog burst. */


/* EN: Consume one traced ActionScript string and publish it on the owner's stdout.
 * RU: Выдать одну строку ActionScript Trace в stdout владельца. */
static FVM_Result trace(FVM_Context *context, FVM_Object *receiver, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    (void)context;
    (void)receiver;
    (void)out;
    (void)state;
    if (argc == 1 && args->type == FVM_VALUE_STRING)
    {
        fwrite(args->string, 1, fvm_value_string_size(args), stdout);
        fputc('\n', stdout);
        fflush(stdout);
    }
    return FVM_OK;
}


/* EN: Connect diagnostics in each freshly uploaded movie context.
 * RU: Подключить диагностику в каждом новом контексте фильма. */
static FVM_Result setup(void *state, FVM_Context *context)
{
    (void)state;
    return fvm_avm1_set_trace_rooted(context, trace, NULL, NULL);
}


/* [ASINC] EN: Alternate wire dispatch and monotonic frame deadlines on the playback owner.
 * RU: Чередовать обработку соединения и монотонные сроки кадров у владельца. */
static int replay(StyxRpcServer *server, FVM_StyxRpcPlayback *playback)
{
    unsigned long previous = 0;
    double deadline = 0;
    for (;;)
    {
        int wait_ms = -1;
        double period;
        unsigned long current;
        if (fvm_styxrpc_playback_clock(playback, &period, &current))
        {
            double now;
            if (!fvm_port_monotonic_ms(&now))
            {
                return 1;
            }
            if (!deadline || current != previous)
            {
                deadline = now + period;
                previous = current;
            }
            if (now >= deadline)
            {
                fvm_styxrpc_playback_tick(playback, PLAYER_TICK);
                deadline += period;
                if (!fvm_port_monotonic_ms(&now))
                {
                    return 1;
                }
                if (deadline <= now)
                {
                    deadline = now + period;
                }
            }
            wait_ms = (int)(deadline - now);
            if ((double)wait_ms < deadline - now)
            {
                ++wait_ms;
            }
        }
        else
        {
            deadline = 0;
        }
        if (!styxrpc_server_step_for(server, wait_ms))
        {
            return 0;
        }
    }
}


/* EN: Parse an inherited, connected stream descriptor without opening a listener or path.
 * RU: Разобрать унаследованный связанный поток без открытия слушателя или файла. */
static int connected(const char *value)
{
    struct sockaddr_storage peer;
    socklen_t size = sizeof(peer), type_size = sizeof(int);
    char *end;
    int type;
    long fd = strtol(value, &end, 10);
    if (!*value || *end || fd < PLAYER_MIN_FD || fd > INT_MAX ||
        getsockopt((int)fd, SOL_SOCKET, SO_TYPE, &type, &type_size) ||
        type != SOCK_STREAM || getpeername((int)fd, (struct sockaddr *)&peer, &size))
    {
        return -1;
    }
    return (int)fd;
}


/* EN: Attach the DisVM filesystem on lane 1 and serve playback control on lane 0.
 * RU: Подключить файловую систему DisVM на канале 1 и обслуживать управление на канале 0. */
static int run(int fd, FVM_ExecutionMode mode)
{
    FVM_Config config;
    FVM_Runtime *runtime = NULL;
    FVM_StyxRpcPlayback *playback = NULL;
    StyxRpcServer *server = NULL;
    int status = 1;
    PhyBidir *mux = phybidircreate(phyposixfd(fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), PLAYER_MUX_LIMIT);
    StyxClient *root = mux ? styxclientmountphy(phybidirchannel(mux, 1), PLAYER_MSIZE, "flash", "", PLAYER_WAIT_MS,
        PLAYER_TAGS) : NULL;
    if (root && fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK)
    {
        FVM_StyxRpcPlaybackSpec spec = {0};
        spec.runtime = runtime;
        spec.session = root;
        spec.upload_limit = FVM_SWF_DEFAULT_LIMIT;
        spec.mode = mode;
        spec.setup = setup;
        if (fvm_styxrpc_playback_create(&spec, &playback) == FVM_OK)
        {
            server = styxrpc_server_create(phybidirchannel(mux, 0), &styxrpc_module_slot_tree,
                fvm_styxrpc_playback_slot(playback));
        }
    }
    if (server)
    {
        status = replay(server, playback);
    }
    else
    {
        fputs("flashvm_styx_player: cannot attach the paired session\n", stderr);
    }
    styxrpc_server_destroy(server);
    fvm_styxrpc_playback_destroy(playback);
    fvm_runtime_destroy(runtime);
    styxclientshutdown(root);
    styxclientfree(root);
    phybidirfree(mux);
    return status;
}


/* EN: Validate launch options, then run one authorized connection to retirement.
 * RU: Проверить параметры запуска и обслужить одно выданное соединение до закрытия. */
int main(int argc, char **argv)
{
    FVM_ExecutionMode mode = FVM_EXECUTION_INTERPRETER;
    int fd = -1;
    if (argc == 2 && !strcmp(argv[1], "--help"))
    {
        puts("usage: flashvm_styx_player --fd FD [--jit | --no-jit]");
        return 0;
    }
    if ((argc == 3 || argc == 4) && !strcmp(argv[1], "--fd"))
    {
        fd = connected(argv[2]);
        if (argc == 4 && !strcmp(argv[3], "--jit"))
        {
            if (!fvm_jit_available())
            {
                fputs("flashvm_styx_player: JIT unavailable\n", stderr);
                return 2;
            }
            mode = FVM_EXECUTION_JIT;
        }
        else if (argc == 4 && strcmp(argv[3], "--no-jit"))
        {
            fd = -1;
        }
    }
    if (fd < 0)
    {
        fputs("flashvm_styx_player: expected --fd FD for a connected stream and optional --jit or --no-jit\n", stderr);
        return 2;
    }
    return run(fd, mode);
}
