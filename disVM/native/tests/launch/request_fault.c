/* Copyright (c) 2026 bhgv
 * EN: Construction failures and in-flight metadata cancellation against an independent 9P peer.
 * RU: Ошибки construction и отмена in-flight metadata против независимого 9P peer.
 */
#include "request.h"
#include "drivers/posix/fd.h"
#include "../../../runtime/vfs/cancel.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/* EN/RU: Fatal fixture invariant, no assert/build-mode dependence. / Явный fatal invariant. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "launch acquisition fault: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Separate native source owner waits only for external fixture admission, then cancels once.
 * Native source ждёт fixture admission и отменяет один раз, без guest execution. */
static void *cancelling(void *opaque)
{
    char byte;
    check(read(0, &byte, 1) == 1 && byte == 'c', "external metadata admission checkpoint");
    dvm_cancel_request(opaque);
    return NULL;
}


/* EN/RU: Fill bounded canonical association metadata, never used as credentials. / Canonical metadata, не credentials. */
static void identity(char *text, char value)
{
    memset(text, value, 42);
    text[42] = 'A';
    text[43] = 0;
}


/* EN/RU: Constructor owns one extra reference; failure must clear output and keep original ref valid.
 * Constructor владеет extra ref; failure очищает output и сохраняет original ref. */
int main(int argc, char **argv)
{
    check(argc == 3, "PTY descriptor and fault case");
    const char *mode = argv[2];
    StyxClient *client = styxclientmountphy(phyposixfd(atoi(argv[1]), PHY_STREAM|PHY_ORDERED|PHY_RELIABLE),
        256, "fixture", "", 2000, 4);
    check(client && !styxclientbidir(client), "ordinary cancellable client");
    DVM_LaunchSpec spec = {0};
    spec.policy = DVM_LAUNCH_TERMINAL_FIRST;
    spec.terminal_origin.role = DVM_ORIGIN_TERMINAL;
    identity(spec.terminal_origin.system, 'T');
    identity(spec.terminal_origin.export_id, 'E');
    identity(spec.terminal_origin.session, 'S');
    identity(spec.expected_provider, 'P');
    strcpy(spec.module, "module.dis");
    DVM_Cancel *cancel = NULL;
    DVM_CancelScope scope = {0};
    pthread_t worker;
    int cancelling_now = !strcmp(mode, "cancel") || !strcmp(mode, "cancelcheck");
    if (cancelling_now)
    {
        cancel = dvm_cancel_create();
        check(cancel && dvm_cancel_enter(&scope, cancel) &&
            !pthread_create(&worker, NULL, cancelling, cancel), "native acquisition cancel source");
    }
    DVM_LaunchRequest *request = NULL;
    check(styxclientretain(client) && !styxlaunchrequest(client, &spec, FVM_ACCESS_READ|FVM_ACCESS_EXEC, &request) &&
        !request, "failed acquisition published ticket/request");
    if (cancelling_now)
    {
        check(!pthread_join(worker, NULL) && dvm_cancel_requested(cancel) && dvm_cancel_leave(&scope),
            "native cancellation retirement");
        dvm_cancel_release(cancel);
    }
    int broken = !strcmp(mode, "loss") || !strcmp(mode, "badtag");
    check(styxclientalive(client) == !broken, "constructor failure misclassified link lifetime");
    styxclientfree(client);
    puts("PASS unpublished acquisition failure and native ownership");
    return 0;
}
