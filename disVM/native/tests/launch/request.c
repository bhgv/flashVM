/* Copyright (c) 2026 bhgv
 * EN: Real remote-root launch tickets, private copied payloads and one-use provider handoff.
 * RU: Реальные remote-root launch tickets, private copied payloads и one-use provider handoff.
 */
#include "request.h"
#include "../../../runtime/vfs/cancel.h"
#include "../../../runtime/vfs/stack.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#ifdef STYX_TERMINAL_RECORD_TEST


/* EN/RU: Borrow coherent bidir client/descriptor for general record transport acceptance.
 * Borrow coherent client/descriptor до конца general record test. */
extern void terminalprobe(StyxClient *, const DVM_TerminalChannels *);
#endif


/* EN/RU: Fatal fixture invariant independent of build profile, diagnostic borrowed. / Явный fatal invariant. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "Styx launch request: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Read remote bytes from owned handoff namespace after ticket/request wrappers retire.
 * Прочитать remote bytes из handoff namespace после retirement ticket/request wrappers. */
static void readhandoff(DVM_LaunchHandoff *handoff, const char *path, const char *expected)
{
    DVM_ResourcePlan plan;
    DVM_VfsHandle handle = {0};
    FVM_FileInfo attributes;
    char bytes[32];
    check(dvm_resource_plan(handoff->group, path, 0, &plan) && dvm_resource_info(&plan, &attributes) == FVM_OK &&
        plan.origin.role == DVM_ORIGIN_TERMINAL &&
        dvm_provider_open(plan.provider, plan.relative, "r", &handle) == FVM_OK, "handoff keeps live remote terminal");
    DVM_IoResult result = dvm_vfs_read_chunk(&handle, bytes, sizeof(bytes));
    check(result.status == FVM_OK && result.count == strlen(expected) &&
        !memcmp(bytes, expected, result.count), "handoff live bytes");
    dvm_vfs_close(&handle);
    dvm_resource_close(&plan);
}


/* EN/RU: Pinned FlashVM channels remain remote after changing the application's union order.
 * FlashVM channels остаются remote после изменения application union order. */
static void terminalhandoff(DVM_LaunchHandoff *handoff, const char *path)
{
    check(handoff->description.terminal_channels && handoff->flashvm.provider &&
        !strcmp(handoff->flashvm.receive, "terminal") && !strcmp(handoff->flashvm.send, "terminal"),
        "immutable bidir terminal descriptor");
    DVM_ResourcePlan plan;
    check(dvm_resource_plan(handoff->group, path, 0, &plan) && plan.layers &&
        dvm_stack_move(&plan, plan.names.version, plan.layer_id, -1) == 1, "application VFS reorder");
    dvm_resource_close(&plan);
    DVM_VfsHandle receive = {0};
    DVM_VfsHandle send = {0};
    check(dvm_terminal_channel_open(&handoff->flashvm, handoff->flashvm.receive, "r", &receive) == FVM_OK &&
        dvm_terminal_channel_open(&handoff->flashvm, handoff->flashvm.send, "w", &send) == FVM_OK,
        "bidir terminal stream pair opens on pinned root");
    char bytes[16];
    DVM_IoResult input = dvm_vfs_read_chunk(&receive, bytes, sizeof(bytes));
    DVM_IoResult output = dvm_vfs_write_chunk(&send, "output", 6);
    check(input.status == FVM_OK && input.count == 5 && !memcmp(bytes, "input", 5) &&
        output.status == FVM_OK && output.count == 6, "actual terminal input/output over bidir 9P");
    dvm_vfs_close(&receive);
    dvm_vfs_close(&send);
}


/* EN/RU: Pass an owned extra ref to constructor refusal; caller's original session must survive.
 * Передать extra ref для constructor refusal, original session должна выжить. */
static void refused(StyxClient *client, const DVM_LaunchSpec *spec, unsigned access)
{
    DVM_LaunchRequest *request = NULL;
    check(styxclientretain(client) && !styxlaunchrequest(client, spec, access, &request) &&
        !request && styxclientalive(client), "invalid request publication or caller ref consumption");
}


/* EN/RU: Prepare a copied spec from borrowed options/argument bytes, no provider root included.
 * Подготовить copied spec из borrowed options/bytes, без provider root. */
