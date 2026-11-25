/* Copyright (c) 2026 bhgv
 * EN: Open TCP stream descriptors for the Linux native carrier layer.
 * RU: Открытие дескрипторов TCP для нативного транспорта Linux.
 */
#define _GNU_SOURCE
#include "tcp.h"
#include <errno.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define TCP_PORT_LIMIT 65535U   /* Highest decimal port accepted by the CLI/API. */
#define TCP_PENDING_PEERS 1     /* Backlog for a single native connection bootstrap. */
#define TCP_MS_PER_SECOND 1000  /* Monotonic clock conversion. */
#define TCP_NS_PER_MS 1000000   /* Monotonic clock conversion. */


/* EN: Resolve an explicit borrowed host and strictly decimal TCP port.
 * Return getaddrinfo storage owned by the caller or NULL on error.
 * RU: Разрешить заданный хост и строго десятичный порт TCP. Вернуть
 * вызывающему список getaddrinfo либо NULL при ошибке. */
static struct addrinfo *addresses(const char *host, const char *port, int listener)
{
    struct addrinfo hints;
    struct addrinfo *list;
    unsigned value = 0;
    const char *digit;
    if (!host || !*host || !port || !*port)
    {
        return NULL;
    }
    for (digit = port; *digit; ++digit)
    {
        if (*digit < '0' || *digit > '9')
        {
            return NULL;
        }
        value = value * 10 + (unsigned)(*digit - '0');
        if (value > TCP_PORT_LIMIT)
        {
            return NULL;
        }
    }
    if (!listener && !value)
    {
        return NULL;
    }
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_NUMERICSERV;
    return getaddrinfo(host, port, &hints, &list) ? NULL : list;
}


/* EN: Read CLOCK_MONOTONIC in milliseconds, or return -1 on failure.
 * RU: Прочитать CLOCK_MONOTONIC в миллисекундах либо вернуть -1. */
static int64_t now_ms(void)
{
    struct timespec now;
    return clock_gettime(CLOCK_MONOTONIC, &now) ? -1 :
        (int64_t)now.tv_sec * TCP_MS_PER_SECOND + now.tv_nsec / TCP_NS_PER_MS;
}


/* EN: Wait for the borrowed dial socket before the shared monotonic deadline.
 * RU: Дождаться подключения сокета до общего монотонного срока. */
static int connected_before(int fd, int64_t deadline)
{
    struct pollfd pending = {fd, POLLOUT, 0};
    for (;;)
    {
        int64_t now = now_ms();
        if (now < 0 || now >= deadline)
        {
            return 0;
        }
        int ready = poll(&pending, 1, (int)(deadline - now));
        if (ready < 0 && errno == EINTR)
        {
            continue;
        }
        int error;
        socklen_t length = sizeof(error);
        return ready > 0 && !getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &length) && !error;
    }
}


/* EN: Set interactive TCP writes on a connected fd; close on option failure.
 * RU: Настроить интерактивную запись в TCP; закрыть fd при ошибке установки. */
static int interactive(int fd)
{
    int enabled = 1;
    if (!setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enabled, sizeof(enabled)))
    {
        return fd;
    }
    close(fd);
    return -1;
}


/* EN/RU: See tcp.h. / См. tcp.h. */
int phytcpdial(const char *host, const char *port, int timeout_ms)
{
    struct addrinfo *list = timeout_ms > 0 ? addresses(host, port, 0) : NULL;
    if (!list)
    {
        return -1;
    }
    int64_t start = now_ms();
    int64_t deadline = start + timeout_ms;
    int fd = -1;
    for (const struct addrinfo *entry = list; entry && start >= 0; entry = entry->ai_next)
    {
        int64_t now = now_ms();
        if (now < 0 || now >= deadline)
        {
            break;
        }
        fd = socket(entry->ai_family, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, entry->ai_protocol);
        if (fd < 0)
        {
            continue;
        }
        if (!connect(fd, entry->ai_addr, entry->ai_addrlen) ||
            ((errno == EINPROGRESS || errno == EINTR) && connected_before(fd, deadline)))
        {
            fd = interactive(fd);
            if (fd >= 0)
            {
                break;
            }
        }
        else
        {
            close(fd);
            fd = -1;
        }
    }
    freeaddrinfo(list);
    return fd;
}


/* EN/RU: See tcp.h. / См. tcp.h. */
int phytcplisten(const char *host, const char *port, unsigned *bound_port)
{
    struct addrinfo *list = bound_port ? addresses(host, port, 1) : NULL;
    if (!list)
    {
        return -1;
    }
    int fd = -1;
    int enabled = 1;
    for (const struct addrinfo *entry = list; entry; entry = entry->ai_next)
    {
        fd = socket(entry->ai_family, SOCK_STREAM | SOCK_CLOEXEC, entry->ai_protocol);
        if (fd < 0)
        {
            continue;
        }
        struct sockaddr_storage local;
        socklen_t length = sizeof(local);
        if (!setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled)) &&
            !bind(fd, entry->ai_addr, entry->ai_addrlen) && !listen(fd, TCP_PENDING_PEERS) &&
            !getsockname(fd, (struct sockaddr *)&local, &length))
        {
            *bound_port = local.ss_family == AF_INET ? ntohs(((struct sockaddr_in *)&local)->sin_port) :
                ntohs(((struct sockaddr_in6 *)&local)->sin6_port);
            break;
        }
        close(fd);
        fd = -1;
    }
    freeaddrinfo(list);
    return fd;
}


/* EN/RU: See tcp.h. / См. tcp.h. */
int phytcpaccept(int listener)
{
    int fd;
    do
    {
        fd = accept4(listener, NULL, NULL, SOCK_CLOEXEC | SOCK_NONBLOCK);
    }
    while (fd < 0 && errno == EINTR);
    return fd < 0 ? -1 : interactive(fd);
}
