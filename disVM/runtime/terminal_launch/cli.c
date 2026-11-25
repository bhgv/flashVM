/* Copyright (c) 2026 bhgv
 * EN: One launch on an externally authorized Styx connection.
 * RU: Один запуск по заранее выданному соединению Styx.
 */
#include "config.h"
#include "unibus_server.h"
#include "vfs/launch/runtime.h"
#include "vfs/export.h"
#include "../../native/terminal/route/launch.h"
#include "../../native/vfs/mount.h"
#include "drivers/posix/fd.h"
#include "drivers/posix/tcp.h"
#include "mux/bidir.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define DVM_STYX_TCP_DIAL_MS 5000 /* Connect budget after name resolution. */


/* EN/RU: See config.h. / См. config.h. */
int dvm_terminal_tcp(const char *host, const char *port, int listen_once)
{
    if (!listen_once)
    {
        return phytcpdial(host, port, DVM_STYX_TCP_DIAL_MS);
    }
    unsigned actual;
    int listener = phytcplisten(host, port, &actual);
    if (listener < 0)
    {
        return -1;
    }
    int fd = -1;
    if (printf("styx listening %u\n", actual) >= 0 && !fflush(stdout))
    {
        fd = phytcpaccept(listener);
    }
    close(listener);
    return fd;
}

/* EN/RU: Duplicate a delegated connected socket, preserving the original FD.
 * Дублировать выданный подключённый сокет, сохранив исходный FD. */
int dvm_terminal_connected(const char *value)
{
    char *end;
    long number = strtol(value, &end, 10);
    if (!value[0] || *end || number < 3 || number > INT_MAX)
    {
        return -1;
    }
    int type;
    socklen_t size = sizeof(type);
    struct sockaddr_storage peer;
    if (getsockopt((int)number, SOL_SOCKET, SO_TYPE, &type, &size) || type != SOCK_STREAM)
    {
        return -1;
    }
    size = sizeof(peer);
    if (getpeername((int)number, (struct sockaddr *)&peer, &size))
    {
        return -1;
    }
    struct stat carrier;
    if (fstat((int)number, &carrier))
    {
        return -1;
    }
    for (int standard = 0; standard < 3; standard++)
    {
        struct stat descriptor;
        if (!fstat(standard, &descriptor) && descriptor.st_dev == carrier.st_dev &&
            descriptor.st_ino == carrier.st_ino)
        {
            return -1;
        }
    }
    return fcntl((int)number, F_DUPFD_CLOEXEC, 3);
}


/* EN/RU: Generate an ephemeral canonical ID for this one-shot invocation.
 * Создать временный канонический ID для одного запуска. */
