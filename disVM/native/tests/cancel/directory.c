/* Copyright (c) 2026 bhgv
 * EN: Complete directory records and policy-checked prefixes through scope, view and original Styx RPCs.
 * RU: Полные записи каталога и проверенные префиксы через scope, view и исходные Styx RPC.
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
    DVM_VfsHandle *held; /* Borrowed already-open directory with caller FIFO ownership. */
    FVM_FileInfo *entries; /* Owned acknowledged and policy-checked entries. */
    size_t count; /* Number of complete entries, never fabricated EOF on cancellation. */
    FVM_Result status; /* Actual directory operation result. */
} Work;


/* EN/RU: Explicit contracts remain active in every build mode.
 * Явные contracts активны в каждом build mode. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "mounted directory cancellation: %s\n", why);
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
    work->status = dvm_vfs_list(work->held, &work->entries, &work->count);
    check(dvm_cancel_leave(&scope) && dvm_cancel_current() == NULL, "scope retirement");
    return NULL;
}


/* EN/RU: Connect to the original server; compare owned entries with real reply/flush order.
 * Main joins the scope owner before inspecting cursor/results or consuming providers.
 * Подключиться к original server; сверить entries с real reply/flush order;
 * main делает join до чтения cursor/results и освобождения providers. */
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
    int raw = getenv("STYX_VFS_RAW") != NULL;
    DVM_Provider *provider = source;
    if (!raw)
    {
        DVM_View *view = dvm_view_create(NULL, NULL, NULL);
        check(view && dvm_view_map(view, "/", source, ".", 7, DVM_VIEW_TREE), "prepared mapping");
        provider = dvm_view_freeze(view);
        check(provider != NULL, "prepared provider");
    }
    DVM_VfsHandle held = {0};
    check(dvm_provider_open(provider, ".", "r", &held) == FVM_OK &&
          (held.native_flags & DVM_VFS_CANCEL_LIST), "directory cancellation promise lost");
    DVM_Cancel *cancel = dvm_cancel_create();
    check(cancel != NULL, "source");
    Work work = {0};
    work.cancel = cancel;
    work.held = &held;
    puts("ready");
    fflush(stdout);
    checkpoint();
    if (!strcmp(mode, "precancel"))
    {
        dvm_cancel_request(cancel);
    }
    pthread_t worker;
    check(!pthread_create(&worker, NULL, run, &work), "native directory worker");
    if (strcmp(mode, "success") && strcmp(mode, "timeout") && strcmp(mode, "precancel") && strcmp(mode, "truncated"))
    {
        checkpoint();
        dvm_cancel_request(cancel);
    }
    check(!pthread_join(worker, NULL), "native completion");
    int race = !strcmp(mode, "race") || !strcmp(mode, "racefail");
    int metadata = strcmp(operation, "list") != 0;
    int metadata_timeout = metadata && !strcmp(mode, "timeout");
    int success = metadata_timeout || !strcmp(mode, "success") || (race && (raw || metadata)) ||
        (!strcmp(operation, "listprefix") && !strcmp(mode, "cancel"));
    size_t expected = !strcmp(mode, "success") || (raw && race) ? 2 :
        race && !strcmp(operation, "listprefix") ? 2 : success ? 1 : 0;
    check((work.status == FVM_OK) == success && work.count == expected &&
          !!work.entries == !!expected, "directory acknowledgement/prefix/EOF mismatch");
    if (expected)
    {
        check(!strcmp(work.entries[0].name, metadata_timeout && !strcmp(operation, "liststat") ? "ctl" : "data"), "first complete record");
        if (expected == 2)
        {
            check(!strcmp(work.entries[1].name, "ctl"), "second complete record");
        }
    }
    fvm_port_directory_entries_free(work.entries);
    if (strcmp(mode, "success") && strcmp(mode, "precancel") && !(raw && race) && !metadata_timeout)
    {
        FVM_FileInfo *next = NULL;
        size_t count = 0;
        check(dvm_vfs_list(&held, &next, &count) != FVM_OK && !next && !count,
              "interrupted cursor silently resumed or reported EOF");
    }
    if (!strcmp(mode, "success"))
    {
        FVM_FileInfo *next = NULL;
        size_t count = 0;
        check(dvm_vfs_list(&held, &next, &count) == FVM_OK && !next && !count,
              "real acknowledged directory EOF lost");
    }
    check(styxclientalive(client) == (strcmp(mode, "racefail") != 0), "cancellation broke unrelated transport");
    puts("returned");
    puts("progress");
    fflush(stdout);
    checkpoint();
    dvm_vfs_close(&held);
    dvm_cancel_release(cancel);
    if (provider != source)
    {
        dvm_provider_release(provider);
    }
    dvm_provider_release(source);
    styxclientshutdown(client);
    styxclientfree(client);
    printf("PASS native control cancellation %s %s\n", operation, mode);
    return 0;
}
