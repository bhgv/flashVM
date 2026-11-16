/* Copyright (c) 2026 bhgv
 * EN: Real native mount-provider acceptance over clipped POSIX stream callbacks.
 * RU: Настоящий mount-provider acceptance через clipped POSIX stream callbacks.
 */
#define _POSIX_C_SOURCE 200809L
#include "mount.h"
#include "notice.h"
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>

typedef struct Stream
{
    int fd; /* Owned native test connection. */
    int closed; /* Exactly-once close oracle. */
    double deadline; /* One monotonic total deadline per RPC, never extended per fragment. */
    unsigned budget; /* Explicit native test deadline in milliseconds. */
    unsigned calls; /* Send callback count, serialized by native client. */
} Stream;
static unsigned lost; /* Native notice count, only the test owner publishes failures. */


/* EN/RU: Keep checks active in all build modes.
 * Сохранить проверки во всех build modes. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "mount client: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Read monotonic milliseconds; failure is terminal for the native test.
 * Прочитать monotonic milliseconds; отказ завершает native test. */
static double now(void)
{
    struct timespec value;
    check(clock_gettime(CLOCK_MONOTONIC, &value) == 0, "monotonic clock");
    return value.tv_sec*1000.0+value.tv_nsec/1000000.0;
}


/* EN/RU: Establish the total RPC deadline once before its first outgoing byte.
 * Установить total RPC deadline до первого исходящего byte. */
static int begin(void *state)
{
    Stream *stream = state;
    stream->deadline = now()+stream->budget;
    return 1;
}


/* EN/RU: Every partial callback shares remaining total time, not a fresh timeout.
 * Каждый partial callback использует оставшееся общее время, не новый timeout. */
