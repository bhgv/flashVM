/* Copyright (c) 2026 bhgv
 * EN: Opt-in deferred byte callbacks with a responsive owner control file.
 * RU: Отложенные байтовые колбэки с доступным управляющим файлом владельца.
 */
#include "styxrpc/serve.h"
#include "drivers/posix/fd.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/* EN: Stable IDs in the fixture namespace.
 * RU: Устойчивые ID пространства имён теста. */
typedef enum NodeId
{
    Root, /* Attach point. */
    Gate, /* Immediate observation and release of callback phases. */
    Data, /* Shared test file, with separate cookies per open. */
    New   /* File published through Tcreate. */
} NodeId;


/* EN: Byte positions of the independent gate status snapshot.
 * RU: Позиции байтов снимка состояния управляющего файла. */
typedef enum Field
{
    ReadEntered,  /* Number of reads that reached the worker. */
    ReadStopped,  /* Number of reads that observed their own stop flag. */
    WriteEntered, /* Number of suffix writes that reached the worker. */
    WriteStopped, /* Number of suffix writes that observed their own stop flag. */
    DataClosed,   /* Count of closed Data cookies. */
    Prefix,       /* Last confirmed written prefix byte. */
    Suffix,       /* Last confirmed written suffix byte. */
    NewByte,      /* Last byte written through the newly created fid. */
    Fields        /* Wire size of the status file. */
} Field;


/* EN: Host context shared by the owner and explicitly selected data workers.
 * RU: Контекст владельца и явно выбранных потоков передачи. */
typedef struct Device
{
    pthread_t owner;              /* Only this thread opens, closes and dispatches. */
    unsigned char status[Fields]; /* Each cell is published and read atomically. */
    int allow_read;               /* Atomic gate that releases new/read workers. */
    int release_stop;             /* Atomic gate allowing a stopped callback to finish. */
    int failed;                   /* Sticky atomic provider contract violation. */
    unsigned open_count;          /* Owner-only cookie accounting. */
    int created;                  /* Owner-only namespace flag for New. */
} Device;


/* EN: One provider cookie owned by its exact open fid through teardown.
 * RU: Одно состояние провайдера, удерживаемое точным fid до закрытия. */
typedef struct Cookie
{
    Device *device; /* Borrowed server context held through final join. */
    uint64_t id;    /* Exact opened node identity. */
    int running;    /* Atomic observation of active worker access. */
} Cookie;

static const StyxRpcEntry entries[] = {
    {Root, ".", 0777, 1},    /* Root directory retains stable identity. */
    {Gate, "gate", 0666, 0}, /* Synchronous control never waits for the worker. */
    {Data, "data", 0666, 0}, /* Deferred read and write example. */
    {New, "new", 0666, 0}    /* Deferred writing also applies to Rcreate fids. */
};


/* EN: Copy metadata for an exact live ID into caller storage.
 * RU: Скопировать метаданные точного живого ID в память вызывающего. */
static int stat_node(void *context, uint64_t id, StyxRpcNode *out)
{
    Device *device = context;
    if (id > New || (id == New && !device->created))
    {
        return 0;
    }
    *out = (StyxRpcNode){.entry = entries[id], .parent = Root, .revision = id == Root ? device->created : 0,
        .length = id == Gate ? Fields : id == Data ? 2 : id == New ? 1 : 0};
    return 1;
}


/* EN: Borrow one entry by index from the root's current directory order.
 * RU: Выдать заимствованную запись по индексу из текущего порядка корня. */
static const StyxRpcEntry *enumerate(void *context, uint64_t parent, size_t index)
{
    Device *device = context;
    return parent == Root && index < 2U + (unsigned)device->created ? &entries[index + 1] : NULL;
}


/* EN: Allocate one cookie for a known file and transfer it to the caller.
 * RU: Выделить состояние известного файла и передать вызывающему. */
static int open_node(void *context, uint64_t id, unsigned access, void **out)
{
    Device *device = context;
    if (!id || id > New || !access || !pthread_equal(device->owner, pthread_self()))
    {
        return 0;
    }
    Cookie *cookie = calloc(1, sizeof(*cookie));
    if (!cookie)
    {
        return 0;
    }
    cookie->device = device;
    cookie->id = id;
    ++device->open_count;
    *out = cookie;
    return 1;
}


