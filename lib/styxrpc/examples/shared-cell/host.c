/* Copyright (c) 2026 bhgv
 * EN: Host a shared-cell registry for C and Limbo, demonstrate assignment,
 * cancellation, and the lifetime of the token on a real Styx connection.
 * RU: Обслужить реестр общих ячеек для C и Limbo и показать запись, отмену
 * и время жизни жетона на настоящем соединении Styx.
 */
#define _POSIX_C_SOURCE 200809L /* CLOCK_MONOTONIC for the caller's deadline. */
#include "styxrpc/named_cell.h"
#include "styxrpc/named_peer.h"
#include "drivers/posix/fd.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <time.h>

#define CELL_CHANNEL "/settings" /* Rendezvous used to transfer the token and order the example. */
#define CELL_MSIZE 4096U /* Requested local Styx message size in bytes. */
#define CELL_REQUEST_MS 2000U /* Timeout of an individual Styx request. */
#define CELL_TAGS 4U /* Maximum pending requests on the local mount. */
#define CELL_EXCHANGE_SECONDS 8 /* Deadline for all steps on one monotonic clock. */
#define CELL_RETIRE_POLL_NS 5000000L /* Wait between successful probes for DisVM fid retirement. */
#define CELL_STREAM (PHY_STREAM | PHY_ORDERED | PHY_RELIABLE) /* A connected byte stream. */

static const unsigned char enabled[] = {STYXRPC_NAMED_BOOLEAN, 1};  /* Complete Boolean frame. */
static const unsigned char disabled[] = {STYXRPC_NAMED_BOOLEAN, 0}; /* Value written by Limbo. */


/* EN: Fill exactly size bytes with Linux host entropy. The registry borrows
 * no extra state, and a refusal prevents the server from issuing a token.
 * RU: Заполнить ровно size байтов энтропией Linux. Дополнительное состояние
 * не нужно. При отказе реестр не выдаёт жетон. */
static int entropy(void *state, unsigned char *bytes, size_t size)
{
    (void)state;
    while (size)
    {
        ssize_t count = getrandom(bytes, size, 0); /* Let the host choose an unpredictable nonce. */
        if (count < 0 && errno == EINTR)
        {
            continue; /* An interrupted call did not fill this suffix. */
        }
        if (count <= 0)
        {
            return 0;
        }
        bytes += count;
        size -= (size_t)count;
    }
    return 1;
}


/* EN: Check a borrowed absolute monotonic deadline on the calling thread.
 * RU: Проверить заимствованный абсолютный срок в потоке вызова. */
static int deadline_reached(void *state)
{
    const struct timespec *end = state;
    struct timespec now;
    return clock_gettime(CLOCK_MONOTONIC, &now) != 0 || now.tv_sec > end->tv_sec ||
        (now.tv_sec == end->tv_sec && now.tv_nsec >= end->tv_nsec);
}


/* EN: Supply an already requested stop for one deterministic pre-write example.
 * RU: Вернуть уже выставленный флаг отмены для примера остановки перед записью. */
static int already_stopped(void *state)
{
    (void)state;
    return 1;
}


/* EN: Send one counted text byte over the borrowed channel, stopping at end.
 * RU: Передать один байт текста через заимствованный канал до срока end. */
static int notify(StyxRpcVfsFile *channel, unsigned char message, struct timespec *end)
{
    const unsigned char frame[] = {STYXRPC_NAMED_TEXT, message};
    return styxrpc_named_send_until(channel, frame, sizeof(frame), deadline_reached, end) == STYXRPC_IO_OK;
}


/* EN: Accept a complete note on the borrowed channel and release its frame.
 * RU: Принять полное уведомление через заимствованный канал и освободить кадр. */
static int await(StyxRpcVfsFile *channel, unsigned char expected, struct timespec *end)
{
    unsigned char *frame = NULL;
    size_t size = 0;
    int ok = styxrpc_named_recv_until(channel, &frame, &size, deadline_reached, end) == STYXRPC_IO_OK &&
        size == 2 && frame[0] == STYXRPC_NAMED_TEXT && frame[1] == expected;
    free(frame);
    return ok;
}


/* EN: Read by token with a temporary fid and compare both Boolean bytes.
 * A separate lease must keep the cell live on the borrowed session.
 * RU: Прочитать по жетону через временный fid и сверить оба байта Boolean.
 * Отдельная аренда должна удерживать ячейку в заимствованной сессии. */
static int reads_disabled(struct StyxClient *session, const unsigned char *token, struct timespec *end)
{
    unsigned char *frame = NULL;
    size_t size = 0;
    int ok = styxrpc_cell_read_until(session, token, &frame, &size, deadline_reached, end) == STYXRPC_IO_OK &&
        size == sizeof(disabled) && !memcmp(frame, disabled, size);
    free(frame);
    return ok;
}


/* EN: Observe revocation on the borrowed session before end. While the
 * peer's clunk is in transit, an accepted probe owns a lease. Close that lease
 * before probing again. Stop immediately on the first unacknowledged attach.
 * RU: Проверить отзыв в заимствованной сессии до end. Пока clunk другой
 * стороны в пути, успешная проверка удерживает аренду. Закрыть её перед
 * следующей проверкой. Остановиться при первом неподтверждённом входе. */
