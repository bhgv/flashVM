/* Copyright (c) 2026 bhgv
 * EN: Real Styx Tclunk/Tflush through the detached runtime close FIFO.
 * RU: Настоящие Styx Tclunk/Tflush через отделённую runtime close FIFO.
 */
#include "mount.h"
#include "export.h"
#include "cancel.h"
#include "close_worker.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; /* Protect copied identity observations. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Completed actual close predicate. */
static uintptr_t identities[3]; /* Copied integer identities, never dereferenced or retained as owners. */
static unsigned calls[3]; /* Exactly one consuming outer close per queued resource. */
static unsigned completed; /* Actual outer callbacks returned. */
void __real_dvm_vfs_close(DVM_VfsHandle *handle);


/* EN/RU: Enforce real protocol/ownership contracts in optimized builds.
 * Проверить настоящие protocol/ownership contracts и в optimized builds. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "native queued Styx close: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Await a driver-observed wire boundary; no guessed sleeps or guest participation.
 * Ждать driver-observed wire boundary без guessed sleeps или участия guest. */
static void checkpoint(void)
{
    char input[32];
    check(fgets(input, sizeof(input), stdin) != NULL, "driver checkpoint");
}


/* EN/RU: Observe only copied outer identities; forward genuine consuming close unchanged.
 * Наблюдать только outer identities; настоящий consuming close без изменений. */
void __wrap_dvm_vfs_close(DVM_VfsHandle *handle)
{
    int found = -1;
    pthread_mutex_lock(&lock);
    for (unsigned i = 0; i < 3; i++)
    {
        if (handle && identities[i] && (uintptr_t)handle->state == identities[i])
        {
            check(!calls[i]++, "duplicate outer close");
            check(dvm_cancel_current() != NULL, "worker did not establish native source scope");
            found = (int)i;
        }
    }
    pthread_mutex_unlock(&lock);
    __real_dvm_vfs_close(handle);
    if (found >= 0)
    {
        pthread_mutex_lock(&lock);
        completed++;
        pthread_cond_broadcast(&changed);
        pthread_mutex_unlock(&lock);
    }
}


/* EN/RU: Original-server file/view-directory/view-file handles are queued and retired once.
 * Main stops the real closer only after an observed Tclunk boundary, or actual success.
 * File/view-directory/view-file исходного сервера закрываются один раз; main
 * останавливает настоящий closer после observed Tclunk либо actual completion. */
int main(int argc, char **argv)
{
    check(argc == 4, "port/operation/mode");
    const char *mode = argv[3];
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons((unsigned short)atoi(argv[1]));
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    check(fd >= 0 && !connect(fd, (struct sockaddr *)&address, sizeof(address)), "connect");
    /* Leave native mux capacity for independent best-effort clunks and the post-stop caller. */
    StyxClient *client = styxclientmountfd(fd, 1024, "fixture", "", !strcmp(mode, "racefail") ? 300 : 3000, 8);
    check(client && styxclientretain(client), "client");
    DVM_Provider *source = styxmountprovider(client, 7);
    DVM_View *view = dvm_view_create(NULL, NULL, NULL);
    check(source && view && dvm_view_map(view, "/", source, ".", 7, DVM_VIEW_TREE), "prepared view");
    DVM_Provider *prepared = dvm_view_freeze(view);
    DVM_VfsHandle handles[3] = {{0}};
    check(prepared && dvm_provider_open(source, "data", "r", &handles[0]) == FVM_OK &&
          dvm_provider_open(prepared, ".", "r", &handles[1]) == FVM_OK &&
          dvm_provider_open(prepared, "data", "r", &handles[2]) == FVM_OK, "opened resources");
    DVM_CloseJob *jobs[3];
    for (unsigned i = 0; i < 3; i++)
    {
        jobs[i] = dvm_close_prepare();
        identities[i] = (uintptr_t)handles[i].state;
        check(jobs[i] && dvm_vfs_close_cancellable(&handles[i]), "close eligibility/reservation");
    }
    check(dvm_close_worker_start(), "runtime close worker");
    puts("ready");
    fflush(stdout);
    checkpoint();
    for (unsigned i = 0; i < 3; i++)
    {
        dvm_close_submit_resource(jobs[i], &handles[i]);
        check(!handles[i].ops && !handles[i].state && !handles[i].close_ready, "exclusive transfer");
    }
    if (!strcmp(mode, "success"))
    {
        pthread_mutex_lock(&lock);
        while (completed != 3)
        {
            pthread_cond_wait(&changed, &lock);
        }
        pthread_mutex_unlock(&lock);
    }
    else
    {
        checkpoint();
    }
    dvm_close_worker_stop();
    check(completed == 3 && calls[0] == 1 && calls[1] == 1 && calls[2] == 1 &&
          !dvm_cancel_current(), "close drain/current source ownership");
    check(styxclientalive(client) == (strcmp(mode, "racefail") != 0), "shutdown close broke unrelated transport");
    if (strcmp(mode, "racefail"))
    {
        FVM_FileInfo meta;
        check(dvm_provider_info(prepared, "data", &meta) == FVM_OK && meta.identity.low == 99,
              "service cancellation leaked into an unrelated native caller");
    }
    dvm_close_statistics();
    puts("returned");
    puts("progress");
    fflush(stdout);
    checkpoint();
    dvm_provider_release(prepared);
    dvm_provider_release(source);
    styxclientshutdown(client);
    styxclientfree(client);
    printf("PASS native control cancellation %s %s\n", argv[2], mode);
    return 0;
}
