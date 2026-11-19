/* Copyright (c) 2026 bhgv
 * EN: POSIX descriptor driver; socket-only flags and poll/pipe details stay below the PHY boundary.
 * RU: POSIX descriptor driver; socket flags и poll/pipe остаются ниже PHY boundary.
 */
#define _POSIX_C_SOURCE 200809L
#include "fd.h"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

/* EN/RU: One native I/O owner; wake writers only touch the immutable pipe descriptor.
 * Один native I/O owner; wake writers используют только immutable pipe descriptor. */
typedef struct Fd
{
    int data;    /* Owned granted descriptor until disconnect. */
    int wake[2]; /* Owned nonblocking readiness, alive through final destroy. */
    int socket;  /* Socket writes require MSG_NOSIGNAL; byte devices use ordinary write. */
} Fd;

/* EN/RU: Convert one native syscall result; zero read is explicit EOF, zero write is failure.
 * Преобразовать syscall result; zero read — EOF, zero write — failure. */


/* [standalone-contract:transferred]
 * EN: transferred: posix/fd implementation and native state.
 * RU: transferred: реализация posix/fd и нативное состояние.
 * count (ssize_t count): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * writing (int writing): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: PhyTransfer; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: PhyTransfer; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static PhyTransfer transferred(ssize_t count, int writing)
{
    PhyTransfer out = {0, PHY_ERROR, PHY_PREFIX};
    if (count > 0)
    {
        out.count = (size_t)count;
        out.status = PHY_OK;
    }
    else if (count < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK))
    {
        out.status = PHY_AGAIN;
    }
    else if (!count && !writing)
    {
        out.status = PHY_EOF;
    }
    return out;
}

/* EN/RU: Nonblocking granted-fd read; bytes borrowed until return, no retry/deadline renewal.
 * Nonblocking read granted fd; bytes borrowed, без retry/deadline renewal. */


/* [standalone-contract:readbytes]
 * EN: readbytes: posix/fd implementation and native state.
 * RU: readbytes: реализация posix/fd и нативное состояние.
 * state (void *state): Opaque callback context; its lifetime/transfer is specified by the creating API.
 * RU: Непрозрачный контекст callback; время жизни/передача заданы создающим API.
 * bytes (void *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * Return: PhyTransfer; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: PhyTransfer; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static PhyTransfer readbytes(void *state, void *bytes, size_t size)
{
    return transferred(read(((Fd *)state)->data, bytes, size), 0);
}

/* EN/RU: Nonblocking exact-prefix write; socket SIGPIPE is suppressed without global signal changes.
 * Nonblocking exact-prefix write; socket SIGPIPE подавлен без global signal changes. */


/* [standalone-contract:writebytes]
 * EN: writebytes: posix/fd implementation and native state.
 * RU: writebytes: реализация posix/fd и нативное состояние.
 * state (void *state): Opaque callback context; its lifetime/transfer is specified by the creating API.
 * RU: Непрозрачный контекст callback; время жизни/передача заданы создающим API.
 * bytes (const void *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * Return: PhyTransfer; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: PhyTransfer; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static PhyTransfer writebytes(void *state, const void *bytes, size_t size)
{
    Fd *fd = state;
    ssize_t count = fd->socket ? send(fd->data, bytes, size, MSG_NOSIGNAL) : write(fd->data, bytes, size);
    return transferred(count, 1);
}

/* EN/RU: Wait for native data/space/local interrupt; drain only our own wake pipe.
 * Owner rechecks protocol deadlines after EINTR; timeout is never restarted here.
 * Ждать data/space/local interrupt; drain только своего pipe; EINTR не обновляет deadline. */


/* [standalone-contract:waitready]
 * EN: waitready: posix/fd implementation and native state.
 * RU: waitready: реализация posix/fd и нативное состояние.
 * state (void *state): Opaque callback context; its lifetime/transfer is specified by the creating API.
 * RU: Непрозрачный контекст callback; время жизни/передача заданы создающим API.
 * wanted (unsigned wanted): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * timeout (int timeout): Native wait budget in milliseconds; zero and upper-bound behavior follow the function contract.
 * RU: Бюджет нативного ожидания в миллисекундах; поведение нуля и верхнего предела задано контрактом.
 * ready (unsigned *ready): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int waitready(void *state, unsigned wanted, int timeout, unsigned *ready)
{
    Fd *fd = state;
    struct pollfd watched[2] = {
        {fd->data, ((wanted & PHY_READ) ? POLLIN : 0) | ((wanted & PHY_WRITE) ? POLLOUT : 0), 0},
        {fd->wake[0], POLLIN, 0}};
    int count = poll(watched, 2, timeout);
    if (count < 0)
    {
        *ready = 0;
        return errno == EINTR ? PHY_OK : PHY_ERROR;
    }
    *ready = 0;
    if (watched[1].revents & POLLIN)
    {
        char bytes[128];
        ssize_t received;
        do
        {
            received = read(fd->wake[0], bytes, sizeof(bytes));
        } while (received < 0 && errno == EINTR);
        *ready |= PHY_WAKE;
    }
    if (watched[0].revents & POLLIN)
    {
        *ready |= PHY_READ;
    }
    if (watched[0].revents & POLLOUT)
    {
        *ready |= PHY_WRITE;
    }
    if (watched[0].revents & (POLLHUP | POLLERR))
    {
        *ready |= PHY_HANGUP;
    }
    if ((watched[0].revents & POLLNVAL) || (watched[1].revents & (POLLHUP | POLLERR | POLLNVAL)))
    {
        *ready |= PHY_FAILED;
    }
    return PHY_OK;
}

/* EN/RU: Thread-safe nonwaiting local interrupt; full pipe already means ready.
 * Thread-safe nonwaiting local interrupt; full pipe уже означает ready. */


