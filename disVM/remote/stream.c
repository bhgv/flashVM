/* Copyright (c) 2026 bhgv
 * EN: Deadline-aware nonblocking socket calls, partial progress and SIGPIPE refusal.
 * RU: Неблокирующие socket calls с deadline, partial progress и защитой от SIGPIPE.
 */
#include "stream.h"
#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <sys/socket.h>
#include <time.h>


/* EN: Read a process-wide monotonic clock without changing caller's output on error.
 * RU: Прочитать monotonic clock; output caller при ошибке не меняется. */
int dvm_remote_now_ms(uint64_t *output)
{
    struct timespec now;
    if (output == NULL)
    {
        return EINVAL;
    }
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
    {
        return errno;
    }
    *output = (uint64_t)now.tv_sec * 1000 + (uint64_t)now.tv_nsec / 1000000;
    return 0;
}


/* EN: Transfer native bytes using one deadline across retries, EINTR and partial
 * reads/writes. Buffer/fd remain borrowed; no callbacks or descriptor ownership.
 * RU: Передать native bytes с одним deadline для retries, EINTR и partial I/O.
 * Buffer/fd заимствованы; callbacks и владения descriptor здесь нет. */
static DvmRemoteIoResult
transfer(int fd, void *read_buffer, const void *write_buffer, size_t capacity, uint64_t deadline, int writing)
{
    DvmRemoteIoResult result = {DVM_REMOTE_IO_OK, 0, 0};
    struct pollfd descriptor;
    if (fd < 0 || capacity > DVM_REMOTE_IO_LIMIT ||
        (capacity != 0 && (writing ? write_buffer == NULL : read_buffer == NULL)))
    {
        result.status = DVM_REMOTE_IO_INVALID;
        return result;
    }
    descriptor.fd = fd;
    descriptor.events = writing ? POLLOUT : POLLIN;
    while (result.transferred < capacity)
    {
        uint64_t now;
        uint64_t remaining;
        ssize_t count;
        int ready;
        int clock_error = dvm_remote_now_ms(&now);
        if (clock_error != 0)
        {
            result.status = DVM_REMOTE_IO_ERROR;
            result.system_error = clock_error;
            break;
        }
        if (now >= deadline)
        {
            result.status = DVM_REMOTE_IO_TIMEOUT;
            break;
        }
        remaining = deadline - now;
        ready = poll(&descriptor, 1, remaining > INT_MAX ? INT_MAX : (int)remaining);
        if (ready < 0 && errno == EINTR)
        {
            continue;
        }
        if (ready < 0 || (ready > 0 && (descriptor.revents & POLLNVAL)))
        {
            result.status = DVM_REMOTE_IO_ERROR;
            result.system_error = ready < 0 ? errno : EBADF;
            break;
        }
        if (ready == 0)
        {
            continue;
        }
        if (writing)
        {
            count = send(fd,
                         (const unsigned char *)write_buffer + result.transferred,
                         capacity - result.transferred,
                         MSG_DONTWAIT | MSG_NOSIGNAL);
        }
        else
        {
            count = recv(
                fd, (unsigned char *)read_buffer + result.transferred, capacity - result.transferred, MSG_DONTWAIT);
        }
        if (count < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK))
        {
            continue;
        }
        if (count < 0)
        {
            result.status = DVM_REMOTE_IO_ERROR;
            result.system_error = errno;
            break;
        }
        if (count == 0)
        {
            result.status = writing ? DVM_REMOTE_IO_ERROR : DVM_REMOTE_IO_EOF;
            result.system_error = writing ? EPIPE : 0;
            break;
        }
        result.transferred += (size_t)count;
    }
    return result;
}


/* EN/RU: Consume socket bytes into borrowed native storage; preserve partial count.
 * Прочитать socket в native storage; сохранить частичный count. No ownership change. */
DvmRemoteIoResult dvm_remote_read_exact(int fd, void *buffer, size_t capacity, uint64_t deadline)
{
    return transfer(fd, buffer, NULL, capacity, deadline, 0);
}


/* EN/RU: Send borrowed native bytes without SIGPIPE or changing descriptor flags.
 * Передать native bytes без SIGPIPE или смены descriptor flags. */
DvmRemoteIoResult dvm_remote_write_exact(int fd, const void *buffer, size_t capacity, uint64_t deadline)
{
    return transfer(fd, NULL, buffer, capacity, deadline, 1);
}
