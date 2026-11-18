/* Copyright (c) 2026 bhgv
 * EN: One native request worker delegates acquisition to the real VFS service and S-frames replies.
 * RU: Один native request worker делегирует получение настоящему VFS service и выдаёт S-ответы.
 */
#define _POSIX_C_SOURCE 200809L
#include "server_internal.h"


/* EN/RU: Take one complete resource request within original budget; 1 record/0 cancelled/ended/invalid.
 * Server/request/size borrowed, no other RX queue is touched; no retry after consumption.
 * Принять один request под исходным budget, 1/0, не трогая другие очереди и не повторяя приём.
 */
static int request(StyxBidirMovieServer *server, const struct timespec *deadline,
    unsigned char *bytes, size_t *size)
{
    for (;;)
    {
        unsigned remaining = movie_budget(deadline); /* Entire server exchange budget. */
        if (!remaining || dvm_cancel_requested(server->cancel))
        {
            return 0;
        }
        unsigned long tag; /* Original request RX tag, not an outgoing authority. */
        int received = styxbidirroutetypednext(server->route, server->index, &tag, bytes, MOVIE_FRAME, size);
        if (received != STYX_CONTROL_EMPTY)
        {
            return received == STYX_CONTROL_RECORD;
        }
        if (!styxbidirroutetypedwaitcancel(server->route, server->index, remaining, server->cancel))
        {
            return 0;
        }
    }
}


/* EN/RU: Forward the actual dedicated service's checked reply bodies on independently granted TX.
 * Borrow server/open/deadline, no new protocol; returns DONE/REFUSED/FAILED. Native callbacks must
 * honour their own I/O bounds; cancellation/budget checked before/after each operation.
 * Передать checked bodies настоящего service по отдельному TX grant, без нового протокола.
 */
static int response(StyxBidirMovieServer *server, DVM_VfsHandle *opened, const struct timespec *deadline)
{
    unsigned char bytes[MOVIE_FRAME]; /* One complete record from the known native service. */
    FVM_SBindings bindings; /* Private original service binding, never exposed as external TX authority. */
    FVM_SBinding binding; /* Exactly one dedicated service association. */
    fvm_sbindings_init(&bindings, &binding, 1);
    for (;;)
    {
        size_t size = 0; /* Exact bytes returned by the actual bounded service. */
        if (!movie_budget(deadline) || dvm_cancel_requested(server->cancel) ||
            dvm_vfs_read(opened, bytes, sizeof(bytes), &size) != FVM_OK || !size)
        {
            return STYX_MOVIE_SERVER_FAILED;
        }
        FVM_SRecord record; /* Checked complete body, borrowed until the next read. */
        size_t consumed; /* Exact verified record extent. */
        if (fvm_srecord_read(bytes, size, MOVIE_FRAME, &record, &consumed) != FVM_S_OK || consumed != size)
        {
            return STYX_MOVIE_SERVER_FAILED;
        }
        if (!record.tag)
        {
            if (bindings.count || fvm_sbindings_apply(&bindings, bytes, size, &consumed) != FVM_S_OK ||
                !movie_bound(&bindings))
            {
                return STYX_MOVIE_SERVER_FAILED;
            }
            continue;
        }
        if (record.tag != MOVIE_TAG || !movie_bound(&bindings) || !record.size)
        {
            return STYX_MOVIE_SERVER_FAILED;
        }
        unsigned remaining = movie_budget(deadline); /* Includes all earlier native reads and TX waits. */
        if (!remaining || dvm_cancel_requested(server->cancel) ||
            !styxbidirroutesendwait(server->route, server->tag, record.payload, record.size, remaining, server->cancel))
        {
            return STYX_MOVIE_SERVER_FAILED;
        }
        if (record.payload[0] == MOVIE_END)
        {
            if (record.size != 2 || record.payload[1] > 1)
            {
                return STYX_MOVIE_SERVER_FAILED;
            }
            return record.payload[1] ? STYX_MOVIE_SERVER_REFUSED : STYX_MOVIE_SERVER_DONE;
        }
    }
}


/* EN/RU: See server_internal.h; one native scope, one VFS open, one request and final consuming close.
 * См. server_internal.h; один native scope, один VFS open, один request и consuming close.
 */
void *movie_server_run(void *state)
{
    StyxBidirMovieServer *server = state; /* Host pins through final worker join. */
    DVM_CancelScope scope = {0}; /* Native providers observe this independent source. */
    DVM_VfsHandle opened = {0}; /* Exclusive actual VFS resource service open. */
    struct timespec deadline; /* Single deadline for request admission and response. */
    unsigned char bytes[MOVIE_FRAME]; /* Copied request or encoded internal service input. */
    unsigned char encoded[MOVIE_FRAME]; /* Complete private service frame, not raw shared-stream traffic. */
    size_t size; /* Exact request or encoded frame length. */
    int outcome = STYX_MOVIE_SERVER_FAILED; /* Failed unless a terminal reply was copied to TX. */
    if (!dvm_cancel_enter(&scope, server->cancel) || clock_gettime(CLOCK_MONOTONIC, &deadline))
    {
        goto done;
    }
    deadline.tv_sec += server->timeout / 1000;
    deadline.tv_nsec += (long)(server->timeout % 1000) * 1000000L;
    if (deadline.tv_nsec >= 1000000000L)
    {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }
    if (!request(server, &deadline, bytes, &size))
    {
        goto done;
    }
    char reference[MOVIE_PATH]; /* Validated copied request reference, not a host path. */
    char base[MOVIE_PATH]; /* Validated copied base, no normalization at this transport boundary. */
    unsigned purpose; /* Validated explicit existing code/data intent. */
    if (!movie_request_parse(bytes, size, &purpose, reference, base) || dvm_cancel_requested(server->cancel) ||
        !movie_budget(&deadline))
    {
        goto done;
    }
    movie_server_publish(server, STYX_MOVIE_SERVER_WORKING);
    if (dvm_provider_open(server->service, ".", "rw", &opened) != FVM_OK ||
        dvm_cancel_requested(server->cancel) || !movie_budget(&deadline))
    {
        goto done;
    }
    size_t encoded_size; /* Exact internal private frame extent. */
    if (!movie_binding(encoded, &encoded_size) || dvm_vfs_write(&opened, encoded, encoded_size) != FVM_OK ||
        dvm_cancel_requested(server->cancel) || !movie_budget(&deadline) ||
        fvm_srecord_write(MOVIE_TAG, bytes, size, encoded, sizeof(encoded), &encoded_size) != FVM_S_OK ||
        dvm_vfs_write(&opened, encoded, encoded_size) != FVM_OK)
    {
        goto done;
    }
    outcome = response(server, &opened, &deadline);
done:
    dvm_vfs_close(&opened);
    if (scope.cancel)
    {
        dvm_cancel_leave(&scope);
    }
    /* EN/RU: Explicit stop already latched STOPPED before requesting cancellation. Route retirement
     * cancels the same I/O scope but is a failure, never a fabricated host stop or successful END.
     * Явный stop уже фиксирует STOPPED до cancel. Route retirement — FAILED, не host stop/успех.
     */
    movie_server_publish(server, dvm_cancel_requested(server->cancel) ? STYX_MOVIE_SERVER_FAILED : outcome);
    return NULL;
}
