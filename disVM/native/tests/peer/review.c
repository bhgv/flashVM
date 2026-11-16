/* Copyright (c) 2026 bhgv
 * EN: Dedicated native test reviewer, condition notification and nonblocking pipe.
 * RU: Выделенный native reviewer теста, condition notification и неблокирующий pipe.
 */
#include "review.h"
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; /* Protect worker control. */
static pthread_cond_t condition = PTHREAD_COND_INITIALIZER; /* Native work notification. */
static pthread_t worker; /* Native reviewer, never a guest execution thread. */
static int active; /* Serialized host lifecycle flag. */
static int queued; /* Work queued under lock. */
static int permitted; /* Explicit test-owner gate under lock. */
static int stopping; /* Joined shutdown request under lock. */
static int pipes[2] = {-1, -1}; /* Owned until socket owner ends. */
static DVM_Publication *publisher; /* Borrowed until joined stop. */
static DVM_Provider *source; /* Retained review grant until end. */


/* EN/RU: Emit one readiness byte; EAGAIN means an earlier byte already wakes owner.
 * Выдать readiness byte; EAGAIN означает уже ожидающее уведомление. */
void reviewwake(void)
{
    char byte = 1;
    if (pipes[1] >= 0)
    {
        ssize_t n;
        do
        {
            n = write(pipes[1], &byte, 1);
        } while (n < 0 && errno == EINTR);
        if (n != 1 && !(n < 0 && errno == EAGAIN))
        {
            abort();
        }
    }
}


/* EN/RU: Review native requests only after test owner permission; wake socket owner.
 * Рассматривать native requests после разрешения теста; разбудить socket owner. */
static void *run(void *argument)
{
    (void)argument;
    pthread_mutex_lock(&lock);
    while (!stopping)
    {
        while (!stopping && !(queued && permitted))
        {
            pthread_cond_wait(&condition, &lock);
        }
        if (stopping)
        {
            break;
        }
        queued = permitted = 0;
        pthread_mutex_unlock(&lock);
        DVM_ExportRequest request;
        while (dvm_publication_next(publisher, &request))
        {
            dvm_publication_reply(publisher, request.id, source);
        }
        reviewwake();
        pthread_mutex_lock(&lock);
    }
    pthread_mutex_unlock(&lock);
    return NULL;
}


/* EN/RU: See review.h; fixture fails explicitly if native setup cannot be created.
 * См. review.h; явный отказ теста при невозможности native setup. */
int reviewstart(DVM_Publication *owner, DVM_Provider *chosen)
{
    if (active || !owner || !chosen || pipe(pipes) < 0)
    {
        return -1;
    }
    if (fcntl(pipes[0], F_SETFL, O_NONBLOCK) < 0 || fcntl(pipes[1], F_SETFL, O_NONBLOCK) < 0 ||
        !dvm_provider_retain(chosen))
    {
        close(pipes[0]);
        close(pipes[1]);
        pipes[0] = pipes[1] = -1;
        return -1;
    }
    publisher = owner;
    source = chosen;
    queued = permitted = stopping = 0;
    if (pthread_create(&worker, NULL, run, NULL))
    {
        reviewend();
        return -1;
    }
    active = 1;
    return 0;
}


/* EN/RU: See review.h; signal copied publication work under the native mutex.
 * См. review.h; сигнализировать publication work под native mutex. */
void reviewnotify(void *context)
{
    (void)context;
    pthread_mutex_lock(&lock);
    queued = 1;
    pthread_cond_signal(&condition);
    pthread_mutex_unlock(&lock);
}


/* EN/RU: See review.h; grant worker permission without executing owner work inline.
 * См. review.h; разрешить worker работу без выполнения inline. */
void reviewpermit(void)
{
    pthread_mutex_lock(&lock);
    permitted = 1;
    pthread_cond_signal(&condition);
    pthread_mutex_unlock(&lock);
}


/* EN/RU: See review.h; descriptor stays valid across joined worker stop.
 * См. review.h; fd остаётся валидным после joined worker stop. */
int reviewfd(void)
{
    return pipes[0];
}


/* EN/RU: See review.h; pipe reads cannot block the socket owner.
 * См. review.h; чтение pipe не блокирует socket-owner. */
int reviewdrain(void)
{
    char bytes[64];
    int notified = 0;
    if (pipes[0] >= 0)
    {
        ssize_t n;
        do
        {
            n = read(pipes[0], bytes, sizeof(bytes));
            if (n > 0)
            {
                notified = 1;
            }
        } while (n > 0 || (n < 0 && errno == EINTR));
    }
    return notified;
}


/* EN/RU: See review.h; join outside lock, no callback context may retire earlier.
 * См. review.h; join вне lock, callback context нельзя освобождать раньше. */
void reviewstop(void)
{
    if (active)
    {
        pthread_mutex_lock(&lock);
        stopping = 1;
        pthread_cond_signal(&condition);
        pthread_mutex_unlock(&lock);
        pthread_join(worker, NULL);
        active = 0;
    }
}


/* EN/RU: See review.h; pipe is consumed only after readiness unregistration/end.
 * См. review.h; pipe поглощается только после unregister/end readiness. */
void reviewend(void)
{
    reviewstop();
    if (pipes[0] >= 0)
    {
        close(pipes[0]);
        close(pipes[1]);
        pipes[0] = pipes[1] = -1;
    }
    dvm_provider_release(source);
    source = NULL;
    publisher = NULL;
}
