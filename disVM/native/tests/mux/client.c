/* Copyright (c) 2026 bhgv
 * EN: Native OS-thread multiplex/flush acceptance with deterministic external event gates.
 * RU: Native OS-thread multiplex/flush acceptance с deterministic external event gates.
 */
#include "client.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Each worker owns one fid operation and its copied acknowledgement.
 * Каждый worker владеет одной fid operation и copied acknowledgement. */
typedef struct Work
{
    StyxFid *fid; /* Borrowed, main closes only after worker join. */
    StyxCancel *cancel; /* Borrowed until admission; job then independently retains. */
    unsigned offset; /* Distinguish replies for equal-Qid fids. */
    int writing; /* Controlled synthetic change command. */
    StyxTransfer result; /* Worker-owned copied operation outcome. */
    char bytes[8]; /* Independent receive storage. */
} Work;


/* EN/RU: Keep checks active in optimized builds.
 * Сохранить проверки в optimized builds. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "native multiplex: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Issue one cancellable native RPC, never enter guest execution.
 * Выполнить один cancellable native RPC без guest execution. */
static void *run(void *argument)
{
    Work *work = argument;
    if (work->writing)
    {
        styxclientwritecancel(work->fid, 0, "change", 6, work->cancel, &work->result);
    }
    else
    {
        styxclientreadcancel(work->fid, work->offset, work->bytes, 2, work->cancel, &work->result);
    }
    return NULL;
}


/* EN/RU: Walk/open independent objects before testing simultaneous data RPCs.
 * Walk/open независимых объектов до simultaneous data RPCs. */
static StyxFid *openfile(StyxClient *client, const char *path, int mode)
{
    StyxFid *fid = styxclientwalk(client, path);
    check(fid && styxclientopen(fid, mode), path);
    return fid;
}


/* EN/RU: Match exact acknowledgement/data, without guessing from cancellation intent.
 * Проверить точный acknowledgement/data, не по cancellation intent. */
static void acknowledged(Work *work, const char *expected)
{
    check(work->result.success && !work->result.cancelled && !work->result.uncertain,
        "known acknowledgement lost to cancellation");
    check(work->result.count == (work->writing ? 6U : 2U), "wrong reply count");
    if (!work->writing)
    {
        check(!memcmp(work->bytes, expected, 2), "reply routed to wrong native waiter");
    }
}


/* EN/RU: The driver only signals after observing admitted wire frames; no sleeps.
 * Driver сигнализирует только после wire admission; без sleeps в native client. */
