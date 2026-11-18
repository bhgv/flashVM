/* Copyright (c) 2026 bhgv
 * EN: Adopt a delegated POSIX stream through RemotePhy/P9MX, capture Styx root and negotiate the selected broker.
 * RU: Принять выданный POSIX-stream через RemotePhy/P9MX, закрепить Styx-корень и согласовать брокер.
 */
#include "session.h"
#include "drivers/posix/fd.h"
#include "mount.h"
#include "order/broker/attach.h"
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>


/* EN/RU: See session.h; consume only locally created references, never the caller's original descriptor number.
 * См. session.h; поглотить лишь созданные здесь refs, не исходный номер descriptor caller.
 */
void unibuscli_disconnect(UnibusCliLink *link)
{
    styxorderedfree(link->peer);
    dvm_provider_release(link->grant);
    if (link->client)
    {
        styxclientshutdown(link->client);
        styxclientfree(link->client);
    }
    phybidirfree(link->mux);
    memset(link, 0, sizeof(*link));
}


/* EN/RU: Reject ordinary files/datagrams before protocol writes; character devices require caller-provided stream adaptation.
 * Отвергнуть обычные файлы/datagram до protocol-записи; character devices требуют stream-адаптации caller.
 */
static int stream(int fd)
{
    struct stat attributes; /* Native delegated descriptor metadata only. */
    if (fstat(fd, &attributes))
    {
        return 0;
    }
    for (int standard = 0; standard < 3; standard++)
    {
        struct stat other; /* Application/diagnostic descriptors may not alias the private carrier endpoint. */
        if (!fstat(standard, &other) && attributes.st_dev == other.st_dev && attributes.st_ino == other.st_ino &&
            (attributes.st_mode & S_IFMT) == (other.st_mode & S_IFMT))
        {
            return 0;
        }
    }
    if (S_ISCHR(attributes.st_mode))
    {
        return 1;
    }
    if (S_ISSOCK(attributes.st_mode))
    {
        int type; /* Exact native socket semantics, not inferred from a path. */
        socklen_t size = sizeof(type); /* Native getsockopt extent. */
        return !getsockopt(fd, SOL_SOCKET, SO_TYPE, &type, &size) && type == SOCK_STREAM;
    }
    return 0;
}


/* EN/RU: See session.h; all path/profile parsing finished before acquiring a carrier or walking a fid.
 * См. session.h; весь parse путей/профиля завершён до получения carrier и walk fid.
 */
int unibuscli_connect(const UnibusCliConfig *config, UnibusCliLink *link)
{
    memset(link, 0, sizeof(*link));
    if (!stream(config->fd))
    {
        return 0;
    }
    int fd = fcntl(config->fd, F_DUPFD_CLOEXEC, 3); /* Owned duplicate, never steals stdin/stdout/stderr. */
    if (fd < 0)
    {
        return 0;
    }
    link->mux = phybidircreate(phyposixfd(fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), 65536);
    if (!link->mux)
    {
        return 0;
    }
    RemotePhy *unused = phybidirchannel(link->mux, 1U - config->channel); /* No reverse filesystem authority exported by this tool. */
    phyfree(unused);
    link->client = styxclientmountphy(phybidirchannel(link->mux, config->channel), 4096, "", "", config->timeout, 8);
    if (!link->client || !styxclientretain(link->client))
    {
        unibuscli_disconnect(link);
        return 0;
    }
    link->grant = styxmountproviderat(link->client, config->root, FVM_ACCESS_MASK); /* Consumes extra retained client reference. */
    DVM_TerminalChannels channels = {0}; /* Captured exact association, no ambient VFS fallback. */
    channels.provider = link->grant;
    strcpy(channels.receive, config->receive);
    strcpy(channels.send, config->send);
    StyxOrderedOptions options = {0}; /* Shared native attachment profile, independent of display and CLI parsing. */
    options.struct_size = sizeof(options);
    options.limit = config->limit;
    options.window = config->window;
    options.lane = config->lane;
    options.tags = config->tags;
    options.count = config->count;
    link->peer = styxbidirorderedchannels(link->client, &channels, &options);
    if (!link->peer)
    {
        unibuscli_disconnect(link);
        return 0;
    }
    for (unsigned direction = STYX_ORDER_RX; direction <= STYX_ORDER_TX; direction++)
    {
        if (!styxorderedwait(link->peer, direction, config->timeout) &&
            !(styxorderedstate(link->peer) == STYX_CONTROL_EOF && styxorderedagreed(link->peer, direction)))
        {
            unibuscli_disconnect(link);
            return 0;
        }
    }
    return 1;
}
