/* Copyright (c) 2026 bhgv
 * EN: Link-time acceptance guard: the real memory-PHY example must not use project
 * descriptor/network paths. Console writes remain allowed; this is not a kernel syscall trace.
 * RU: Link-time guard: настоящий memory-PHY пример не использует descriptor/network
 * paths проекта. Console writes разрешены; это не kernel syscall trace.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sys/socket.h>
#include <sys/eventfd.h>
#include <poll.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>


/* EN/RU: Borrow operation name, diagnose and terminate without touching carrier state.
 * Заимствовать имя operation, сообщить и завершить процесс без доступа к carrier state.
 */
static void forbidden(const char *operation)
{
    fprintf(stderr, "unexpected descriptor/network operation: %s\n", operation);
    _Exit(9);
}


/* EN/RU: Parameters are borrowed but must never reach an OS socket factory in this test.
 * domain/type/protocol не должны попасть в OS socket factory этого теста.
 */
int __wrap_socket(int domain, int type, int protocol)
{
    (void)domain;
    (void)type;
    (void)protocol;
    forbidden("socket");
    return -1;
}


/* EN/RU: fd/address/size identify a forbidden network connection; no ownership changes.
 * fd/address/size — запрещённое сетевое подключение; владение не меняется.
 */
int __wrap_connect(int fd, const struct sockaddr *address, socklen_t size)
{
    (void)fd;
    (void)address;
    (void)size;
    forbidden("connect");
    return -1;
}


/* EN/RU: descriptors/flags/event are forbidden descriptor creation arguments, not PHY state.
 * descriptors/flags/event — запрещённые аргументы создания fd, не PHY state.
 */
int __wrap_pipe(int *descriptors)
{
    (void)descriptors;
    forbidden("pipe");
    return -1;
}


/* EN/RU: descriptors/flags cannot create a hidden local-wake pipe in the memory driver.
 * descriptors/flags не могут создать скрытый wake pipe в memory driver.
 */
int __wrap_pipe2(int *descriptors, int flags)
{
    (void)descriptors;
    (void)flags;
    forbidden("pipe2");
    return -1;
}


/* EN/RU: event/flags cannot create a hidden event descriptor; wake is a binary futex latch.
 * event/flags не могут создать скрытый event fd; wake — binary futex latch.
 */
int __wrap_eventfd(unsigned event, int flags)
{
    (void)event;
    (void)flags;
    forbidden("eventfd");
    return -1;
}


/* EN/RU: descriptors/count/timeout belong to a forbidden descriptor readiness path.
 * descriptors/count/timeout принадлежат запрещённому пути descriptor readiness.
 */
int __wrap_poll(struct pollfd *descriptors, nfds_t count, int timeout)
{
    (void)descriptors;
    (void)count;
    (void)timeout;
    forbidden("poll");
    return -1;
}


/* EN/RU: descriptor carrier reads are forbidden, including accidental use of token 0/1/2 as an OS fd.
 * Чтения носителя через fd запрещены, включая ошибочную трактовку token 0/1/2 как OS fd.
 */
ssize_t __wrap_read(int fd, void *bytes, size_t size)
{
    (void)fd;
    (void)bytes;
    (void)size;
    forbidden("read");
    return -1;
}


/* EN/RU: No driver owns an OS fd in this example, so no project-level close is valid.
 * Драйвер не владеет OS fd; project-level close в этом примере недопустим.
 */
int __wrap_close(int fd)
{
    (void)fd;
    forbidden("close");
    return -1;
}


/* EN/RU: path/flags are borrowed forbidden file-open arguments; no extra varargs are accessed.
 * path/flags — заимствованные запрещённые file-open аргументы; varargs не читаются.
 */
int __wrap_open(const char *path, int flags, ...)
{
    (void)path;
    (void)flags;
    forbidden("open");
    return -1;
}


/* EN/RU: domain/type/protocol/descriptors cannot create a hidden pair for memory transport.
 * domain/type/protocol/descriptors не могут создать скрытую пару для memory transport.
 */
int __wrap_socketpair(int domain, int type, int protocol, int *descriptors)
{
    (void)domain;
    (void)type;
    (void)protocol;
    (void)descriptors;
    forbidden("socketpair");
    return -1;
}