static int retired_token(struct StyxClient *session, const unsigned char *token, struct timespec *end)
{
    const struct timespec interval = {0, CELL_RETIRE_POLL_NS};
    while (!deadline_reached(end))
    {
        StyxRpcVfsFile *probe = NULL;
        StyxRpcIoOutcome outcome = styxrpc_cell_attach_until(session, token, &probe, deadline_reached, end);
        styxrpc_vfs_close(probe, 0);
        if (outcome != STYXRPC_IO_OK)
        {
            /* EN: Rerror can lack positive write acknowledgement. The next
             * rendezvous checks that the attachment is still usable.
             * RU: При Rerror возможна потеря положительного подтверждения.
             * Следующее сообщение проверяет работоспособность подключения. */
            return outcome == STYXRPC_IO_FAILED || outcome == STYXRPC_IO_UNCERTAIN;
        }
        nanosleep(&interval, NULL);
    }
    return 0;
}


/* EN: Transfer an alias, demonstrate early cancellation, and retire every
 * lease before checking the old token. Channel, session and deadline stay borrowed.
 * RU: Передать ссылку, показать раннюю отмену и закрыть все аренды перед
 * проверкой старого жетона. Канал, сессия и срок заимствуются. */
static int exchange(struct StyxClient *session, StyxRpcVfsFile *channel, struct timespec *end)
{
    unsigned char descriptor[1 + STYXRPC_CELL_TOKEN] = {STYXRPC_NAMED_CELL_REF};
    unsigned char *token = descriptor + 1; /* Tag 8 precedes the byte-for-byte token. */
    StyxRpcVfsFile *owner = NULL;
    StyxRpcVfsFile *alias = NULL;
    int ok = styxrpc_cell_create_until(session, enabled, sizeof(enabled), token,
        &owner, deadline_reached, end) == STYXRPC_IO_OK &&
        styxrpc_named_send_until(channel, descriptor, sizeof(descriptor), deadline_reached, end) == STYXRPC_IO_OK &&
        await(channel, 'a', end) && /* Limbo holds an independent lease and has set false. */
        styxrpc_cell_attach_until(session, token, &alias, deadline_reached, end) == STYXRPC_IO_OK;
    styxrpc_vfs_close(owner, 0); /* The new leases can now keep the cell alive. */
    if (!ok)
    {
        fputs("cannot share the cell or acquire the new lease\n", stderr);
        return 0;
    }
    ok = styxrpc_cell_write_until(session, token, enabled, sizeof(enabled), already_stopped, NULL) ==
        STYXRPC_IO_CANCELLED && reads_disabled(session, token, end);
    if (!ok)
    {
        fputs("early stop or readback did not preserve the value\n", stderr);
    }
    ok = ok && styxrpc_cell_write_until(session, token, enabled, sizeof(enabled), deadline_reached, end) ==
        STYXRPC_IO_OK && notify(channel, 'r', end) && await(channel, 'c', end);
    styxrpc_vfs_close(alias, 0); /* Limbo closed its own lease before sending c. */

    if (!ok)
    {
        fputs("the confirmed change or the guest's close did not complete\n", stderr);
        return 0;
    }
    if (!retired_token(session, token, end))
    {
        fputs("the old token did not retire before the deadline\n", stderr);
        return 0;
    }
    /* EN: Wait for the guest to consume d before destroying the serving stream.
     * RU: Дождаться получения d гостем перед остановкой обслуживающего потока. */
    if (!notify(channel, 'd', end) || !await(channel, 'e', end))
    {
        fputs("the final guest acknowledgement did not arrive\n", stderr);
        return 0;
    }
    return 1;
}


/* EN: Consume one connected inherited FD. Return 0 after both peers verify
 * the exchange and all native workers have retired.
 * RU: Принять один унаследованный соединённый FD. Вернуть 0 после проверки
 * обмена обеими сторонами и завершения нативных потоков. */
int main(int argc, char **argv)
{
    char *suffix = NULL;
    errno = 0;
    long fd = argc == 2 ? strtol(argv[1], &suffix, 10) : -1;
    if (argc != 2 || suffix == argv[1] || *suffix || errno || fd < 0 || fd > INT_MAX)
    {
        fputs("usage: styxrpc-shared-cell FD\n", stderr);
        return 2;
    }
    StyxRpcNamedRegistry *registry = styxrpc_named_registry(NULL, NULL, NULL, NULL);
    if (!styxrpc_named_cells_enable(registry, entropy, NULL))
    {
        styxrpc_named_destroy(registry);
        fputs("cannot configure the shared-cell registry\n", stderr);
        return 1;
    }
    StyxRpcNamedPeer *peer = styxrpc_named_peer_host(phyposixfd((int)fd, CELL_STREAM), registry,
        CELL_MSIZE, CELL_REQUEST_MS, CELL_TAGS); /* Consumes the stream. */
    struct StyxClient *session = styxrpc_named_peer_client(peer); /* Borrow through destroy. */
    StyxRpcVfsFile *channel = NULL;
    struct timespec end;
    int ok = session && clock_gettime(CLOCK_MONOTONIC, &end) == 0;
    if (ok)
    {
        end.tv_sec += CELL_EXCHANGE_SECONDS;
        ok = styxrpc_named_open_until(session, CELL_CHANNEL, &channel, deadline_reached, &end) == STYXRPC_IO_OK &&
            exchange(session, channel, &end);
    }
    styxrpc_vfs_close(channel, 0);     /* Retire the final local participant first. */
    styxrpc_named_peer_destroy(peer);  /* Join both owners before freeing their registry. */
    styxrpc_named_destroy(registry);
    if (!ok)
    {
        fputs("shared-cell exchange, deadline or retirement check failed\n", stderr);
        return 1;
    }
    puts("PASS C shared cell cancellation and token retirement");
    return 0;
}
