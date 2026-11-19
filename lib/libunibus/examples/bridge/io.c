/* Copyright (c) 2026 bhgv
 * EN: Exact opaque I/O and serialized complete output records. Queues, backend
 * execution and the RX owner do not share the physical output mutex.
 * RU: Точный непрозрачный I/O и сериализация целых записей вывода. Очереди,
 * backend execution и RX owner не используют mutex физического вывода.
 * standalone-reviewed: physical backpressure differs from waiting for a client's message.
 */
#include "bridge.h"
#include "flashvm/srecord.h"
#include <unistd.h>
#include <errno.h>
#include <poll.h>

static pthread_mutex_t output_lock = PTHREAD_MUTEX_INITIALIZER; /* Only complete stdout records, never backend I/O or RX dispatch. */
static int output_dead; /* Protected by output_lock: an incomplete record poisons this physical direction until process exit. */


/* EN: Transfer borrowed fd/bytes/size in writing direction. output selects an
 * unbounded physical-output readiness wait; backend EAGAIN remains a socket timeout.
 * Return 1/0, preserving partial effects. No descriptor flags or ownership changes.
 * RU: Передать fd/bytes/size в направлении writing. output выбирает ожидание
 * физического вывода; backend EAGAIN остаётся socket timeout. Вернуть 1/0;
 * partial effects сохраняются, flags/владение fd не меняются.
 */
static int transfer(int fd, void *bytes, size_t size, int writing, int output)
{
    unsigned char *cursor = bytes;
    while (size)
    {
        ssize_t amount = writing ? write(fd, cursor, size) : read(fd, cursor, size);
        if (amount < 0 && errno == EINTR)
        {
            continue;
        }
        if (amount < 0 && output && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            struct pollfd writable = {fd, POLLOUT, 0};
            int ready;
            do
            {
                ready = poll(&writable, 1, -1); /* Borrowed stdout may already be nonblocking; preserve its flags. */
            } while (ready < 0 && errno == EINTR);
            if (ready > 0 && (writable.revents & POLLOUT) && !(writable.revents & (POLLERR | POLLHUP | POLLNVAL)))
            {
                continue; /* Resume only the remaining bytes after physical backpressure clears. */
            }
            return 0;
        }
        if (amount <= 0)
        {
            return 0; /* Bytes already transferred remain transferred; callers cannot infer an undone backend effect. */
        }
        cursor += amount;
        size -= amount;
    }
    return 1;
}


/* EN/RU: See bridge.h; backend syscall timeout is terminal, not an infinite poll retry.
 * См. bridge.h; backend syscall timeout — отказ, не бесконечный повтор через poll.
 */
int bridge_exact(int fd, void *bytes, size_t size, int writing)
{
    return transfer(fd, bytes, size, writing, 0);
}


/* EN/RU: See bridge.h; encode before locking, then preserve whole-record output.
 * См. bridge.h; кодировать до lock, затем сохранять целостность записи вывода.
 */
int bridge_record(unsigned long tag, const void *payload, size_t size)
{
    unsigned char output[BRIDGE_LIMIT + 256];
    size_t used;
    if (fvm_srecord_write(tag, payload, size, output, sizeof(output), &used) != FVM_S_OK)
    {
        return 0;
    }
    pthread_mutex_lock(&output_lock);
    int result = !output_dead && transfer(STDOUT_FILENO, output, used, 1, 1); /* Never append another record after an uncertain partial write. */
    if (!result)
    {
        output_dead = 1; /* Release the mutex, but never reuse the damaged stream direction. */
    }
    pthread_mutex_unlock(&output_lock);
    return result;
}


/* EN/RU: See bridge.h; nested application errors never occupy outer service tag 0.
 * См. bridge.h; вложенные ошибки приложения не занимают внешний служебный tag 0.
 */
int bridge_reply(unsigned long tag, unsigned long inner, const void *bytes, size_t size)
{
    unsigned char output[BRIDGE_LIMIT + 128];
    size_t used;
    return fvm_srecord_write(inner, bytes, size, output, sizeof(output), &used) == FVM_S_OK &&
        bridge_record(tag, output, used);
}


/* EN/RU: See bridge.h; byte operations avoid alignment and host-endian assumptions.
 * См. bridge.h; побайтовая операция не зависит от alignment и host endian.
 */
unsigned bridge_length(const unsigned char *bytes)
{
    return (unsigned)bytes[0] | ((unsigned)bytes[1] << 8) | ((unsigned)bytes[2] << 16) | ((unsigned)bytes[3] << 24);
}
