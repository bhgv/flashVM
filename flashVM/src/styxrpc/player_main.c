/* Copyright (c) 2026 bhgv
 * EN: Run the paired headless playback owner on a delegated bidirectional Styx connection.
 * RU: Запуск парного плеера без окна на выданном двустороннем соединении Styx.
 */
#include "playback_clock.h"
#include "player_view.h"
#include "player_terminal.h"
#include "session_methods.h"
#include "player_peer.h"
#include "movie_load.h"
#include "../../ports/monotonic_port.h"
#include "flashvm/swf.h"
#include "flashvm/standard_text.h"
#include "client.h"
#include "drivers/posix/fd.h"
#include "drivers/posix/tcp.h"
#include "mux/bidir.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PLAYER_MIN_FD 3         /* Standard streams remain available for diagnostics. */
#define PLAYER_MUX_LIMIT 65536U /* Maximum frame on the bidirectional carrier. */
#define PLAYER_TCP_DIAL_MS 5000 /* TCP connect budget after name resolution. */
#define PLAYER_TICK 1U         /* Timed operation advances one frame with no backlog burst. */
#define PLAYER_FONT_LIMIT (4U * 1024U * 1024U) /* Maximum TTF bytes read from the attached VFS. */


/* EN: Borrow the reverse namespace and own the live terminal during one playback session.
 * RU: Заимствовать встречное пространство имён и владеть терминалом в одной сессии. */
typedef struct PlayerHost
{
    StyxRpcExchange *exchange;   /* Owned application mailbox for the session. */
    StyxRpcTerminal terminal;   /* Live reserved UniBus channel for this playback. */
    struct StyxClient *session; /* Borrowed reverse filesystem mount. */
    FVM_PairedView *view;        /* Borrowed player presentation for all movie generations. */
    int flash_peer;              /* Peer serves a movie, so no Dis font VFS is expected. */
} PlayerHost;


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
    PlayerHost *host = state;
    FVM_Result result = fvm_styxrpc_player_terminal(context, &host->terminal);
    if (result == FVM_OK)
    {
        result = fvm_styxrpc_host_exchange(context, host->exchange);
    }
#ifdef FVM_HAS_FONTS
    if (result == FVM_OK && !host->flash_peer)
    {
        unsigned char *bytes;
        size_t size;
        result = fvm_styxrpc_read_asset(host->session, "ide-assets/fonts/NotoSansMono-Regular.ttf",
            PLAYER_FONT_LIMIT, &bytes, &size);
        if (result == FVM_NOT_FOUND)
        {
            result = fvm_styxrpc_read_asset(host->session, "fonts/NotoSansMono-Regular.ttf",
                PLAYER_FONT_LIMIT, &bytes, &size);
        }
        if (result == FVM_OK)
        {
            result = fvm_standard_text_register_font(context, "Noto Sans Mono", bytes, size, 0);
            if (result == FVM_OK)
            {
                result = fvm_standard_text_install(context);
            }
        }
        else if (result == FVM_NOT_FOUND)
        {
            result = FVM_OK;
        }
        free(bytes);
    }
#endif
    if (result == FVM_OK)
    {
        result = fvm_paired_view_install_input(host->view, context);
    }
    return result == FVM_OK ? fvm_avm1_set_trace_rooted(context, trace, NULL, NULL) : result;
}


/* [ASINC] EN: Alternate wire dispatch and monotonic frame deadlines on the playback owner.
 * RU: Чередовать обработку соединения и монотонные сроки кадров у владельца. */