int main(int argc, char **argv)
{
    check(argc == 3, "arguments");
    const char *mode = argv[2];
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons((unsigned short)atoi(argv[1]));
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    check(fd >= 0 && !connect(fd, (struct sockaddr *)&address, sizeof(address)), "connect");
    unsigned timeout = (!strcmp(mode, "timeout") || !strcmp(mode, "noack") || !strcmp(mode, "failall") || !strcmp(mode, "racefail")) ? 250 : 3000;
    StyxClient *client = styxclientmountfd(fd, 1024, "alice", "", timeout, 2);
    check(client != NULL, "version/attach");
    int writing = !strcmp(mode, "writecancel") || !strcmp(mode, "writerace");
    StyxFid *a = openfile(client, writing ? "ctl" : "first", writing ? 1 : 0);
    StyxFid *b = openfile(client, "first", 0);
    StyxFid *metadata = !strcmp(mode, "quota") ? styxclientwalk(client, "first") : NULL;
    if (!strcmp(mode, "finaldrop"))
    {
        styxclientclunk(a);
        styxclientclunk(b);
        styxclientfree(client);
        puts("owner dropped");
        fflush(stdout);
        char checkpoint[32];
        check(fgets(checkpoint, sizeof(checkpoint), stdin) != NULL, "external EOF checkpoint");
        check(fcntl(fd, F_GETFD) < 0 && errno == EBADF, "autonomous owner failed to retire");
        puts("PASS native multiplex/flush: finaldrop");
        return 0;
    }
    StyxCancel *cancel = styxcancelcreate();
    check(cancel != NULL, "cancel capability");
    Work first = {0}, second = {0};
    first.fid = a;
    first.cancel = cancel;
    first.writing = writing;
    second.fid = b;
    second.offset = 2;
    int parallel = !strcmp(mode, "reorder") || !strcmp(mode, "quota") || !strcmp(mode, "failall") || !strcmp(mode, "shutdown");
    pthread_t threads[2];
    if (!strcmp(mode, "precancel"))
    {
        styxcancelrequest(cancel);
        run(&first);
        check(first.result.cancelled && !first.result.success && !first.result.uncertain && !first.result.count,
            "unsent cancellation fabricated effects");
    }
    else
    {
        check(!pthread_create(&threads[0], NULL, run, &first), "first native worker");
        if (parallel)
        {
            check(!pthread_create(&threads[1], NULL, run, &second), "second native worker");
        }
        if (strcmp(mode, "reorder") && strcmp(mode, "timeout"))
        {
            char command[32];
            check(fgets(command, sizeof(command), stdin) != NULL, "driver admission gate");
            if (!strcmp(mode, "quota"))
            {
                StyxMeta meta;
                check(metadata && !styxclientstat(metadata, &meta) && styxclientalive(client), "quota failure broke connection");
                puts("quota refused");
                fflush(stdout);
            }
            else if (!strcmp(mode, "shutdown"))
            {
                styxclientshutdown(client);
            }
            else
            {
                if (strcmp(mode, "detach"))
                {
                    styxcancelrequest(cancel);
                }
                styxcancelfree(cancel);
                cancel = NULL;
                if (!strcmp(mode, "detach"))
                {
                    puts("caller reference dropped");
                    fflush(stdout);
                }
            }
        }
        pthread_join(threads[0], NULL);
        if (parallel)
        {
            pthread_join(threads[1], NULL);
        }
    }
    if (!strcmp(mode, "late"))
    {
        puts("waiting for invalid reply retirement");
        fflush(stdout);
        char checkpoint[32];
        check(fgets(checkpoint, sizeof(checkpoint), stdin) != NULL, "late-reply EOF checkpoint");
    }
    int broken = !strcmp(mode, "noack") || !strcmp(mode, "failall") || !strcmp(mode, "shutdown") || !strcmp(mode, "late") || !strcmp(mode, "badflush") || !strcmp(mode, "racefail");
    if (parallel && !broken)
    {
        acknowledged(&first, "in");
        acknowledged(&second, "it");
    }
    else if (!strcmp(mode, "race") || !strcmp(mode, "writerace") || !strcmp(mode, "detach") || !strcmp(mode, "racefail"))
    {
        acknowledged(&first, "in");
    }
    else if (strcmp(mode, "precancel"))
    {
        check(!first.result.success && first.result.uncertain && first.result.confirmed && !first.result.count,
            "unacknowledged effects were guessed/replayed");
        check(first.result.cancelled || !strcmp(mode, "shutdown"), "missing cancellation result");
        if (parallel)
        {
            check(!second.result.success && second.result.uncertain, "another pending waiter was stranded");
        }
    }
    check(styxclientalive(client) == !broken, "wrong terminal/healthy connection state");
    if (!broken && !parallel)
    {
        StyxTransfer result;
        char bytes[8];
        styxclientread(b, 0, bytes, 2, &result);
        check(result.success && result.count == 2 && !memcmp(bytes, writing ? "ch" : "in", 2), "unrelated fid failed after cancellation");
    }
    if (writing)
    {
        if (!strcmp(mode, "writecancel"))
        {
            StyxTransfer result;
            styxclientwrite(a, 0, "change", 6, &result);
            check(!result.success && !result.count, "uncertain write replayed");
        }
    }
    styxcancelfree(cancel);
    styxclientclunk(metadata);
    styxclientclunk(a);
    styxclientclunk(b);
    styxclientshutdown(client);
    check(fcntl(fd, F_GETFD) < 0 && errno == EBADF, "native fd owner still live");
    styxclientfree(client);
    printf("PASS native multiplex/flush: %s\n", mode);
    return 0;
}
