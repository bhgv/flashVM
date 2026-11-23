/* Copyright (c) 2026 bhgv
 * EN: A C-only device publishes one callback function on a delegated Styx stream.
 * RU: Устройство на C публикует функцию-колбэк в выданном потоке Styx.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "server.h"
#include "drivers/posix/fd.h"
#include "styxrpc/rpc.h"
#include "adjust.h"
#include "styxrpc/vfs.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define MOUNTED 1UL /* Directory of published modules. */
#define DEVICE 2UL /* Read/execute module directory. */
#define PROFILE 3UL /* Compiler-compatible interface. */
#define ADJUST 6UL  /* Managed function with per-fid record buffers. */
#define VALUE 5UL /* Byte file backed by a device register. */
#define SAMPLE 4UL /* An opened function has private request and reply state. */

static Styxserver server; /* Exactly one native dispatcher owns this example. */
static unsigned char profile[256]; /* Immutable interface bytes after startup. */
static size_t profile_size; /* Confirmed encoded extent. */


/* EN: Increment an int sensor sample. The reply encoder checks its range.
 * RU: Увеличить показание int. Диапазон проверяет кодек ответа. */
static int sample(void *unused, int64_t argument, int64_t *result)
{
    (void)unused;
    *result = argument + 1;
    return 1;
}

static const StyxRpcExport exported = {"sample", STYXRPC_INT_SIGNATURE, sample, NULL};


static unsigned char device_register = 'A'; /* Shared across open fids. */


/* EN: Return the sole child on index zero under DEVICE; ignore context.
 * RU: Вернуть единственного потомка DEVICE по индексу ноль; context не используется. */
static const StyxRpcEntry *enumerate_value(void *context, uint64_t directory, size_t index)
{
    (void)context;
    static const StyxRpcEntry entry = {VALUE, "value", 0666, 0};
    return directory == DEVICE && index == 0 ? &entry : NULL;
}


/* EN: Borrow context as the cookie for VALUE. The dispatcher applies access mode.
 * RU: Использовать context как состояние VALUE. Режим доступа проверяет диспетчер. */
static int open_value(void *context, uint64_t node, unsigned access, void **cookie)
{
    (void)access;
    *cookie = context;
    return node == VALUE;
}


/* EN: Read one byte from the cookie at offset zero into a buffer with room.
 * RU: Прочитать байт из cookie по нулевому смещению, если в буфере есть место. */
static int read_value(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    *size = !offset && capacity ? 1 : 0;
    if (*size)
    {
        *(unsigned char *)bytes = *(unsigned char *)cookie;
    }
    return 1;
}


/* EN: Replace the register at offset zero when the caller supplies a byte.
 * RU: Обновить регистр по нулевому смещению, если передан хотя бы один байт. */
static int write_value(void *cookie, uint64_t offset, const void *bytes, size_t count, size_t *size)
{
    *size = !offset && count ? 1 : 0;
    if (*size)
    {
        *(unsigned char *)cookie = *(const unsigned char *)bytes;
    }
    return 1;
}

static const StyxRpcVfs device_files = {enumerate_value, open_value, read_value, write_value, NULL};

/* EN: Release the open file cookie on clunk or stream loss.
 * RU: Освободить состояние открытого файла при clunk или потере соединения. */
static void close_value(void *file)
{
    styxrpc_file_close(file);
    free(file);
}

typedef struct AdjustFid
{
    StyxRpcManagedCall call;    /* Exact this fid's checked request and reply. */
    unsigned char request[1024]; /* Whole staged request. */
    unsigned char reply[1024];   /* Immutable reply. */
} AdjustFid;


/* EN: Release the owned managed fid state, cancelling any pending work.
 * RU: Освободить состояние fid, отменив ожидающую работу. */
static void close_adjust(void *state)
{
    AdjustFid *fid = state;
    styxrpc_managed_close(&fid->call);
    free(fid);
}


/* EN: Attach a new read-write call to the server's current Topen.
 * RU: Закрепить вызов чтения и записи на текущем Topen. */
static char *open_adjust(int mode)
{
    if (mode != ORDWR)
    {
        return Eperm;
    }
    AdjustFid *fid = calloc(1, sizeof(*fid));
    if (!fid)
    {
        return Enomem;
    }
    if (styxrpc_managed_open(&fid->call, &adjust_export, fid->request, sizeof(fid->request),
        fid->reply, sizeof(fid->reply)) != STYXRPC_OK || styxsetauxowned(&server, fid, close_adjust) < 0)
    {
        close_adjust(fid);
        return Eperm;
    }
    return nil;
}


/* EN: Route the Qid once at Topen and pin a new call on the exact wire fid.
 * RU: Выбрать узел по Qid в Topen и удержать вызов на точном fid. */
static char *open_file(Qid *qid, int mode)
{
    if (qid->path == ADJUST)
    {
        return open_adjust(mode);
    }
    if (qid->path == VALUE)
    {
        if (mode != OREAD && mode != OWRITE && mode != ORDWR)
        {
            return Eperm;
        }
        unsigned access = mode == OREAD ? STYXRPC_READ :
            mode == OWRITE ? STYXRPC_WRITE : STYXRPC_READ | STYXRPC_WRITE;
        StyxRpcFile *file = calloc(1, sizeof(*file));
        if (!file)
        {
            return Enomem;
        }
        if (styxrpc_file_open(file, &device_files, &device_register, VALUE, access) != STYXRPC_OK ||
            styxsetauxowned(&server, file, close_value) < 0)
        {
            close_value(file);
            return Eperm;
        }
        return nil;
    }
    if (qid->path != SAMPLE)
    {
        return mode == OREAD ? nil : Eperm;
    }
    if (mode != ORDWR)
    {
        return Eperm;
    }
    StyxRpcCall *call = malloc(sizeof(*call));
    if (!call)
    {
        return Enomem;
    }
    if (styxrpc_open(call, &exported) != STYXRPC_OK || styxsetauxowned(&server, call, free) < 0)
    {
        free(call);
        return Eperm;
    }
    return nil;
}


