/* Copyright (c) 2026 bhgv
 * EN: Real original-codec TCP cancellation through ordinary provider/prepared-view wrappers.
 * RU: Real original-codec TCP cancellation через ordinary provider/prepared-view wrappers.
 */
#include "mount.h"
#include "export.h"
#include "cancel.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Native source thread owns result/buffer, main pins every borrowed input through join.
 * Native source thread владеет result/buffer, main удерживает inputs до join. */
typedef struct Work
{
    DVM_Cancel *cancel; /* Borrowed source, scope takes an independent pin. */
    DVM_VfsHandle *fd; /* Borrowed provider stream under caller FIFO/lifetime ownership. */
    int writing; /* Immutable selected data direction. */
    DVM_IoResult result; /* Exact copied provider acknowledgement. */
    char bytes[5]; /* Independent receive storage. */
} Work;


/* EN/RU: Preserve semantic contracts in optimized/sanitizer builds.
 * Сохранить semantic contracts в optimized/sanitizer builds. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "mounted data cancel: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Await only externally observed actual request admission, never guessed sleeps.
 * Ждать только externally observed actual request admission, без guessed sleeps. */
static void checkpoint(void)
{
    char command[32];
    check(fgets(command, sizeof(command), stdin) != NULL, "driver checkpoint");
}


/* EN/RU: Actual native synchronous provider dispatch inherits this source-only cancellation scope.
 * Actual native synchronous provider dispatch наследует только этот cancellation scope. */
static void *run(void *opaque)
{
    Work *work = opaque;
    DVM_CancelScope scope = {0};
    check(dvm_cancel_current() == NULL && dvm_cancel_enter(&scope, work->cancel), "native scope admission");
    work->result = work->writing ? dvm_vfs_write_chunk(work->fd, "change", 6) :
        dvm_vfs_read_chunk(work->fd, work->bytes, sizeof(work->bytes));
    check(dvm_cancel_leave(&scope) && dvm_cancel_current() == NULL, "native scope retirement");
    return NULL;
}


/* EN/RU: Compare exact acknowledged effects with the observed reply/flush order.
 * Сравнить exact acknowledged effects с observed reply/flush order. */
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
    check(client && styxclientretain(client), "native connection");
    DVM_Provider *source = styxmountprovider(client, 7);
    check(source != NULL, "mount");
    DVM_Provider *provider = source;
    if (!getenv("STYX_VFS_RAW"))
    {
        DVM_View *view = dvm_view_create(NULL, NULL, NULL);
        check(view && dvm_view_map(view, "/", source, ".", 7, DVM_VIEW_TREE), "prepared mapping");
        provider = dvm_view_freeze(view);
        check(provider != NULL, "prepared provider");
    }
    DVM_VfsHandle stream = {0};
    int writing = !strcmp(operation, "write");
    check(dvm_provider_open(provider, "data", writing ? "wo" : "r", &stream) == FVM_OK &&
          (stream.native_flags & DVM_VFS_CANCEL_DATA), "wrapper lost data cancellation capability");
    DVM_Cancel *cancel = dvm_cancel_create();
    check(cancel != NULL, "native cancellation source");
    Work work = {cancel, &stream, writing, {0}, {0}};
    puts("ready");
    fflush(stdout);
    checkpoint();
    if (!strcmp(mode, "precancel"))
    {
        dvm_cancel_request(cancel);
    }
    pthread_t worker;
    check(!pthread_create(&worker, NULL, run, &work), "native I/O worker");
    if (strcmp(mode, "success") && strcmp(mode, "timeout") && strcmp(mode, "precancel"))
    {
        checkpoint();
        dvm_cancel_request(cancel);
    }
    check(!pthread_join(worker, NULL), "native I/O completion");
    int success = !strcmp(mode, "success") || !strcmp(mode, "race") || !strcmp(mode, "racefail");
    check((work.result.status == FVM_OK) == success && (work.result.flags & DVM_IO_PREFIX), "provider acknowledgement");
    if (success)
    {
        check(work.result.count == (writing ? 6U : 5U) && !(work.result.flags & DVM_IO_UNCERTAIN) &&
              work.result.stop == DVM_IO_NONE, "acknowledgement lost/replayed through wrappers");
        if (!writing)
        {
            check(!memcmp(work.bytes, "depth", 5), "read acknowledgement bytes");
        }
    }
    else
    {
        check(!work.result.count && work.result.stop == DVM_IO_CANCELLED &&
              !!(work.result.flags & DVM_IO_UNCERTAIN) == (strcmp(mode, "precancel") != 0),
              "cancelled provider cause/effects");
    }
    check(styxclientalive(client) == (strcmp(mode, "racefail") != 0), "provider cancellation closed unrelated connection work");
    puts("returned");
    puts("progress");
    fflush(stdout);
    checkpoint();
    dvm_cancel_request(cancel);
    dvm_cancel_release(cancel);
    dvm_vfs_close(&stream);
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