static int replay(StyxRpcServer *server, FVM_StyxRpcPlayback *playback, PlayerHost *host)
{
    unsigned long previous = 0;
    double deadline = 0;
    for (;;)
    {
        int wait_ms = -1;
        double period;
        unsigned long current;
        int presentation;
        if (fvm_styxrpc_playback_clock(playback, &period, &current))
        {
            FVM_Context *context;
            FVM_Object *root;
            double now;
            if (fvm_styxrpc_playback_scene(playback, &context, &root) &&
                fvm_styxrpc_player_terminal_notify(context, root, &host->terminal) != FVM_OK)
            {
                fputs("flashvm_styx_player: terminal notification failed\n", stderr);
                return 1;
            }
            if (!fvm_port_monotonic_ms(&now))
            {
                return 1;
            }
            if (!deadline || current != previous)
            {
                deadline = now;
                previous = current;
            }
            if (now >= deadline)
            {
                FVM_Result result = fvm_styxrpc_playback_tick(playback, PLAYER_TICK);
                if (result != FVM_OK)
                {
                    fprintf(stderr, "flashvm_styx_player: movie tick failed with status %d\n", (int)result);
                }
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
        presentation = fvm_paired_view_step(host->view, playback, &wait_ms);
        if (presentation <= 0)
        {
            if (presentation < 0)
            {
                fputs("flashvm_styx_player: window update failed\n", stderr);
            }
            return presentation < 0;
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


/* EN: Open the explicit TCP address. Announce a listening port before accepting
 * one peer. Return an owned stream or -1 on failure.
 * RU: Открыть указанный TCP-адрес. Объявить порт до приёма одного соединения.
 * Вернуть свой поток или -1 при ошибке. */
static int open_tcp(const char *host, const char *port, int listen_once)
{
    unsigned actual;
    int listener;
    int fd = -1;
    if (!listen_once)
    {
        return phytcpdial(host, port, PLAYER_TCP_DIAL_MS);
    }
    listener = phytcplisten(host, port, &actual);
    if (listener >= 0)
    {
        if (printf("styx listening %u\n", actual) >= 0 && !fflush(stdout))
        {
            fd = phytcpaccept(listener);
        }
        close(listener);
    }
    return fd;
}


/* EN: Adopt fd, attach the complementary client lane and serve playback on
 * serve_lane. Stage the optional local startup image on the same owner.
 * RU: Принять fd, подключить встречный клиентский канал и обслуживать плеер
 * на serve_lane. Подготовить необязательный стартовый SWF у того же владельца. */
static int run(int fd, FVM_ExecutionMode mode, FVM_PairedView *view, unsigned serve_lane,
    int flash_peer, const char *movie)
{
    FVM_Config config;
    StyxRpcMessageType types[STYXRPC_EXCHANGE_END - 1];
    StyxRpcMessageSpec exchange_spec;
    PlayerHost host = {0};
    FVM_Runtime *runtime = NULL;
    FVM_StyxRpcPlayback *playback = NULL;
    StyxRpcServer *server = NULL;
    PlayerPeerChannels channels = {0};
    int status = 1;
    int named = 0;
    PhyBidir *mux = phybidircreate_lanes(phyposixfd(fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE),
        PLAYER_MUX_LIMIT, flash_peer ? PHY_BIDIR_MAX_LANES : PHY_BIDIR_LANES);
    unsigned client_lane = serve_lane == PLAYER_SERVE_FORWARD ? PLAYER_SERVE_REVERSE : PLAYER_SERVE_FORWARD;
    StyxClient *root = NULL;
    if (mux && !flash_peer)
    {
        root = styxclientmountphy(phybidirchannel(mux, client_lane), PLAYER_MSIZE, "flash", "", PLAYER_WAIT_MS,
            PLAYER_TAGS);
    }
    host.view = view;
    host.flash_peer = flash_peer;
    if (mux && (root || flash_peer) && fvm_config_init(&config) == FVM_OK &&
        fvm_runtime_create(&config, &runtime) == FVM_OK)
    {
        FVM_StyxRpcPlaybackSpec spec = {0};
        spec.runtime = runtime;
        spec.session = root;
        spec.upload_limit = FVM_SWF_DEFAULT_LIMIT;
        spec.mode = mode;
        spec.setup = setup;
        spec.host = &host;
        if (fvm_styxrpc_playback_create(&spec, &playback) == FVM_OK)
        {
            playback->awaiting_session = flash_peer;
            playback->defer_command_tick = 1;
            server = styxrpc_server_create(phybidirchannel(mux, serve_lane), &styxrpc_module_slot_tree,
                fvm_styxrpc_playback_slot(playback));
            playback->message_server = server;
            fvm_styxrpc_debug_init(playback, server);
        }
    }
    host.exchange = styxrpc_exchange_create();
    if (host.exchange)
    {
        styxrpc_exchange_spec(host.exchange, 1, types, &exchange_spec);
    }
    if (server && host.exchange && fvm_styxrpc_player_publish_terminal(server, &host.terminal) &&
        styxrpc_messages_publish(server, STYXRPC_EXCHANGE_DEFAULT, &exchange_spec) == STYXRPC_OK)
    {
        if (flash_peer)
        {
            root = fvm_styxrpc_player_peer_mount(server, phybidirchannel(mux, client_lane), &named);
            playback->spec.session = root;
            playback->awaiting_session = 0;
        }
        else
        {
            fvm_styxrpc_player_dis_channels(playback, server, root, &channels);
        }
        host.session = root;
        if (!root)
        {
            fputs("flashvm_styx_player: cannot mount the FlashVM peer\n", stderr);
        }
        else if (named && !fvm_styxrpc_player_peer_channels(playback, mux, serve_lane, &channels))
        {
            fputs("flashvm_styx_player: cannot attach the peer channel service\n", stderr);
        }
        else if (movie && !fvm_styxrpc_player_peer_movie(playback, movie))
        {
            fprintf(stderr, "flashvm_styx_player: cannot load startup SWF %s\n", movie);
        }
        else
        {
            status = replay(server, playback, &host);
            if (!status && !fvm_paired_view_capture(view, playback))
            {
                fputs("flashvm_styx_player: could not capture the final movie frame\n", stderr);
                status = 1;
            }
        }
    }
    else
    {
        fputs("flashvm_styx_player: cannot attach the paired session\n", stderr);
    }
    if (playback)
    {
        fvm_styxrpc_player_clear_messages(playback->script);
    }
    styxrpc_server_destroy(server);
    fvm_styxrpc_player_peer_close(&channels);
    fvm_styxrpc_playback_destroy(playback);
    styxrpc_exchange_destroy(host.exchange);
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
    FVM_PairedView *view = NULL;
    const char *capture = NULL;
    const char *movie = NULL;
    unsigned serve_lane = PLAYER_SERVE_FORWARD;
    int flash_peer = 0;
    int windowed = 0, mode_seen = 0;
    int fd, i, result, dial, listen_once;
    if (argc == 2 && !strcmp(argv[1], "--help"))
    {
        puts("usage: flashvm_styx_player (--fd FD | --listen HOST PORT | --connect HOST PORT)\n"
             "       [--peer forward|reverse [--movie FILE]] [--jit | --no-jit] [--window] [--capture PATH]");
        return 0;
    }
    dial = argc >= 4 && !strcmp(argv[1], "--connect");
    listen_once = argc >= 4 && !strcmp(argv[1], "--listen");
    if (!dial && !listen_once && (argc < 3 || strcmp(argv[1], "--fd")))
    {
        fputs("flashvm_styx_player: expected --fd FD, --listen HOST PORT or --connect HOST PORT\n", stderr);
        return 2;
    }
    for (i = dial || listen_once ? 4 : 3; i < argc; ++i)
    {
        if (!mode_seen && (!strcmp(argv[i], "--jit") || !strcmp(argv[i], "--no-jit")))
        {
            mode_seen = 1;
            if (!strcmp(argv[i], "--jit"))
            {
                if (!fvm_jit_available())
                {
                    fputs("flashvm_styx_player: JIT unavailable\n", stderr);
                    return 2;
                }
                mode = FVM_EXECUTION_JIT;
            }
        }
        else if (!windowed && !strcmp(argv[i], "--window"))
        {
            windowed = 1;
        }
        else if (!capture && !strcmp(argv[i], "--capture") && i + 1 < argc && argv[i + 1][0])
        {
            capture = argv[++i];
        }
        else if (!flash_peer && !strcmp(argv[i], "--peer") && i + 1 < argc &&
            (!strcmp(argv[i + 1], "forward") || !strcmp(argv[i + 1], "reverse")))
        {
            flash_peer = 1;
            serve_lane = !strcmp(argv[++i], "reverse") ? PLAYER_SERVE_REVERSE : PLAYER_SERVE_FORWARD;
        }
        else if (!movie && !strcmp(argv[i], "--movie") && i + 1 < argc && argv[i + 1][0])
        {
            movie = argv[++i];
        }
        else
        {
            fputs("flashvm_styx_player: invalid option or missing option argument\n", stderr);
            return 2;
        }
    }
    if (movie && !flash_peer)
    {
        fputs("flashvm_styx_player: --movie requires --peer\n", stderr);
        return 2;
    }
    fd = -1;
    if (!dial && !listen_once)
    {
        fd = connected(argv[2]);
        if (fd < 0)
        {
            fputs("flashvm_styx_player: expected --fd FD for a connected stream\n", stderr);
            return 2;
        }
    }
    if (windowed || capture)
    {
        view = fvm_paired_view_create(windowed, capture);
        if (!view)
        {
            return 2;
        }
    }
    if (dial || listen_once)
    {
        fd = open_tcp(argv[2], argv[3], listen_once);
    }
    if (fd < 0)
    {
        fputs("flashvm_styx_player: cannot open TCP Styx endpoint\n", stderr);
        result = 2;
    }
    else
    {
        result = run(fd, mode, view, serve_lane, flash_peer, movie);
    }
    fvm_paired_view_destroy(view);
    return result;
}
