/* Copyright (c) 2026 bhgv
 * EN: Actual native control-RPC cancellation with externally observed wire checkpoints.
 * RU: Настоящая native control-RPC cancellation с внешними wire checkpoints.
 */
#include "client.h"
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: One native worker owns outputs; main pins inputs until admitted completion.
 * Один native worker владеет outputs; main удерживает inputs до completion. */
typedef struct Work
{
    StyxClient *client; /* Borrowed connection, pinned by main. */
    StyxFid *base; /* Borrowed unopened base, pinned until join. */
    StyxFid *fid; /* Owned by main, or moved to the remove worker. */
    StyxFid *walked; /* Owned result, transferred to main after join. */
    StyxCancel *cancel; /* Caller pin can drop after wire admission; scope/flight retains it. */
    StyxTransfer result; /* Exact copied control acknowledgement, count always zero. */
    StyxMeta meta; /* Output must remain untouched after any failed stat. */
    const char *operation; /* walk/open/stat selected by the driver. */
    const char *mode; /* Deterministic cancellation/reply order. */
    const char *path; /* Full or base-relative canonical path. */
    int status; /* Native open/stat return code. */
} Work;


/* EN/RU: Explicit test contracts remain active in optimized and sanitizer builds.
 * Явные test contracts активны в optimized и sanitizer builds. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "control cancel: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Await only a driver-observed native wire boundary, never a guessed sleep.
 * Ждать только driver-observed native wire boundary, без guessed sleep. */
static void checkpoint(void)
{
    char command[32];
    check(fgets(command, sizeof(command), stdin) != NULL, "driver checkpoint");
}


/* EN/RU: Read through an ordinary independent fid, proving other work still progresses.
 * Прочитать ordinary independent fid, проверяя progress остальных операций. */
static void readable(StyxFid *fid)
{
    char bytes[5] = {0};
    StyxTransfer result;
    styxclientread(fid, 0, bytes, sizeof(bytes), &result);
    check(result.success && result.count == sizeof(bytes) && !memcmp(bytes, "depth", 5), "independent fid progress");
}


/* EN/RU: Execute exactly one real native control call; no provider/guest callbacks.
 * Выполнить ровно один real native control call, без provider/guest callbacks. */
static void *run(void *opaque)
{
    Work *work = opaque;
    if (!strcmp(work->operation, "walk"))
    {
        work->walked = !strncmp(work->mode, "from-", 5) ?
            styxclientwalkfromcancel(work->base, work->path, work->cancel, &work->result) :
            styxclientwalkcancel(work->client, work->path, work->cancel, &work->result);
    }
    else if (!strcmp(work->operation, "open"))
    {
        work->status = styxclientopencancel(work->fid, !strcmp(work->mode, "error") ? 1 : 0,
                                          work->cancel, &work->result);
    }
    else if (!strcmp(work->operation, "create"))
    {
        work->status = styxclientcreatecancel(work->base, !strcmp(work->mode, "error") ? "reject" : "child",
            0666, 1, work->cancel, &work->result);
    }
    else if (!strcmp(work->operation, "remove"))
    {
        work->status = styxclientremovecancel(work->fid, work->cancel, &work->result);
    }
    else
    {
        work->status = styxclientstatcancel(work->fid, &work->meta, work->cancel, &work->result);
    }
    return NULL;
}


/* EN/RU: Verify precise outcomes, one-use tokens, uncertain-open fencing and native cleanup.
 * Проверить exact outcomes, one-use tokens, uncertain-open fencing и native cleanup. */
