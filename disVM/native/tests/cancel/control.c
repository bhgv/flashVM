/* Copyright (c) 2026 bhgv
 * EN: Actual mounted-provider control operations through scope, view and original Styx RPCs.
 * RU: Actual mounted-provider control operations через scope, view и original Styx RPCs.
 */
#include "mount.h"
#include "export.h"
#include "cancel.h"
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: One native worker owns its result; main pins every borrowed capability until join.
 * Один native worker владеет result; main удерживает borrowed capabilities до join. */
typedef struct Work
{
    DVM_Cancel *cancel; /* Borrowed sticky source, independently pinned by scope. */
    DVM_Provider *provider; /* Borrowed prepared/native provider. */
    DVM_VfsHandle *held; /* Borrowed already-open file with caller FIFO ownership. */
    DVM_VfsHandle output; /* Owned newly opened result, transferred to main. */
    FVM_FileInfo metadata; /* Copied only on acknowledged metadata success. */
    FVM_Result status; /* Actual provider operation result. */
    long position; /* Seek output must remain unchanged on failure. */
    const char *operation; /* pathstat/pathopen/fdstat/seek/cleanup. */
} Work;


/* EN/RU: Explicit contracts remain active in every build mode.
 * Явные contracts активны в каждом build mode. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "mounted control cancellation: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Wait for a driver-observed real wire boundary, never guessed elapsed time.
 * Ждать driver-observed real wire boundary, без guessed elapsed time. */
static void checkpoint(void)
{
    char command[32];
    check(fgets(command, sizeof(command), stdin) != NULL, "driver checkpoint");
}


/* EN/RU: Dispatch ordinary provider methods under exactly one logical native scope.
 * Выполнить ordinary provider methods под одним logical native scope. */
static void *run(void *opaque)
{
    Work *work = opaque;
    DVM_CancelScope scope = {0};
    check(dvm_cancel_enter(&scope, work->cancel), "native scope");
    if (!strcmp(work->operation, "pathopen"))
    {
        work->status = dvm_provider_open(work->provider, "data", "r", &work->output);
    }
    else if (!strcmp(work->operation, "dirclose"))
    {
        dvm_vfs_close(work->held);
        work->status = FVM_OK;
    }
    else if (!strcmp(work->operation, "fdstat"))
    {
        work->status = dvm_vfs_info(work->held, &work->metadata);
    }
    else if (!strcmp(work->operation, "seek"))
    {
        work->status = dvm_vfs_seek(work->held, 0, FVM_FILE_SEEK_END, &work->position);
    }
    else
    {
        work->status = dvm_provider_info(work->provider, "data", &work->metadata);
    }
    check(dvm_cancel_leave(&scope) && dvm_cancel_current() == NULL, "scope retirement");
    return NULL;
}


/* EN/RU: Verify logical method acknowledgements, held-fid reuse, cleanup and cursor preservation.
 * Проверить logical method acknowledgements, held-fid reuse, cleanup и cursor preservation. */
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
    StyxClient *client = styxclientmountfd(fd, 1024, "fixture", "", !strcmp(mode, "racefail") ||
        !strcmp(mode, "timeout") ? 300 : 3000, 3);
    check(client && styxclientretain(client), "client");
    DVM_Provider *source = styxmountprovider(client, 7);
    check(source && (dvm_provider_native_flags(source) & DVM_PROVIDER_CANCEL_PATH), "native path promise");
    DVM_View *view = dvm_view_create(NULL, NULL, NULL);
    check(view && dvm_view_map(view, "/", source, ".", 7, DVM_VIEW_TREE), "prepared mapping");
    DVM_Provider *provider = dvm_view_freeze(view);
    check(provider && (dvm_provider_native_flags(provider) & DVM_PROVIDER_CANCEL_PATH), "view lost path promise");
    DVM_VfsHandle held = {0};
    int closing = !strcmp(operation, "dirclose");
    check(dvm_provider_open(provider, closing ? "." : "data", "r", &held) == FVM_OK &&
          (closing ? (held.native_flags & DVM_VFS_CANCEL_LIST) :
          (held.native_flags & (DVM_VFS_CANCEL_INFO|DVM_VFS_CANCEL_SEEK)) == (DVM_VFS_CANCEL_INFO|DVM_VFS_CANCEL_SEEK)),
          "view lost held control capabilities");
    DVM_Cancel *cancel = dvm_cancel_create();
    check(cancel != NULL, "source");
    Work work = {0};
    work.cancel = cancel;
    work.provider = provider;
    work.held = &held;
    work.operation = operation;
    work.position = 77;
    memset(&work.metadata, 0x5a, sizeof(work.metadata));
    FVM_FileInfo before = work.metadata;
    puts("ready");
    fflush(stdout);
    checkpoint();
    if (!strcmp(mode, "precancel"))
    {
        dvm_cancel_request(cancel);
    }
    pthread_t worker;
    check(!pthread_create(&worker, NULL, run, &work), "native control worker");
    if (strcmp(mode, "success") && strcmp(mode, "timeout") && strcmp(mode, "precancel"))
    {
        checkpoint();
        dvm_cancel_request(cancel);
    }
    check(!pthread_join(worker, NULL), "native completion");
    int success = closing || !strcmp(mode, "success") || !strcmp(mode, "race") || !strcmp(mode, "racefail") ||
        (!strcmp(operation, "cleanup") && strcmp(mode, "precancel"));
    check((work.status == FVM_OK) == success, "method acknowledgement lost/fabricated");
    if (closing)
    {
        check(!held.ops && !held.state, "close did not consume held directory");
    }
    else if (!strcmp(operation, "pathopen"))
    {
        check(!!work.output.ops == success, "open ownership contract");
    }
    else if (!strcmp(operation, "seek"))
    {
        check(work.position == (success ? 5 : 77), "seek published unknown position");
        long current = -1;
        check(dvm_vfs_seek(&held, 0, FVM_FILE_SEEK_CURRENT, &current) == FVM_OK && current == (success ? 5 : 0),
              "failed seek changed actual cursor");
    }
    else if (success)
    {
        check(work.metadata.identity.low == 99, "metadata identity");
    }
    else
    {
        check(!memcmp(&before, &work.metadata, sizeof(before)), "failed metadata modified output");
    }
    check(styxclientalive(client) == (strcmp(mode, "racefail") != 0), "cancellation broke unrelated transport");
    puts("returned");
    puts("progress");
    fflush(stdout);
    checkpoint();
    dvm_vfs_close(&work.output);
    dvm_vfs_close(&held);
    dvm_cancel_release(cancel);
    dvm_provider_release(provider);
    dvm_provider_release(source);
    styxclientshutdown(client);
    styxclientfree(client);
    printf("PASS native control cancellation %s %s\n", operation, mode);
    return 0;
}