static int remaining(Stream *stream)
{
    double milliseconds = stream->deadline-now();
    if (milliseconds <= 0)
    {
        return 0;
    }
    unsigned long left = (unsigned long)milliseconds+1;
    struct timeval timeout = {left/1000, (left%1000)*1000};
    return !setsockopt(stream->fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) &&
        !setsockopt(stream->fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
}


/* EN/RU: Force exact-reader continuation independently of TCP packet boundaries.
 * Принудительно проверить exact-reader continuation независимо от TCP packets. */
static long receive(void *state, void *bytes, size_t n)
{
    Stream *stream = state;
    long count;
    do
    {
        if (!remaining(stream))
        {
            return -1;
        }
        count = recv(stream->fd, bytes, n > 2 ? 2 : n, 0);
    } while (count < 0 && errno == EINTR);
    return count;
}


/* EN/RU: Force short sends; no lost acknowledgement can trigger a replay.
 * Принудительные short sends; потеря подтверждения не вызывает replay. */
static long sendbytes(void *state, const void *bytes, size_t n)
{
    Stream *stream = state;
    stream->calls++;
    long count;
    do
    {
        if (!remaining(stream))
        {
            return -1;
        }
        count = send(stream->fd, bytes, n > 3 ? 3 : n, MSG_NOSIGNAL);
    } while (count < 0 && errno == EINTR);
    return count;
}


/* EN/RU: Consume transport once; diagnostic counters remain borrowed by main.
 * Поглотить transport один раз; diagnostic counters остаются у main. */
static void closestream(void *state)
{
    Stream *stream = state;
    check(!stream->closed, "duplicate transport close");
    stream->closed++;
    close(stream->fd);
}


/* EN/RU: Copy only the native event; never execute VM, perform I/O or query provider.
 * Скопировать только native event, без VM/I/O/provider query. */
static void notice(void *state, const DVM_ChangeEvent *event)
{
    (void)state;
    check(event->kinds == (DVM_CHANGE_CONNECTION|DVM_CHANGE_NETWORK), "loss event class");
    lost++;
}


/* EN/RU: Open one actual provider handle, retaining its factory lifetime.
 * Открыть provider handle, удерживающий lifetime factory. */
static DVM_VfsHandle openfile(DVM_Provider *provider, const char *name, const char *mode)
{
    DVM_VfsHandle out = {0};
    check(dvm_provider_open(provider, name, mode, &out) == FVM_OK, name);
    return out;
}


/* EN/RU: Seek an independent retained object and verify confirmed read bytes.
 * Seek независимого объекта и проверка подтверждённых read bytes. */
static void contents(DVM_VfsHandle *file, const char *expected)
{
    char bytes[32];
    long position;
    check(dvm_vfs_seek(file, 0, FVM_FILE_SEEK_START, &position) == FVM_OK && position == 0, "seek zero");
    DVM_IoResult result = dvm_vfs_read_chunk(file, bytes, sizeof(bytes));
    check(result.status == FVM_OK && result.count == strlen(expected) && !memcmp(bytes, expected, result.count), "file contents");
    check(!(result.flags & DVM_IO_PROBED), "successful read made metadata preflight");
}


/* EN/RU: Native threads share one session while retaining independent open cursors.
 * Native threads делят session, сохраняя независимые open cursors. */
static void *reader(void *state)
{
    for (unsigned i = 0; i < 12; i++)
    {
        contents(state, "initial");
    }
    return NULL;
}


/* EN/RU: Exercise live names, full-width Qids/offsets and factory-free retained opens.
 * Проверить live names, Qid/offset64 и opens после release factory. */
static void basic(DVM_Provider *provider)
{
    DVM_VfsHandle a = openfile(provider, "first", "r");
    DVM_VfsHandle b = openfile(provider, "first", "r");
    DVM_VfsHandle control = openfile(provider, "ctl", "wo");
    DVM_VfsHandle directory = openfile(provider, ".", "r");
    pthread_t threads[2];
    check(!pthread_create(&threads[0], NULL, reader, &a) && !pthread_create(&threads[1], NULL, reader, &b), "native threads");
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    long position;
    check(dvm_vfs_seek(&a, 0x123456789abcdefL, FVM_FILE_SEEK_START, &position) == FVM_OK, "offset64 seek");
    char bytes[32];
    DVM_IoResult result = dvm_vfs_read_chunk(&a, bytes, sizeof(bytes));
    check(result.status == FVM_OK && !result.count && result.stop == DVM_IO_EOF, "offset64 EOF");
    FVM_FileInfo meta;
    check(dvm_vfs_info(&a, &meta) == FVM_OK && meta.identity.high == 0xfedcba98UL && meta.identity.low == 0x76543210UL, "Qid64");
    result = dvm_vfs_write_chunk(&control, "change", 6);
    check(result.status == FVM_OK && result.count == 6, "control write");
    contents(&a, "changed");
    contents(&b, "changed");
    FVM_FileInfo *entries;
    size_t count;
    check(dvm_vfs_list(&directory, &entries, &count) == FVM_OK && count == 1 && !strcmp(entries[0].name, "later") && entries[0].mode == 0640, "live directory records");
    fvm_port_directory_entries_free(entries);
    check(dvm_vfs_list(&directory, &entries, &count) == FVM_OK && count == 0 && !entries, "actual directory EOF");
    check(dvm_provider_info(provider, "first", &meta) != FVM_OK && dvm_provider_info(provider, "later", &meta) == FVM_OK, "live namespace lookup");
    DVM_VfsHandle refused = {0};
    check(dvm_provider_open(provider, "later", "wo", &refused) != FVM_OK && !refused.state, "operation beats writable metadata");
    dvm_provider_release(provider);
    contents(&a, "changed");
    dvm_vfs_close(&a);
    contents(&b, "changed");
    dvm_vfs_close(&b);
    dvm_vfs_close(&directory);
    dvm_vfs_close(&control);
}


/* EN/RU: Test an acknowledged short write, Rerror uncertainty and no automatic replay.
 * Проверить short write, Rerror uncertainty и отсутствие replay. */
static void partial(DVM_Provider *provider)
{
    DVM_VfsHandle read = openfile(provider, "file", "r");
    DVM_VfsHandle write = openfile(provider, "file", "rw");
    DVM_VfsHandle control = openfile(provider, "ctl", "wo");
    DVM_IoResult result = dvm_vfs_write_chunk(&write, "HELLO", 5);
    check(result.status == FVM_OK && result.count == 3 && !(result.flags & DVM_IO_UNCERTAIN), "exact short Rwrite");
    result = dvm_vfs_write_chunk(&write, "xx", 2);
    check(result.status != FVM_OK && !result.count && (result.flags & DVM_IO_UNCERTAIN) && (result.flags & DVM_IO_PROBED), "failed write uncertainty and post-stat");
    result = dvm_vfs_write_chunk(&write, "xx", 2);
    check(result.status != FVM_OK && !result.count, "poisoned write retried");
    contents(&read, "HELdefgh");
    result = dvm_vfs_write_chunk(&control, "unpublish", 9);
    check(result.status == FVM_OK && result.count == 9, "remote unpublish");
    contents(&read, "HELdefgh");
    long position;
    check(dvm_vfs_seek(&control, 0, FVM_FILE_SEEK_START, &position) == FVM_OK, "control rewind");
    result = dvm_vfs_write_chunk(&control, "stop", 4);
    check(result.status == FVM_OK && result.count == 4, "remote stop");
    dvm_provider_release(provider);
    dvm_vfs_close(&read);
    dvm_vfs_close(&write);
    dvm_vfs_close(&control);
}


/* EN/RU: Exercise bounded chunks or terminal failures without exposing fake EOF/success.
 * Проверить bounded chunks и terminal failures без fake EOF/success. */
static void exceptional(DVM_Provider *provider, const char *mode)
{
    DVM_NoticeTap *tap = dvm_notice_subscribe(dvm_changes_notices(dvm_provider_changes(provider)), notice, NULL, NULL);
    check(tap != NULL, "native event subscription");
    if (!strcmp(mode, "badtag") || !strcmp(mode, "badwalk"))
    {
        FVM_FileInfo meta;
        check(dvm_provider_info(provider, ".", &meta) != FVM_OK && lost == 1, "malformed tag invalidates provider");
        check(dvm_provider_info(provider, ".", &meta) != FVM_OK && lost == 1, "loss event duplicated");
    }
    else
    {
        DVM_VfsHandle file = openfile(provider, !strcmp(mode, "dirbad") ? "." : "first", "r");
        if (!strcmp(mode, "dirbad"))
        {
            FVM_FileInfo *entries;
            size_t count;
            check(dvm_vfs_list(&file, &entries, &count) != FVM_OK && !entries && !count, "unsafe directory name accepted");
            check(dvm_vfs_list(&file, &entries, &count) != FVM_OK, "directory error became EOF");
        }
        else if (!strcmp(mode, "iounit"))
        {
            char bytes[16];
            size_t total = 0;
            DVM_IoResult result;
            do
            {
                result = dvm_vfs_read_chunk(&file, bytes+total, sizeof(bytes)-total);
                check(result.status == FVM_OK && result.count <= 2, "Ropen iounit ceiling");
                total += result.count;
            } while (result.count);
            check(total == 7 && !memcmp(bytes, "initial", 7), "bounded read reconstruction");
        }
        else
        {
            DVM_VfsHandle control = {0};
            char bytes[16];
            DVM_IoResult result;
            if (!strcmp(mode, "loss"))
            {
                control = openfile(provider, "ctl", "wo");
                result = dvm_vfs_write_chunk(&control, "change", 6);
                check(result.status != FVM_OK && (result.flags & DVM_IO_UNCERTAIN), "lost write acknowledgement treated as success");
                result = dvm_vfs_write_chunk(&control, "change", 6);
                check(result.status != FVM_OK, "uncertain write replay");
            }
            else
            {
                result = dvm_vfs_read_chunk(&file, bytes, sizeof(bytes));
                check(result.status != FVM_OK && (result.flags & DVM_IO_UNCERTAIN), "malformed read count accepted");
            }
            result = dvm_vfs_read_chunk(&file, bytes, sizeof(bytes));
            check(result.status != FVM_OK && lost == 1, "broken session revived/fake EOF or duplicate event");
            dvm_vfs_close(&control);
        }
        dvm_vfs_close(&file);
    }
    dvm_notice_unsubscribe(tap);
    dvm_provider_release(provider);
}


/* EN/RU: A host read-only ceiling refuses open before wire effects; held reads survive.
 * Host read-only ceiling отклоняет open до wire effects; held reads продолжаются. */
static void readonly(DVM_Provider *provider, Stream *stream)
{
    DVM_VfsHandle file = openfile(provider, "first", "r");
    DVM_VfsHandle denied = {0};
    unsigned before = stream->calls;
    check(dvm_provider_open(provider, "ctl", "wo", &denied) != FVM_OK && stream->calls == before, "host ceiling reached remote open");
    DVM_IoResult result = dvm_vfs_write_chunk(&file, "no", 2);
    check(result.status != FVM_OK && result.count == 0 && !(result.flags & DVM_IO_UNCERTAIN), "read-only handle widened");
    contents(&file, "initial");
    dvm_provider_release(provider);
    dvm_vfs_close(&file);
}


/* EN/RU: Walk beyond MAXWELEM and reject a partial walk without leaking its new fid.
 * Пройти больше MAXWELEM и убрать partial walk без утечки нового fid. */
static void deep(DVM_Provider *provider)
{
    char path[128] = "";
    for (unsigned i = 0; i < 17; i++)
    {
        strcat(path, "d/");
    }
    strcat(path, "file");
    DVM_VfsHandle file = openfile(provider, path, "r");
    contents(&file, "depth");
    FVM_FileInfo meta;
    check(dvm_provider_info(provider, "d/d/missing", &meta) != FVM_OK, "partial walk was accepted");
    DVM_VfsHandle control = openfile(provider, "ctl", "wo");
    DVM_IoResult result = dvm_vfs_write_chunk(&control, "stop", 4);
    check(result.status == FVM_OK && result.count == 4, "deep fixture stop");
    dvm_provider_release(provider);
    dvm_vfs_close(&file);
    dvm_vfs_close(&control);
}


/* EN/RU: Connect only to explicit local fixture and transfer native ownership to client.
 * Подключиться только к явному local fixture и передать native ownership client. */
int main(int argc, char **argv)
{
    check(argc == 3, "arguments");
    Stream stream = {0};
    stream.fd = socket(AF_INET, SOCK_STREAM, 0);
    check(stream.fd >= 0, "socket");
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons((unsigned short)atoi(argv[1]));
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    struct timeval timeout = {5, 0};
    int one = 1;
    setsockopt(stream.fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(stream.fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    setsockopt(stream.fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    check(!connect(stream.fd, (struct sockaddr *)&address, sizeof(address)), "connect");
    stream.budget = !strcmp(argv[2], "slow") ? 120 : 5000;
    StyxTransport transport = {&stream, receive, sendbytes, closestream, begin};
    int multiplex = getenv("STYX_NATIVE_MUX") != NULL;
    StyxClient *client = multiplex ? styxclientmountfd(stream.fd, 1024, "alice",
        !strcmp(argv[2], "prepared") ? "service" : "", stream.budget, 8) :
        styxclientmount(&transport, 1024, "alice", !strcmp(argv[2], "prepared") ? "service" : "");
    if (multiplex && client)
    {
        check(styxclientretain(client), "native monitor pin");
    }
    if (multiplex && !client)
    {
        check(fcntl(stream.fd, F_GETFD) < 0 && errno == EBADF, "rejected fd not closed");
        stream.closed = 1;
    }
    if (!strcmp(argv[2], "msize"))
    {
        check(!client && stream.closed == 1, "oversized negotiated msize accepted/leaked");
    }
    else
    {
        check(client != NULL, "native version/attach");
        DVM_Provider *provider = styxmountprovider(client, !strcmp(argv[2], "readonly") ? FVM_ACCESS_READ|FVM_ACCESS_EXEC : FVM_ACCESS_MASK);
        check(provider != NULL, "ordinary mount provider");
        unsigned calls = stream.calls;
        FVM_FileInfo meta;
        check(dvm_provider_info(provider, "../escape", &meta) != FVM_OK && stream.calls == calls, "escaping path reached transport");
        check(dvm_provider_access(provider, ".", FVM_ACCESS_READ) == FVM_UNSUPPORTED, "metadata fabricated effective access");
        if (!strcmp(argv[2], "basic"))
        {
            basic(provider);
        }
        else if (!strcmp(argv[2], "prepared"))
        {
            DVM_VfsHandle file = openfile(provider, "file", "r");
            dvm_provider_release(provider);
            contents(&file, "bravo");
            char bytes[8];
            DVM_IoResult result = dvm_vfs_read_chunk(&file, bytes, sizeof(bytes));
            check(result.status == FVM_OK && !result.count && result.stop == DVM_IO_EOF, "prepared held fid EOF");
            dvm_vfs_close(&file);
        }
        else if (!strcmp(argv[2], "readonly"))
        {
            readonly(provider, &stream);
        }
        else if (!strcmp(argv[2], "deep"))
        {
            deep(provider);
        }
        else if (!strcmp(argv[2], "partial"))
        {
            partial(provider);
        }
        else
        {
            exceptional(provider, argv[2]);
        }
        if (multiplex)
        {
            styxclientshutdown(client);
            check(fcntl(stream.fd, F_GETFD) < 0 && errno == EBADF, "fd owner not retired");
            styxclientfree(client);
            stream.closed = 1;
        }
        check(stream.closed == 1, "transport not consumed exactly once");
    }
    printf("PASS native mount provider: %s, one transport close\n", argv[2]);
    return 0;
}