static int launch_id(char id[DVM_ORIGIN_ID_BYTES])
{
    static const char digits[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    unsigned char bytes[DVM_ORIGIN_ID_BYTES - 1];
    size_t used = 0;
    while (used < sizeof(bytes))
    {
        ssize_t count = getrandom(bytes + used, sizeof(bytes) - used, 0);
        if (count < 0 && errno == EINTR)
        {
            continue;
        }
        if (count <= 0)
        {
            return 0;
        }
        used += (size_t)count;
    }
    for (size_t i = 0; i < sizeof(bytes); i++)
    {
        id[i] = digits[bytes[i] & (i + 1 == sizeof(bytes) ? 60 : 63)];
    }
    id[sizeof(bytes)] = 0;
    return 1;
}


/* EN/RU: Provide one explicit readonly file from the authorized code export.
 * Выдать один явно указанный файл из разрешённого экспорта кода. */
static DVM_Provider *code_root(DVM_Provider *root, const char *file, const char *dependency, DVM_Provider *inbox)
{
    DVM_View *view = dvm_view_create(NULL, NULL, NULL);
    if (!view || !dvm_view_map(view, "/reverse-exit", inbox, ".", FVM_ACCESS_READ, 0) ||
        !dvm_view_map(view, "/app.dis", root, file, FVM_ACCESS_READ | FVM_ACCESS_EXEC, 0) ||
        (strcmp(dependency, "-") && !dvm_view_map(view, "/dep.dis", root, dependency,
            FVM_ACCESS_READ | FVM_ACCESS_EXEC, 0)))
    {
        dvm_view_discard(view);
        return NULL;
    }
    return dvm_view_freeze(view);
}


/* EN/RU: Read one configured launch route and atomically take its reviewed environment.
 * Принять одну запись заданного маршрута, проверить запрос и забрать окружение. */
static DVM_LaunchHandoff *receive_launch(StyxClient *client, DVM_Provider *root,
    const TerminalLaunchConfig *config, DVM_Provider *inbox)
{
    DVM_LaunchSpec spec = {0};
    spec.policy = DVM_LAUNCH_OWNER_FIRST;
    spec.terminal_origin.role = DVM_ORIGIN_TERMINAL;
    DVM_Origin origin = {0};
    origin.role = DVM_ORIGIN_PROVIDER;
    if (!launch_id(spec.terminal_origin.system) || !launch_id(spec.terminal_origin.export_id) ||
        !launch_id(spec.terminal_origin.session) || !launch_id(origin.system) ||
        !launch_id(origin.export_id))
    {
        return NULL;
    }
    strcpy(spec.module, config->label);
    strcpy(spec.expected_provider, origin.system);
    strcpy(origin.session, spec.terminal_origin.session);

    DVM_TerminalChannels ingress = {0};
    ingress.provider = root;
    strcpy(ingress.receive, config->request);
    strcpy(ingress.send, config->paths.send);
    const StyxRouteType type = {"disvm.launch", 1};
    DVM_Provider *code = code_root(root, config->code, config->dependency, inbox);
    DVM_LaunchService *service = code ? dvm_launch_service_create(spec.expected_provider, 1) : NULL;
    DVM_LaunchEndpoint *endpoint = service ? dvm_launch_service_endpoint(service) : NULL;
    StyxBidirRoute *route = endpoint ? styxbidirbindingroutes(client, &ingress, 4096, 2, NULL, 0, &type, 1) : NULL;
    DVM_LaunchTicket *ticket = NULL;
    DVM_LaunchHandoff *handoff = NULL;
    unsigned char payload[4096];
    size_t size;
    if (route && styxbidirroutetypedwait(route, 0, config->timeout) &&
        styxbidirroutelaunchnext(route, 0, &spec, FVM_ACCESS_MASK, &config->paths, endpoint, config->tag,
            payload, sizeof(payload), &size, &ticket) == STYX_CONTROL_RECORD)
    {
        DVM_LaunchReview *review = dvm_launch_service_next(service);
        DVM_LaunchRequest *request = review ? dvm_launch_review_request(review) : NULL;
        if (request && dvm_launch_request_prepare(request, code, &origin))
        {
            handoff = dvm_launch_request_take(request);
        }
        dvm_launch_review_free(review);
    }
    styxbidirroutefree(route);
    dvm_launch_ticket_free(ticket);
    dvm_launch_endpoint_free(endpoint);
    dvm_launch_service_free(service);
    dvm_provider_release(code);
    return handoff;
}


/* EN/RU: Run the deterministic one-shot profile and retire owners in reverse order.
 * Выполнить профиль одного запуска и освободить владельцев в обратном порядке. */
int dvm_terminal_launch_cli(int argc, char **argv)
{
    TerminalLaunchConfig config;
    if (!dvm_terminal_launch_config(argc, argv, &config))
    {
        fprintf(stderr, "usage: disvm --terminal-launch-fd FD LABEL CODE-FILE DEP-FILE-OR-DASH "
            "[--request PATH TAG] [--records RX TX] [--lane TAG] [--timeout-ms MS] "
            "[--grant PATH TAG r|w|rw]... [--fd-grant PATH NODE r|w|rw]... "
            "[--inherit-fd NUMBER r|w|rw]... -- [VM OPTIONS] BOOTSTRAP.dis\n");
        return 2;
    }
    int fd = dvm_terminal_connected(config.fd);
    if (fd < 0)
    {
        fprintf(stderr, "disvm: expected a delegated connected stream FD >= 3\n");
        return 2;
    }
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
    {
        close(fd);
        return 1;
    }
    PhyBidir *mux = phybidircreate(phyposixfd(fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), 65536);
    StyxClient *client = NULL;
    StyxUnibusServer *reverse = NULL;
    if (mux)
    {
        reverse = styxunibusserverstart(phybidirchannel(mux, 1), STYX_UNIBUS_REPLY_TAG);
        if (reverse)
        {
            client = styxclientmountphy(phybidirchannel(mux, 0), 4096, "", "", config.timeout, 8);
        }
    }
    DVM_Provider *root = client && styxclientretain(client) ?
        styxmountproviderat(client, ".", FVM_ACCESS_MASK) : NULL;
    DVM_LaunchHandoff *handoff = root ? receive_launch(client, root, &config, styxunibusserverinbox(reverse)) : NULL;
    const StyxOrderedOptions ordered = {sizeof(ordered), 128, 16, config.lane, config.tags, config.count};
    StyxOrderedFiles *files = handoff ? styxorderedlaunchfileswithfds(client, handoff, &ordered,
        config.files, config.count, config.fds, config.fd_count) : NULL;
    int status = 1;
    if (files)
    {
        memcpy(handoff->inherited_fds, config.inherited_fds, sizeof(config.inherited_fds));
        status = dvm_runtime_main_with_handoff(argc - config.separator, argv + config.separator, handoff);
    }
    else
    {
        fprintf(stderr, "disvm: terminal launch admission failed\n");
    }
    dvm_launch_handoff_free(handoff);
    if (!styxorderedfilesfree(files))
    {
        fprintf(stderr, "disvm: terminal file set still has live users\n");
        status = 1;
    }
    dvm_provider_release(root);
    if (client)
    {
        styxclientshutdown(client);
        styxclientfree(client);
    }
    if (reverse)
    {
        shutdown(fd, SHUT_RDWR);
        if (!styxunibusserverjoin(reverse))
        {
            status = 1;
        }
    }
    phybidirfree(mux);
    return status;
}
