/* Copyright (c) 2026 bhgv
 * EN: Own partial worker initialization, startup wake pipe and join-before-close.
 * RU: Владение частичной инициализацией workers, wake pipe и join-before-close.
 * standalone-reviewed: resource stages are explicit; no guest scheduler is involved.
 */
#include "bridge.h"
#include "flashvm/sbindings.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>


/* EN/RU: See bridge.h; fd ownership transfers before any initialization attempt.
 * См. bridge.h; владение fd передаётся до любой попытки инициализации.
 */
int bridge_worker_start(Worker *worker, int fd, unsigned long tag, int notify)
{
    memset(worker, 0, sizeof(*worker)); /* No uninitialized queue links or terminal flags reach the worker. */
    worker->socket = fd;
    worker->tag = tag;
    worker->notify = notify;
    int error = pthread_mutex_init(&worker->lock, NULL); /* First acquired resource after the consumed socket. */
    if (error)
    {
        fprintf(stderr, "client %lu mutex: %s\n", tag, strerror(error));
        close(fd);
        return 0;
    }
    error = pthread_cond_init(&worker->ready, NULL);
    if (error)
    {
        fprintf(stderr, "client %lu condition: %s\n", tag, strerror(error));
        pthread_mutex_destroy(&worker->lock); /* No thread exists and no queue state is shared yet. */
        close(fd);
        return 0;
    }
    error = pthread_create(&worker->thread, NULL, bridge_worker, worker);
    if (error)
    {
        fprintf(stderr, "client %lu thread: %s\n", tag, strerror(error));
        pthread_cond_destroy(&worker->ready);
        pthread_mutex_destroy(&worker->lock);
        close(fd); /* Every resource acquired for this failed start has retired. */
        return 0;
    }
    return 1; /* Main must retain Worker, socket and notify until join. */
}


/* EN/RU: See bridge.h; shutdown wakes native socket I/O without closing/reusing its fd.
 * См. bridge.h; shutdown будит native socket I/O без закрытия/переиспользования fd.
 */
void bridge_worker_stop(Worker *worker, int aborting)
{
    pthread_mutex_lock(&worker->lock);
    worker->ending = 1; /* Reject new queue admission; ordinary EOF still permits drain. */
    if (aborting)
    {
        worker->aborting = 1;
        worker->failed = 1;
    }
    pthread_cond_signal(&worker->ready);
    pthread_mutex_unlock(&worker->lock);
    if (aborting)
    {
        shutdown(worker->socket, SHUT_RDWR); /* An in-flight backend effect may already have happened. Never replay it. */
    }
}


/* EN/RU: See bridge.h; process-fatal join failure cannot justify freeing borrowed state.
 * См. bridge.h; фатальная ошибка join не разрешает освобождать заимствованное состояние.
 */
int bridge_worker_finish(Worker *worker)
{
    int error = pthread_join(worker->thread, NULL);
    if (error)
    {
        fprintf(stderr, "client %lu join: %s\n", worker->tag, strerror(error));
        _Exit(2); /* Do not unwind main's Worker array while an unretired thread might still use it. */
    }
    close(worker->socket); /* Only now may this descriptor number be reused by the process. */
    while (worker->head)
    {
        Packet *packet = worker->head;
        worker->head = packet->next;
        free(packet); /* Output loss may leave admitted but unexecuted requests in the FIFO. */
    }
    worker->tail = NULL;
    int valid = !worker->output_failed; /* Successful join publishes the worker's terminal result. */
    error = pthread_cond_destroy(&worker->ready);
    int mutex_error = pthread_mutex_destroy(&worker->lock);
    if (error || mutex_error)
    {
        fprintf(stderr, "client %lu synchronization cleanup failed\n", worker->tag);
        valid = 0;
    }
    return valid;
}


/* EN: Create owned descriptors[2]; mark only this private pipe nonblocking.
 * Return 1/0, close both on partial failure and preserve errno for the diagnostic.
 * RU: Создать owned descriptors[2]; сделать nonblocking только этот private pipe.
 * Вернуть 1/0; при частичном отказе закрыть оба и сохранить errno.
 */
static int wake_pipe(int *descriptors)
{
    if (pipe(descriptors))
    {
        return 0;
    }
    for (unsigned index = 0; index < 2; index++)
    {
        int flags = fcntl(descriptors[index], F_GETFL);
        if (flags < 0 || fcntl(descriptors[index], F_SETFL, flags | O_NONBLOCK) < 0)
        {
            int error = errno;
            close(descriptors[0]);
            close(descriptors[1]);
            errno = error;
            return 0;
        }
    }
    return 1; /* Never change the open-file-description flags of borrowed stdin/stdout. */
}


/* EN/RU: See bridge.h; metadata is a service-owned assignment, not client data.
 * См. bridge.h; metadata — служебное назначение, не данные клиента.
 */
int bridge_run(const unsigned *ports)
{
    int wake[2]; /* Main owns both ends until every worker that borrows the writer has joined. */
    if (!wake_pipe(wake))
    {
        perror("gateway wake pipe");
        return 2;
    }
    Worker workers[BRIDGE_CLIENTS];
    unsigned started = 0; /* Only these entries own initialized synchronization/thread resources. */
    int valid = 1;
    for (unsigned index = 0; index < BRIDGE_CLIENTS; index++)
    {
        int fd = bridge_connect(ports[index]); /* Each client owns its own 9P connection, even with equal ports. */
        if (fd < 0)
        {
            perror("backend connect");
            valid = 0;
            break;
        }
        if (!bridge_worker_start(&workers[index], fd, index + 7, wake[1]))
        {
            valid = 0; /* Failed start already consumed fd and all its partial resources. */
            break;
        }
        started++;
    }
    FVM_SBinding assignments[2] = {{7, 1, 13, "example.9p.v1"}, {8, 1, 13, "example.9p.v1"}};
    unsigned char metadata[256];
    size_t size;
    if (valid)
    {
        valid = fvm_sbindings_payload_write(assignments, 2, metadata, sizeof(metadata), &size) == FVM_S_OK &&
            bridge_record(0, metadata, size); /* Startup must publish a complete service record before application traffic. */
    }
    int received = valid ? bridge_receive(workers, wake[0]) : 0;
    valid = valid && received == 1;
    int aborting = received < 0;
    for (unsigned index = 0; index < started; index++)
    {
        pthread_mutex_lock(&workers[index].lock);
        aborting |= workers[index].output_failed; /* An admission failure can race with the wake byte; inspect sticky state too. */
        pthread_mutex_unlock(&workers[index].lock);
    }
    for (unsigned index = 0; index < started; index++)
    {
        bridge_worker_stop(&workers[index], aborting); /* Output failure aborts; complete EOF/input failure drains admitted work. */
    }
    for (unsigned index = 0; index < started; index++)
    {
        if (!bridge_worker_finish(&workers[index]))
        {
            valid = 0;
            for (unsigned remaining = index + 1; remaining < started; remaining++)
            {
                bridge_worker_stop(&workers[remaining], 1); /* Do not start further queued effects after observed output loss. */
            }
        }
    }
    close(wake[0]);
    close(wake[1]); /* No worker retains this descriptor after the joins. */
    return valid ? 0 : 2;
}
