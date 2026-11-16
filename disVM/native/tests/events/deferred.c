/* Copyright (c) 2026 bhgv
 * EN: Actual native client reading a real deferred C server while heartbeat/cancellation progresses.
 * RU: Настоящий native client читает deferred C server при heartbeat/cancellation.
 */
#include "client.h"
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Main retains fid/token through admission and joins before consuming copied result.
 * Main удерживает fid/token до admission и join перед чтением copied result. */
typedef struct Work
{
    StyxFid *fid; /* Borrowed independently held remote file. */
    StyxCancel *cancel; /* Borrowed main token, operation independently pins after admission. */
    StyxTransfer result; /* Worker-owned until join. */
    char bytes[16]; /* Exclusive read storage. */
} Work;


/* EN/RU: Active native check, without assert/build-mode dependence.
 * Активная native check без зависимости от assert/build mode. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "native deferred read: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Execute one actual read on an independent native caller, never a guest callback.
 * Выполнить read на отдельном native caller, не guest callback. */
static void *reading(void *opaque)
{
    Work *work = opaque;
    styxclientreadcancel(work->fid, 0, work->bytes, sizeof(work->bytes), work->cancel, &work->result);
    return NULL;
}


/* EN/RU: Capture one ordinary independently opened event fid.
 * Захватить один обычный independently opened event fid. */
static StyxFid *openfile(StyxClient *client)
{
    StyxFid *fid = styxclientwalk(client, "events");
    check(fid && styxclientopen(fid, 0), "event open");
    return fid;
}


/* EN/RU: External fixture gate starts only after real server admission and repeated native probes.
 * External fixture gate начинается после real server admission и повторных native probes. */
int main(int argc, char **argv)
{
    check(argc == 3, "port/mode");
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(atoi(argv[1]));
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    check(fd >= 0 && !connect(fd, (struct sockaddr *)&address, sizeof(address)), "connect");
    StyxClient *client = styxclientmountfd(fd, 1024, "alice", "", 5000, 8);
    check(client && styxclientkeepalive(client, 40, 500, 4), "native mount/heartbeat");
    Work work = {0};
    work.fid = openfile(client);
    work.cancel = styxcancelcreate();
    check(work.cancel != NULL, "native cancellation token");
    pthread_t worker;
    check(!pthread_create(&worker, NULL, reading, &work), "native reader");
    check(getchar() == 'g', "external admission gate");
    if (!strcmp(argv[2], "cancel"))
    {
        styxcancelrequest(work.cancel);
    }
    check(!pthread_join(worker, NULL), "native reader join");
    styxcancelfree(work.cancel);
    styxclientclunk(work.fid);
    if (!strcmp(argv[2], "cancel"))
    {
        check(work.result.cancelled && !work.result.success && !work.result.count && styxclientalive(client), "read cancellation closed healthy session");
        puts("cancelled");
        fflush(stdout);
        memset(&work, 0, sizeof(work));
        work.fid = openfile(client);
        check(!pthread_create(&worker, NULL, reading, &work) && !pthread_join(worker, NULL), "new read after Rflush");
        styxclientclunk(work.fid);
    }
    check(work.result.success && work.result.count == 3 && !memcmp(work.bytes, "one", 3), "deferred event lost/aggregated");
    styxclientshutdown(client);
    styxclientfree(client);
    printf("PASS actual native deferred read: %s\n", argv[2]);
    return 0;
}