static DVM_LaunchSpec spec(const DVM_LaunchOptions *options, unsigned char *bytes)
{
    DVM_LaunchSpec input = {0};
    input.policy = options->policy;
    input.terminal_origin = options->terminal_origin;
    strcpy(input.expected_provider, options->provider_origin.system);
    strcpy(input.module, "provider/module.dis");
    input.arguments = bytes;
    input.argument_bytes = 4;
    return input;
}


/* EN/RU: Exercise local malformed fields, observed pre-cancel and missing output before acquisition.
 * Проверить malformed fields, pre-cancel и missing output до acquisition. */
static void invalid(StyxClient *client, const DVM_LaunchOptions *options)
{
    unsigned char bytes[] = {0, 1, 128, 255};
    unsigned access = FVM_ACCESS_READ|FVM_ACCESS_EXEC;
    DVM_LaunchSpec input = spec(options, bytes);
    input.terminal = options->provider;
    refused(client, &input, access);
    input.terminal = NULL;
    input.policy = 0;
    refused(client, &input, access);
    input.policy = options->policy;
    input.expected_provider[0] = '!';
    refused(client, &input, access);
    input = spec(options, bytes);
    input.module[0] = 0;
    refused(client, &input, access);
    memset(input.module, 'X', sizeof(input.module));
    refused(client, &input, access);
    input = spec(options, bytes);
    input.argument_bytes = DVM_LAUNCH_ARGUMENT_LIMIT+1;
    refused(client, &input, access);
    input.argument_bytes = 4;
    input.arguments = NULL;
    refused(client, &input, access);
    input = spec(options, bytes);
    refused(client, &input, FVM_ACCESS_READ);
    DVM_TerminalChannels channels = {0};
    input.flashvm = &channels;
    refused(client, &input, access);
    input.flashvm = NULL;
    StyxTerminalPaths paths = {"../escape", "terminal", NULL};
    DVM_LaunchRequest *output = NULL;
    check(styxclientretain(client) && !styxbidirlaunchrequest(client, &input, FVM_ACCESS_MASK, &paths, &output) &&
        !output, "invalid FlashVM endpoint paths refuse before acquisition");
    paths.receive = "terminal";
    check(styxclientretain(client) && !styxbidirlaunchrequest(client, &input, access, &paths, &output) &&
        !output, "FlashVM needs write ceiling and bidir");
    if (!styxclientbidir(client))
    {
        check(styxclientretain(client) && !styxbidirlaunchrequest(client, &input, FVM_ACCESS_MASK, &paths, &output) &&
            !output && styxclientalive(client), "ordinary session cannot carry a FlashVM terminal grant");
    }
    check(styxclientretain(client) && !styxlaunchrequest(client, &input, access, NULL), "missing output consumes extra ref");
    DVM_Cancel *cancel = dvm_cancel_create();
    DVM_CancelScope scope = {0};
    check(cancel && dvm_cancel_enter(&scope, cancel), "constructor cancel scope");
    dvm_cancel_request(cancel);
    refused(client, &input, access);
    check(dvm_cancel_leave(&scope), "constructor scope retirement");
    dvm_cancel_release(cancel);
}


/* EN/RU: Native take caller borrows request through joined race; owns winner's result.
 * Native take caller заимствует request до join; владеет winner result. */
typedef struct Taker
{
    DVM_LaunchRequest *request; /* Borrowed stable wrapper through join. */
    DVM_LaunchHandoff *handoff; /* Worker-only result until join. */
} Taker;


/* EN/RU: Race one-use take on native OS thread, never guest/managed scheduling. / Native one-use take. */
static void *taking(void *state)
{
    Taker *work = state;
    work->handoff = dvm_launch_request_take(work->request);
    return NULL;
}


/* EN/RU: One lifecycle variant; retained roots/observers must retire or transfer independently.
 * Один lifecycle variant; roots/observers завершаются либо передаются независимо. */