int main(int argc, char **argv)
{
    check(argc == 4, "port/operation/mode");
    const char *operation = argv[2], *mode = argv[3];
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons((unsigned short)atoi(argv[1]));
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    check(fd >= 0 && !connect(fd, (struct sockaddr *)&address, sizeof(address)), "connect");
    unsigned timeout = !strcmp(mode, "timeout") || !strcmp(mode, "racefail") ? 300 : 3000;
    const char *slots = getenv("STYX_CONTROL_SLOTS");
    unsigned limit = slots ? (unsigned)strtoul(slots, NULL, 10) : 3;
    StyxClient *client = styxclientmountfd(fd, 1024, "fixture", "", timeout, limit);
    check(client != NULL, "native mount");
    char path[128] = "";
    for (unsigned i = 0; i < 17; i++)
    {
        strcat(path, "d/");
    }
    strcat(path, "file");
    StyxFid *data = styxclientwalk(client, path);
    StyxFid *target = styxclientwalk(client, !strcmp(operation, "remove") && !strcmp(mode, "error") ? "ctl" : path);
    StyxFid *base = styxclientwalk(client, "d");
    check(data && target && base && styxclientopen(data, 0), "setup independent fids");
    Work work = {0};
    work.client = client;
    work.fid = target;
    if (!strcmp(operation, "remove"))
    {
        target = NULL;
    }
    work.base = base;
    work.operation = operation;
    work.mode = mode;
    work.path = !strcmp(mode, "partial") ? "d/missing" : !strncmp(mode, "from-", 5) ? path+2 : path;
    work.cancel = styxcancelcreate();
    check(work.cancel != NULL, "token");
    StyxTransfer invalid;
    check(!styxclientwalkcancel(client, "../d", work.cancel, &invalid) && !invalid.success && !invalid.uncertain,
          "invalid path reached wire/consumed token");
    check(!styxclientwalkcancel(client, ".", work.cancel, NULL) &&
          !styxclientopencancel(work.fid, 0, work.cancel, NULL) &&
          !styxclientstatcancel(work.fid, &work.meta, work.cancel, NULL), "missing outcome admitted RPC");
    memset(&work.meta, 0x5a, sizeof(work.meta));
    StyxMeta untouched = work.meta;
    puts("ready");
    fflush(stdout);
    checkpoint();
    int precancel = !strcmp(mode, "precancel");
    int immediate = precancel || !strcmp(mode, "success") || !strcmp(mode, "from-success") ||
        !strcmp(mode, "partial") || !strcmp(mode, "error") || !strcmp(mode, "stepfail") || !strcmp(mode, "between");
    if (precancel)
    {
        styxcancelrequest(work.cancel);
    }
    pthread_t worker;
    check(!pthread_create(&worker, NULL, run, &work), "native control worker");
    StyxCancel *caller = work.cancel;
    if (!immediate && strcmp(mode, "timeout"))
    {
        checkpoint();
        styxcancelrequest(caller);
        if (!strcmp(mode, "detach"))
        {
            styxcancelfree(caller);
            caller = NULL;
        }
    }
    check(!pthread_join(worker, NULL), "control join");
    int success = !strcmp(mode, "success") || !strcmp(mode, "from-success") ||
        !strcmp(mode, "race") || !strcmp(mode, "racefail");
    check(work.result.confirmed && !work.result.count && work.result.success == success, "control acknowledgement");
    if (success)
    {
        check(!work.result.cancelled && !work.result.uncertain, "acknowledgement lost to flush");
        if (!strcmp(operation, "walk"))
        {
            check(work.walked != NULL, "missing walked fid");
        }
        else
        {
            check(work.status == 1, "return code disagrees with acknowledgement");
        }
    }
    else if (!strcmp(mode, "partial") || !strcmp(mode, "error") || !strcmp(mode, "stepfail"))
    {
        check(!work.result.cancelled && !work.result.uncertain && !work.walked && !work.status, "known refusal became uncertainty");
    }
    else
    {
        check(work.result.cancelled && work.result.uncertain == !(precancel || !strcmp(mode, "between")) && !work.walked && !work.status,
              "cancelled control effects");
    }
    if (!strcmp(operation, "stat") && !success)
    {
        check(!memcmp(&work.meta, &untouched, sizeof(work.meta)), "cancelled stat modified output");
    }
    if (!strcmp(operation, "open") && work.result.uncertain)
    {
        check(!styxclientopen(target, 0), "uncertain Topen replayed");
        StyxTransfer rejected;
        char byte;
        styxclientread(target, 0, &byte, 1, &rejected);
        check(!rejected.success && !rejected.uncertain && !rejected.count, "uncertain Topen allowed data");
    }
    if (!strcmp(operation, "create") && work.result.uncertain)
    {
        StyxTransfer rejected;
        check(!styxclientcreate(base, "retry", 0666, 1, &rejected) && !rejected.uncertain,
              "uncertain Tcreate replayed");
    }
    puts("returned");
    fflush(stdout);
    if (!strcmp(mode, "racefail"))
    {
        check(!styxclientalive(client), "lost Rflush did not retire connection");
    }
    else
    {
        check(styxclientalive(client), "acknowledged flush broke connection");
        readable(data);
        if (!strcmp(operation, "open") && (!strcmp(mode, "error") || precancel))
        {
            check(styxclientopen(target, 0), "known unopened fid was poisoned");
        }
        if (!strcmp(operation, "stat"))
        {
            StyxMeta meta;
            check(styxclientstat(target, &meta) && meta.identity == 18, "cancelled stat poisoned fid");
        }
        if (caller)
        {
            StyxTransfer reused;
            check(!styxclientwalkcancel(client, ".", caller, &reused) && !reused.success && !reused.uncertain,
                  "control token reused for another logical operation");
        }
    }
    puts("progress");
    fflush(stdout);
    checkpoint();
    styxcancelfree(caller);
    styxclientclunk(work.walked);
    styxclientclunk(target);
    styxclientclunk(base);
    styxclientclunk(data);
    styxclientshutdown(client);
    styxclientfree(client);
    printf("PASS native control cancellation %s %s\n", operation, mode);
    return 0;
}
