/* Copyright (c) 2026 bhgv
 * EN: Observe socket creation mask and restoration without requiring AF_UNIX permission.
 * RU: Наблюдать socket creation mask и восстановление без разрешения AF_UNIX.
 */
#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

static int owned = -1;  /* Private fake socket FD; no VM threads start after bind refusal. */
static mode_t original; /* Launch-time umask sampled before socket bind. */
int __real_close(int);


/* EN/RU: Replace only startup Unix socket with a real pipe FD, preserving FD ownership.
 * Заменить только startup Unix socket настоящим pipe FD, сохраняя FD ownership. */
int __wrap_socket(int domain, int type, int protocol)
{
    int descriptors[2];
    (void)type;
    (void)protocol;
    if (domain != AF_UNIX || pipe(descriptors))
    {
        errno = EOPNOTSUPP;
        return -1;
    }
    original = umask(0);
    umask(original);
    owned = descriptors[0];
    __real_close(descriptors[1]);
    return owned;
}


/* EN: Observe actual bind-time mode of a Unix socket's0777 base. Refuse bind
 * intentionally, so no fake socket inode or listener is ever admitted.
 * RU: Проверить actual bind-time mode относительно socket base0777. Намеренно
 * отказать bind: fake socket inode/listener никогда не допускается. */
int __wrap_bind(int descriptor, const struct sockaddr *address, socklen_t size)
{
    mode_t mask = umask(0);
    (void)address;
    (void)size;
    umask(mask);
    if (descriptor == owned)
    {
        fprintf(stderr, "DEBUGSOCKETMASK mask=%04o mode=%04o\n", (unsigned int)mask, (unsigned int)(0777 & ~mask));
    }
    errno = EOPNOTSUPP;
    return -1;
}


/* EN/RU: Observe restoration before fake FD cleanup; forward every genuine close exactly once.
 * Проверить restoration до cleanup fake FD; передать каждый close ровно один раз. */
int __wrap_close(int descriptor)
{
    if (descriptor == owned)
    {
        mode_t mask = umask(0);
        umask(mask);
        fprintf(stderr, "DEBUGSOCKETRESTORE matched=%d\n", mask == original);
        owned = -1;
    }
    return __real_close(descriptor);
}