/* EN: Retire an owned cookie on the owner after any native callback returns.
 * RU: Освободить своё состояние у владельца после завершения нативного колбэка. */
static void close_node(void *state)
{
    Cookie *cookie = state;
    Device *device = cookie->device;
    if (!pthread_equal(device->owner, pthread_self()) ||
        __atomic_load_n(&cookie->running, __ATOMIC_ACQUIRE))
    {
        __atomic_store_n(&device->failed, 1, __ATOMIC_RELEASE);
    }
    if (cookie->id == Data)
    {
        __atomic_add_fetch(&device->status[DataClosed], 1, __ATOMIC_RELEASE);
    }
    --device->open_count;
    free(cookie);
}


/* EN: Opt only the stock data file and the Tcreate writer into worker execution.
 * RU: Выбрать выполнение в потоках только для файла данных и писателя Tcreate. */
static unsigned deferred_file(void *context, uint64_t id)
{
    Device *device = context;
    if (!pthread_equal(device->owner, pthread_self()))
    {
        __atomic_store_n(&device->failed, 1, __ATOMIC_RELEASE);
    }
    return id == Data ? STYXRPC_READ | STYXRPC_WRITE : id == New ? STYXRPC_WRITE : 0;
}


/* EN: Wait briefly between cooperative stop checks on a native worker.
 * RU: Коротко ждать между проверками отмены в нативном потоке. */
static void pause_worker(void)
{
    const struct timespec interval = {0, 1000000};
    nanosleep(&interval, NULL);
}


/* EN: Publish the exact worker's entry, then wait for allow or its own stop.
 * NULL allow waits only for stop. The final gate keeps the stopped worker alive
 * long enough to check early clunk.
 * RU: Отметить вход точного потока и ждать допуска либо его отмены.
 * При NULL allow ждать только отмены. Последний барьер удерживает отменённый
 * поток для проверки раннего clunk. */
static int wait_worker(Cookie *cookie, int *allow, Field entered, Field stopped)
{
    Device *device = cookie->device;
    __atomic_add_fetch(&device->status[entered], 1, __ATOMIC_RELEASE);
    for (unsigned i = 0; i < 5000; ++i)
    {
        if (styxrpc_server_io_cancelled())
        {
            __atomic_add_fetch(&device->status[stopped], 1, __ATOMIC_RELEASE);
            for (unsigned remaining = 2500; remaining &&
                !__atomic_load_n(&device->release_stop, __ATOMIC_ACQUIRE); --remaining)
            {
                pause_worker();
            }
            return 0;
        }
        if (allow && __atomic_load_n(allow, __ATOMIC_ACQUIRE))
        {
            return 1;
        }
        pause_worker();
    }
    __atomic_store_n(&device->failed, 1, __ATOMIC_RELEASE);
    return 0;
}


/* EN: Copy a status slice synchronously or return one byte from the read worker.
 * RU: Синхронно скопировать срез состояния либо выдать байт из потока чтения. */
static int read_node(void *state, uint64_t offset, void *bytes, size_t size, size_t *used)
{
    Cookie *cookie = state;
    Device *device = cookie->device;
    if (cookie->id == Gate)
    {
        if (!pthread_equal(device->owner, pthread_self()) || styxrpc_server_io_cancelled())
        {
            return 0;
        }
        *used = offset >= Fields ? 0 : Fields - (size_t)offset;
        if (*used > size)
        {
            *used = size;
        }
        for (size_t i = 0; i < *used; ++i)
        {
            ((unsigned char *)bytes)[i] = __atomic_load_n(&device->status[offset + i], __ATOMIC_ACQUIRE);
        }
        return 1;
    }
    if (cookie->id != Data || pthread_equal(device->owner, pthread_self()))
    {
        return 0;
    }
    __atomic_store_n(&cookie->running, 1, __ATOMIC_RELEASE);
    int ready = wait_worker(cookie, &device->allow_read, ReadEntered, ReadStopped);
    if (ready)
    {
        *used = !offset && size ? 1U : 0;
        if (*used)
        {
            *(char *)bytes = 'R';
        }
    }
    __atomic_store_n(&cookie->running, 0, __ATOMIC_RELEASE);
    return ready;
}


/* EN: Apply one owner command from the gate's copied write input.
 * RU: Применить одну команду владельца из скопированных данных gate. */
