/* Copyright (c) 2026 bhgv
 * EN: Own pending launch records and per-fid completion across native workers.
 * RU: Хранить запросы запуска и результаты fid между нативными потоками.
 */

#define LAUNCH_PENDING_LIMIT 64U
#define LAUNCH_WIRE_LIMIT 64U

struct StyxLaunchTicket
{
    StyxLaunchTicket *next; /* Protected by the queue lock until dequeued. */
    unsigned owners; /* Queue/worker and fid/legacy reader each hold one reference. */
    int abandoned; /* The read/write fid closed before the worker took the launch. */
    int status; /* Atomic: pending 0, success 1, error 2. */
    size_t size;
    unsigned char wire[LAUNCH_WIRE_LIMIT];
};

struct StyxLaunchQueue
{
    pthread_mutex_t lock;
    StyxLaunchTicket *first;
    StyxLaunchTicket *last;
    StyxLaunchTicket *legacy; /* One compatibility result for the write-only endpoint. */
    unsigned count;
    unsigned long tag;
    int sealed;
};

static StyxLaunchQueue *launch_queue(unsigned long tag)
{
    StyxLaunchQueue *queue = calloc(1, sizeof(*queue));
    if (!queue || pthread_mutex_init(&queue->lock, NULL))
    {
        free(queue);
        return NULL;
    }
    queue->tag = tag;
    return queue;
}

static void drop_launch(StyxLaunchTicket *ticket)
{
    if (ticket && __atomic_sub_fetch(&ticket->owners, 1, __ATOMIC_ACQ_REL) == 0)
    {
        free(ticket);
    }
}

static void close_launch_ticket(StyxLaunchTicket *ticket)
{
    if (ticket)
    {
        __atomic_store_n(&ticket->abandoned, 1, __ATOMIC_RELEASE);
        drop_launch(ticket);
    }
}

static void seal_launch_queue(StyxLaunchQueue *queue)
{
    if (!queue)
    {
        return;
    }
    pthread_mutex_lock(&queue->lock);
    queue->sealed = 1;
    StyxLaunchTicket *ticket = queue->first;
    queue->first = queue->last = NULL;
    queue->count = 0;
    pthread_mutex_unlock(&queue->lock);
    while (ticket)
    {
        StyxLaunchTicket *next = ticket->next;
        drop_launch(ticket);
        ticket = next;
    }
}

static void free_launch_queue(StyxLaunchQueue *queue)
{
    if (queue)
    {
        seal_launch_queue(queue);
        drop_launch(queue->legacy);
        pthread_mutex_destroy(&queue->lock);
        free(queue);
    }
}

static int enqueue_launch(StyxLaunchQueue *queue, const void *bytes, size_t size,
    int waiting, StyxLaunchTicket **fid)
{
    FVM_SRecord record;
    DVM_LaunchProgram program;
    size_t used;
    if (size > LAUNCH_WIRE_LIMIT ||
        fvm_srecord_read(bytes, size, LAUNCH_WIRE_LIMIT, &record, &used) != FVM_S_OK ||
        used != size || record.tag != queue->tag ||
        !styxlaunchprogramdecode(record.payload, record.size, &program))
    {
        return 0;
    }
    StyxLaunchTicket *ticket = calloc(1, sizeof(*ticket));
    if (!ticket)
    {
        return 0;
    }
    ticket->owners = 2;
    ticket->size = size;
    memcpy(ticket->wire, bytes, size);
    pthread_mutex_lock(&queue->lock);
    int accepted = !queue->sealed && queue->count < LAUNCH_PENDING_LIMIT && (waiting || !queue->legacy);
    if (accepted)
    {
        if (queue->last)
        {
            queue->last->next = ticket;
        }
        else
        {
            queue->first = ticket;
        }
        queue->last = ticket;
        queue->count++;
        if (waiting)
        {
            *fid = ticket;
        }
        else
        {
            queue->legacy = ticket;
        }
    }
    pthread_mutex_unlock(&queue->lock);
    if (!accepted)
    {
        free(ticket);
    }
    return accepted;
}


/* EN/RU: See unibus_server.h. / См. unibus_server.h. */
StyxLaunchTicket *styxunibusservernextlaunch(StyxUnibusServer *server, void *wire, size_t capacity, size_t *size)
{
    StyxLaunchQueue *queue = server ? server->launches : NULL;
    if (!queue || !wire || capacity < LAUNCH_WIRE_LIMIT || !size)
    {
        return NULL;
    }
    const struct timespec pause = {0, 10000000};
    while (!dvm_cancel_requested(dvm_cancel_current()))
    {
        pthread_mutex_lock(&queue->lock);
        int sealed = queue->sealed;
        StyxLaunchTicket *ticket = queue->first;
        if (ticket)
        {
            queue->first = ticket->next;
            if (!queue->first)
            {
                queue->last = NULL;
            }
            queue->count--;
        }
        pthread_mutex_unlock(&queue->lock);
        if (ticket)
        {
            if (__atomic_load_n(&ticket->abandoned, __ATOMIC_ACQUIRE))
            {
                drop_launch(ticket);
                continue;
            }
            memcpy(wire, ticket->wire, ticket->size);
            *size = ticket->size;
            return ticket;
        }
        if (sealed)
        {
            break;
        }
        nanosleep(&pause, NULL);
    }
    return NULL;
}


/* EN/RU: See unibus_server.h. / См. unibus_server.h. */
void styxunibuslaunchcomplete(StyxLaunchTicket *ticket, int status)
{
    __atomic_store_n(&ticket->status, status ? 2 : 1, __ATOMIC_RELEASE);
    drop_launch(ticket);
}

static size_t launch_ticket_read(const StyxLaunchTicket *ticket, char *bytes, size_t count, uint64_t offset)
{
    int value = ticket ? __atomic_load_n(&ticket->status, __ATOMIC_ACQUIRE) : 0;
    const char *status = value == 1 ? "ok\n" : value == 2 ? "error\n" : "";
    size_t length = strlen(status);
    if (offset >= length)
    {
        return 0;
    }
    length -= (size_t)offset;
    count = count < length ? count : length;
    memcpy(bytes, status + offset, count);
    return count;
}