/* EN: Read static profile data or the immutable reply on the current open fid.
 * RU: Прочитать статические метаданные или неизменяемый ответ текущего fid. */
static char *read_file(Qid qid, char *bytes, ulong *count, vlong offset)
{
    if (offset < 0 || (uvlong)offset > SIZE_MAX)
    {
        return Ebadarg;
    }
    if (qid.path == PROFILE)
    {
        size_t available = (size_t)offset < profile_size ? profile_size - (size_t)offset : 0;
        if (*count > available)
        {
            *count = available;
        }
        if (*count)
        {
            memcpy(bytes, profile + offset, *count);
        }
        return nil;
    }
    size_t size;
    if (qid.path == VALUE)
    {
        if (styxrpc_file_read(styxgetaux(&server), offset, bytes, *count, &size) != STYXRPC_OK)
        {
            return Ebadarg;
        }
        *count = size;
        return nil;
    }
    if (qid.path == ADJUST)
    {
        AdjustFid *fid = styxgetaux(&server);
        StyxRpcStatus status = styxrpc_managed_read(&fid->call, offset, bytes, *count, &size);
        if (status == STYXRPC_INVALID)
        {
            return Ebadarg;
        }
        *count = size;
        return nil;
    }
    if (qid.path != SAMPLE ||
        styxrpc_read(styxgetaux(&server), (size_t)offset, bytes, *count, &size) == STYXRPC_INVALID)
    {
        return Ebadarg;
    }
    *count = size;
    return nil;
}


/* EN: Pass consecutive request bytes to the current fid, committing once.
 * RU: Передать последовательные байты запроса текущему fid с одним исполнением. */
static char *write_file(Qid qid, char *bytes, ulong *count, vlong offset)
{
    if (offset < 0 || (uvlong)offset > SIZE_MAX)
    {
        return Ebadarg;
    }
    if (qid.path == VALUE)
    {
        size_t size;
        if (styxrpc_file_write(styxgetaux(&server), offset, bytes, *count, &size) != STYXRPC_OK)
        {
            return Ebadarg;
        }
        *count = size;
        return nil;
    }
    if (qid.path == ADJUST)
    {
        AdjustFid *fid = styxgetaux(&server);
        unsigned char scratch[1024];
        return styxrpc_managed_write(&fid->call, offset, bytes, *count, scratch, sizeof(scratch)) ==
            STYXRPC_INVALID ? Ebadarg : nil;
    }
    return qid.path == SAMPLE &&
        styxrpc_write(styxgetaux(&server), (size_t)offset, bytes, *count) != STYXRPC_INVALID ?
        nil : Ebadarg;
}


/* EN: Consume the supplied connected fd and serve until the peer disconnects.
 * RU: Принять выданный подключённый fd и обслуживать до закрытия соединения. */
int main(int argc, char **argv)
{
    char *end;
    long fd = argc == 2 ? strtol(argv[1], &end, 10) : -1;
    if (fd < 3 || fd > INT_MAX || *end ||
        styxrpc_profile(&exported, 1, profile, sizeof(profile), &profile_size) != STYXRPC_OK)
    {
        return 2;
    }
    size_t typed_size;
    if (styxrpc_managed_profile(&adjust_export, profile + profile_size, sizeof(profile) - profile_size,
        &typed_size) != STYXRPC_OK)
    {
        return 2;
    }
    profile_size += typed_size;
    static Styxops ops = {.open = open_file, .read = read_file, .write = write_file};
    RemotePhy *phy = phyposixfd((int)fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE);
    if (styxphyinit(&server, &ops, phy, 0555, 0, 8))
    {
        return 1;
    }
    Styxfile *iface = NULL;
    int published = styxadddir(&server, Qroot, MOUNTED, "mounted", 0555, "device") &&
        styxadddir(&server, MOUNTED, DEVICE, "device.dis", 0555, "device") &&
        (iface = styxaddfile(&server, DEVICE, PROFILE, ".interface", 0444, "device")) &&
        styxaddfile(&server, DEVICE, SAMPLE, "sample", 0777, "device") &&
        styxaddfile(&server, DEVICE, ADJUST, "adjust", 0777, "device");
    for (size_t index = 0; published; ++index)
    {
        StyxRpcEntry entry;
        StyxRpcStatus status = styxrpc_dir_entry(&device_files, NULL, DEVICE, index, &entry);
        if (status == STYXRPC_MORE)
        {
            break;
        }
        published = status == STYXRPC_OK && !entry.directory &&
            styxaddfile(&server, DEVICE, entry.id, (char *)entry.name, entry.mode, "device");
    }
    if (published)
    {
        iface->d.length = profile_size;
        while (!styxwait(&server) && !styxprocess(&server))
        {
        }
    }
    styxend(&server);
    return published ? 0 : 1;
}