/* [standalone-contract:wake]
 * EN: wake: posix/fd implementation and native state.
 * RU: wake: реализация posix/fd и нативное состояние.
 * state (void *state): Opaque callback context; its lifetime/transfer is specified by the creating API.
 * RU: Непрозрачный контекст callback; время жизни/передача заданы создающим API.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void wake(void *state)
{
    Fd *fd = state;
    char byte = 1;
    ssize_t count;
    do
    {
        count = write(fd->wake[1], &byte, 1);
    } while (count < 0 && errno == EINTR);
}

/* EN/RU: Query available native bytes, never infer EOF from zero or permission from metadata.
 * Запросить available bytes; zero не означает EOF, metadata не даёт permission. */


/* [standalone-contract:available]
 * EN: available: posix/fd implementation and native state.
 * RU: available: реализация posix/fd и нативное состояние.
 * state (void *state): Opaque callback context; its lifetime/transfer is specified by the creating API.
 * RU: Непрозрачный контекст callback; время жизни/передача заданы создающим API.
 * bytes (size_t *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int available(void *state, size_t *bytes)
{
    int count;
    if (ioctl(((Fd *)state)->data, FIONREAD, &count))
    {
        return errno == ENOTTY || errno == EINVAL ? PHY_UNSUPPORTED : PHY_ERROR;
    }
    if (count < 0)
    {
        return PHY_ERROR;
    }
    *bytes = (size_t)count;
    return PHY_OK;
}

/* EN/RU: Disconnect granted wire once; local wake remains live for already pinned callers.
 * Закрыть granted wire один раз; local wake остаётся для pinned callers. */


/* [standalone-contract:disconnect]
 * EN: disconnect: posix/fd implementation and native state.
 * RU: disconnect: реализация posix/fd и нативное состояние.
 * state (void *state): Opaque callback context; its lifetime/transfer is specified by the creating API.
 * RU: Непрозрачный контекст callback; время жизни/передача заданы создающим API.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void disconnect(void *state)
{
    Fd *fd = state;
    if (fd->data >= 0)
    {
        close(fd->data);
        fd->data = -1;
    }
}

/* EN/RU: Consume native descriptors/state after all I/O/wake users retire.
 * Поглотить descriptors/state после завершения всех I/O/wake users. */


/* [standalone-contract:destroy]
 * EN: destroy: posix/fd implementation and native state.
 * RU: destroy: реализация posix/fd и нативное состояние.
 * state (void *state): Opaque callback context; its lifetime/transfer is specified by the creating API.
 * RU: Непрозрачный контекст callback; время жизни/передача заданы создающим API.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void destroy(void *state)
{
    Fd *fd = state;
    disconnect(fd);
    if (fd->wake[0] >= 0)
    {
        close(fd->wake[0]);
    }
    if (fd->wake[1] >= 0)
    {
        close(fd->wake[1]);
    }
    free(fd);
}

/* EN/RU: Preserve other native descriptor flags while setting NONBLOCK/CLOEXEC;1/0.
 * Сохранить прочие descriptor flags при NONBLOCK/CLOEXEC;1/0. */


/* [standalone-contract:configure]
 * EN: configure: posix/fd implementation and native state.
 * RU: configure: реализация posix/fd и нативное состояние.
 * fd (int fd): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int configure(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    int descriptor = fcntl(fd, F_GETFD);
    return flags >= 0 && descriptor >= 0 && !fcntl(fd, F_SETFL, flags | O_NONBLOCK) &&
           !fcntl(fd, F_SETFD, descriptor | FD_CLOEXEC);
}

/* EN/RU: See fd.h; callbacks/native wake are fully initialized before publishing the adapter.
 * См. fd.h; callbacks/native wake готовы до публикации adapter. */


/* [standalone-contract:phyposixfd]
 * EN: phyposixfd: posix/fd implementation and native state.
 * RU: phyposixfd: реализация posix/fd и нативное состояние.
 * descriptor (int descriptor): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * capabilities (unsigned capabilities): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: RemotePhy *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: RemotePhy *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
RemotePhy *phyposixfd(int descriptor, unsigned capabilities)
{
    static const PhyOps ops = {readbytes, writebytes, waitready, wake, available, NULL, disconnect, destroy};
    Fd *fd = calloc(1, sizeof(*fd));
    if (!fd)
    {
        if (descriptor >= 0)
        {
            close(descriptor);
        }
        return NULL;
    }
    fd->data = descriptor;
    fd->wake[0] = fd->wake[1] = -1;
    int flags = fcntl(descriptor, F_GETFL);
    int kind;
    socklen_t size = sizeof(kind);
    fd->socket = getsockopt(descriptor, SOL_SOCKET, SO_TYPE, &kind, &size) == 0;
    struct linger linger = {0, 0};
    if (!(capabilities & PHY_STREAM) || (capabilities & (PHY_PACKET | PHY_BIDIR_SESSION)) || descriptor < 0 ||
        flags < 0 || (flags & O_ACCMODE) != O_RDWR ||
        (fd->socket &&
         (kind != SOCK_STREAM || setsockopt(descriptor, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)))) ||
        !configure(descriptor) || pipe(fd->wake) || !configure(fd->wake[0]) || !configure(fd->wake[1]))
    {
        destroy(fd);
        return NULL;
    }
    PhyInfo info = {capabilities, 0, 0, 0, 1};
    RemotePhy *phy = phycreate(&ops, fd, &info);
    if (!phy)
    {
        destroy(fd);
    }
    return phy;
}