static void lifecycle(StyxClient *client, const DVM_LaunchOptions *options,
    const char *path, const char *expected, unsigned kind)
{
    unsigned char bytes[] = {0, 1, 128, 255};
    DVM_LaunchSpec input = spec(options, bytes);
    DVM_LaunchRequest *request = NULL;
    check(styxclientretain(client), "constructor client pin");
    char receive[] = "terminal";
    char send[] = "terminal";
    StyxTerminalPaths paths = {receive, send, NULL};
    DVM_LaunchTicket *ticket = kind == 4 ?
        styxbidirlaunchrequest(client, &input, FVM_ACCESS_MASK, &paths, &request) :
        styxlaunchrequest(client, &input, FVM_ACCESS_READ|FVM_ACCESS_EXEC, &request);
    check(ticket && request && dvm_launch_ticket_status(ticket) == DVM_LAUNCH_PENDING && !input.terminal,
        "separate requester/provider capabilities");
    receive[0] = send[0] = 'X';
    bytes[0] = 99;
    input.module[0] = 'X';
    DVM_LaunchDescription description;
    unsigned char copied[4];
    size_t count = 0;
    check(dvm_launch_describe(request, &description) && !strcmp(description.module, "provider/module.dis") &&
        dvm_launch_arguments(request, 0, copied, sizeof(copied), &count) && count == 4 &&
        copied[0] == 0 && copied[3] == 255, "description and arguments copied before publication");
    DVM_Changes *journal = dvm_launch_observe(ticket);
    check(journal != NULL, "independent status notice source");
    DVM_Origin wrong = options->provider_origin;
    wrong.system[0] = 'X';
    check(!dvm_launch_request_prepare(request, options->provider, &wrong) &&
        dvm_launch_ticket_status(ticket) == DVM_LAUNCH_PENDING, "wrong execution installation remains pending");
    DVM_LaunchHandoff *handoff = NULL;
    if (kind == 1)
    {
        check(dvm_launch_ticket_cancel(ticket) && !dvm_launch_request_prepare(request, options->provider,
            &options->provider_origin) && !dvm_launch_request_take(request), "pending cancellation blocks prepare/take");
    }
    else if (kind == 2)
    {
        check(dvm_launch_request_refuse(request) && !dvm_launch_request_take(request) &&
            dvm_launch_ticket_status(ticket) == DVM_LAUNCH_DENIED, "provider refusal");
    }
    else
    {
        check(dvm_launch_request_prepare(request, options->provider, &options->provider_origin) &&
            dvm_launch_ticket_status(ticket) == DVM_LAUNCH_READY, "provider prepares actual remote-root environment");
        if (kind == 3)
        {
            check(dvm_launch_ticket_cancel(ticket) && !dvm_launch_request_take(request), "ready cancellation retires environment");
        }
        else
        {
            Taker takers[2] = {{request, NULL}, {request, NULL}};
            pthread_t threads[2];
            check(!pthread_create(&threads[0], NULL, taking, &takers[0]) &&
                !pthread_create(&threads[1], NULL, taking, &takers[1]), "native take contenders");
            check(!pthread_join(threads[0], NULL) && !pthread_join(threads[1], NULL), "take contenders retired");
            check((takers[0].handoff != NULL) != (takers[1].handoff != NULL), "exactly one provider handoff");
            handoff = takers[0].handoff ? takers[0].handoff : takers[1].handoff;
            check(dvm_launch_ticket_status(ticket) == DVM_LAUNCH_TAKEN && !dvm_launch_ticket_cancel(ticket) &&
                !dvm_launch_request_take(request), "taken environment cannot be retaken or revoked");
        }
    }
    dvm_launch_request_free(request);
    dvm_launch_ticket_free(ticket);
    if (handoff)
    {
        check(handoff->arguments[0] == 0 && handoff->arguments[3] == 255, "handoff immutable copied payload");
        readhandoff(handoff, path, expected);
        if (kind == 4)
        {
            terminalhandoff(handoff, path);
#ifdef STYX_TERMINAL_RECORD_TEST
            terminalprobe(client, &handoff->flashvm);
#endif
        }
        dvm_launch_handoff_free(handoff);
    }
    DVM_ChangeCursor cursor = {0};
    DVM_ChangeBatch batch;
    check(dvm_changes_pending(journal, &cursor, DVM_CHANGE_ALL, &batch) > 0,
        "status journal survives both wrappers");
    dvm_changes_destroy(journal);
    check(styxclientalive(client), "ticket lifecycle preserves caller-held session");
}


/* EN/RU: Borrow all inputs; real remote-root constructor and prepare/take are exercised before events.
 * Borrow inputs; real remote-root constructor и prepare/take проверены до event tests. */
void launchrequests(StyxClient *client, const DVM_LaunchOptions *options, const char *path, const char *expected)
{
    invalid(client, options);
    for (unsigned kind = 0; kind < (styxclientbidir(client) ? 5U : 4U); kind++)
    {
        lifecycle(client, options, path, expected, kind);
    }
}