static int gate_write(Device *device, char command)
{
    switch (command)
    {
    case 'R':
        __atomic_store_n(&device->allow_read, 1, __ATOMIC_RELEASE);
        return 1;
    case 'P':
        __atomic_store_n(&device->allow_read, 0, __ATOMIC_RELEASE);
        return 1;
    case 'D':
        __atomic_store_n(&device->release_stop, 1, __ATOMIC_RELEASE);
        return 1;
    default:
        return 0;
    }
}


/* EN: Confirm one prefix byte and park the suffix, keeping the copied byte
 * through cancellation. A freshly created fid has an independent test result.
 * RU: Подтвердить один байт префикса и ждать на остатке, сохраняя
 * скопированный байт при отмене. Новый fid выдаёт отдельный результат. */
static int write_node(void *state, uint64_t offset, const void *bytes, size_t size, size_t *used)
{
    Cookie *cookie = state;
    Device *device = cookie->device;
    if (!size)
    {
        return 0;
    }
    if (cookie->id == Gate)
    {
        if (!pthread_equal(device->owner, pthread_self()) || offset || size != 1 ||
            !gate_write(device, *(const char *)bytes))
        {
            return 0;
        }
        *used = 1;
        return 1;
    }
    if (pthread_equal(device->owner, pthread_self()) || offset > 1 || (cookie->id == New && offset))
    {
        return 0;
    }
    __atomic_store_n(&cookie->running, 1, __ATOMIC_RELEASE);
    int ready = !offset || wait_worker(cookie, NULL, WriteEntered, WriteStopped);
    if (offset && *(const char *)bytes != 'B')
    {
        __atomic_store_n(&device->failed, 1, __ATOMIC_RELEASE);
        ready = 0;
    }
    if (ready)
    {
        Field field = cookie->id == New ? NewByte : offset ? Suffix : Prefix;
        __atomic_store_n(&device->status[field], *(const unsigned char *)bytes, __ATOMIC_RELEASE);
        *used = 1;
    }
    __atomic_store_n(&cookie->running, 0, __ATOMIC_RELEASE);
    return ready;
}


/* EN: Publish one new file together with its owned already-open writer cookie.
 * RU: Опубликовать один файл вместе с собственным уже открытым состоянием записи. */
static int create_file(void *context, uint64_t parent, const char *name, int directory,
    uint64_t *id, uint32_t *revision, void **cookie)
{
    Device *device = context;
    if (parent != Root || directory || device->created || strcmp(name, "new") ||
        !open_node(context, New, STYXRPC_WRITE, cookie))
    {
        return 0;
    }
    device->created = 1;
    *id = New;
    *revision = 0;
    return 1;
}


/* EN: Serve one owned local socket and check teardown retires all cookies.
 * RU: Обслужить один собственный сокет и проверить освобождение всех состояний. */
int main(int argc, char **argv)
{
    if (argc != 2)
    {
        return 1;
    }
    static const StyxRpcTree tree = {.files = {enumerate, open_node, read_node, write_node, close_node},
        .stat = stat_node}; /* Immutable callback tree throughout the session. */
    static const StyxRpcMutations mutations = {.create = create_file}; /* Only the fixture file is creatable. */
    Device device = {.owner = pthread_self()};
    device.status[Suffix] = '_';
    StyxRpcServer *server = styxrpc_server_create(phyposixfd(atoi(argv[1]), PHY_STREAM | PHY_ORDERED | PHY_RELIABLE),
        &tree, &device);
    if (!styxrpc_server_enable_deferred_io(server, deferred_file) ||
        styxrpc_server_enable_deferred_io(server, deferred_file) ||
        !styxrpc_server_enable_mutations(server, &mutations))
    {
        styxrpc_server_destroy(server);
        return 1;
    }
    while (styxrpc_server_step(server))
    {
    }
    styxrpc_server_destroy(server);
    if (device.open_count || __atomic_load_n(&device.failed, __ATOMIC_ACQUIRE) || styxrpc_server_io_cancelled())
    {
        fprintf(stderr, "deferred device did not retire its workers and cookies cleanly\n");
        return 1;
    }
    printf("deferred device retired %u reads and %u writes by cancellation\n",
        device.status[ReadStopped], device.status[WriteStopped]);
    return 0;
}
