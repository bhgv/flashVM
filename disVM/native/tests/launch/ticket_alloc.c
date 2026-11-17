/* Copyright (c) 2026 bhgv
 * EN: Constructor cleanup and handoff allocation refusal preserve request ownership and READY.
 * RU: Cleanup конструктора и allocation refusal handoff сохраняют ownership и READY.
 */
#include "backend.h"
#include "../../../runtime/vfs/launch/service.h"
static void *live[4096]; /* Native test-owned allocation index, no allocation while tracking. */
static int accounting; /* Only ticket/preparation/handoff allocations are recorded. */
static unsigned allocation; /* Count only allocations during the selected controlled call. */
static unsigned fail_at; /* One-based refusal position, zero disables injection. */
void *__real_malloc(size_t bytes);
void *__real_calloc(size_t count, size_t bytes);
void __real_free(void *pointer);


/* EN/RU: Record actual successful native allocation without allocating observer state.
 * Pointer ownership is unchanged; tracked objects must later reach wrapped free.
 * Учесть успешную native allocation без allocation observer state; ownership не меняется. */
static void *record(void *pointer)
{
    if (pointer && accounting)
    {
        for (unsigned i = 0; i < sizeof(live)/sizeof(live[0]); i++)
        {
            if (!live[i])
            {
                live[i] = pointer;
                return pointer;
            }
        }
        launch_check(0, "allocation observer capacity exhausted");
    }
    return pointer;
}


/* EN/RU: Retire a recorded native pointer before real free; unrelated fixture allocations are ignored.
 * Убрать tracked pointer до real free; unrelated fixture allocations не учитываются. */
void __wrap_free(void *pointer)
{
    if (pointer)
    {
        for (unsigned i = 0; i < sizeof(live)/sizeof(live[0]); i++)
        {
            if (live[i] == pointer)
            {
                live[i] = NULL;
                break;
            }
        }
    }
    __real_free(pointer);
}


/* EN/RU: Refuse one selected allocation without changing other native allocator behaviour.
 * Отказать одной выбранной allocation, остальной native allocator не менять. */
void *__wrap_malloc(size_t bytes)
{
    if (fail_at && ++allocation == fail_at)
    {
        return NULL;
    }
    return record(__real_malloc(bytes));
}


/* EN/RU: Apply the same selected refusal to zeroed constructor/handoff storage.
 * Применить selected refusal к zeroed constructor/handoff storage. */
void *__wrap_calloc(size_t count, size_t bytes)
{
    if (fail_at && ++allocation == fail_at)
    {
        return NULL;
    }
    return record(__real_calloc(count, bytes));
}


/* EN/RU: Inject service construction/submission refusal and require exact quota restoration.
 * Service objects and slots remain inside the allocation/free observer interval.
 * Проверить allocation refusal service/submit и точный возврат quota, учитывая все objects/slots. */
static void service_failures(DVM_LaunchOptions *options, DVM_LaunchSpec *spec)
{
    for (unsigned position = 1; position <= 20; position++)
    {
        allocation = 0;
        fail_at = position;
        DVM_LaunchService *owner = dvm_launch_service_create(options->provider_origin.system, 1);
        fail_at = 0;
        dvm_launch_service_free(owner);
    }
    DVM_LaunchService *owner = dvm_launch_service_create(options->provider_origin.system, 1);
    DVM_LaunchEndpoint *endpoint = dvm_launch_service_endpoint(owner);
    launch_check(owner && endpoint, "service allocation fixture");
    for (unsigned position = 1; position <= 40; position++)
    {
        allocation = 0;
        fail_at = position;
        DVM_LaunchTicket *ticket = dvm_launch_service_submit(endpoint, spec);
        fail_at = 0;
        if (ticket)
        {
            DVM_LaunchReview *review = dvm_launch_service_next(owner);
            launch_check(review != NULL, "submitted request lost review");
            dvm_launch_ticket_free(ticket);
            dvm_launch_review_free(review);
        }
        DVM_LaunchServiceInfo info;
        launch_check(dvm_launch_service_info(owner, &info) && !info.outstanding && !info.queued,
                     "failed submission leaked quota or queue entry");
    }
    dvm_launch_endpoint_free(endpoint);
    dvm_launch_service_free(owner);
}


/* EN/RU: Fail each constructor allocation, then each handoff allocation, preserving transferable READY.
 * Проверить constructor/handoff allocations, сохранив transferable READY после отказа take. */
int main(void)
{
    LaunchState terminal = {0}, provider = {0};
    terminal.terminal = 1;
    DVM_LaunchOptions options = launch_options(&terminal, &provider);
    DVM_LaunchSpec spec = {0};
    spec.policy = options.policy;
    spec.terminal = options.terminal;
    spec.terminal_origin = options.terminal_origin;
    strcpy(spec.expected_provider, options.provider_origin.system);
    strcpy(spec.module, "entry");
    spec.arguments = "args";
    spec.argument_bytes = 4;
    DVM_TerminalChannels channels = {0};
    channels.provider = options.terminal;
    strcpy(channels.receive, ".");
    strcpy(channels.send, ".");
    spec.flashvm = &channels;
    accounting = 1;
    unsigned failures = 0;
    for (unsigned position = 1; position <= 32; position++)
    {
        allocation = 0;
        fail_at = position;
        DVM_LaunchRequest *request = NULL;
        DVM_LaunchTicket *ticket = dvm_launch_request_create(&spec, &request);
        fail_at = 0;
        if (!ticket)
        {
            failures++;
            launch_check(!request, "failed constructor published provider authority");
        }
        else
        {
            launch_check(request != NULL, "partial constructor success");
            dvm_launch_ticket_free(ticket);
            dvm_launch_request_free(request);
        }
    }
    launch_check(failures >= 8, "constructor refusal coverage missing");
    DVM_Cancel *parent = dvm_cancel_create();
    launch_check(parent != NULL, "outer source for preparation refusal");
    unsigned prepare_failures = 0;
    for (unsigned position = 1; position <= 20; position++)
    {
        DVM_LaunchRequest *review = NULL;
        DVM_LaunchTicket *pending = dvm_launch_request_create(&spec, &review);
        DVM_CancelScope outer = {0};
        launch_check(pending && dvm_cancel_enter(&outer, parent), "outer refusal scope");
        allocation = 0;
        fail_at = position;
        int ready = dvm_launch_request_prepare(review, options.provider, &options.provider_origin);
        fail_at = 0;
        launch_check(dvm_cancel_current() == parent && !dvm_cancel_requested(parent) &&
            dvm_cancel_leave(&outer) && !dvm_cancel_current(), "preparation refusal lost outer source");
        if (!ready)
        {
            prepare_failures++;
            launch_check(dvm_launch_ticket_status(pending) == DVM_LAUNCH_DENIED &&
                !dvm_launch_request_take(review), "failed preparation published environment");
        }
        dvm_launch_ticket_free(pending);
        dvm_launch_request_free(review);
    }
    launch_check(prepare_failures >= 6, "preparation allocation refusal coverage missing");
    dvm_cancel_release(parent);
    DVM_LaunchRequest *request = NULL;
    DVM_LaunchTicket *ticket = dvm_launch_request_create(&spec, &request);
    launch_check(ticket && dvm_launch_request_prepare(request, options.provider, &options.provider_origin), "prepare for take refusal");
    for (unsigned position = 1; position <= 2; position++)
    {
        allocation = 0;
        fail_at = position;
        DVM_LaunchHandoff *handoff = dvm_launch_request_take(request);
        fail_at = 0;
        launch_check(!handoff && dvm_launch_ticket_status(ticket) == DVM_LAUNCH_READY, "failed take consumed environment");
    }
    DVM_LaunchHandoff *handoff = dvm_launch_request_take(request);
    launch_check(handoff && dvm_launch_ticket_status(ticket) == DVM_LAUNCH_TAKEN &&
        !memcmp(handoff->arguments, "args", 4), "ready did not survive refusal");
    dvm_launch_ticket_free(ticket);
    dvm_launch_request_free(request);
    dvm_launch_handoff_free(handoff);
    service_failures(&options, &spec);
    for (unsigned i = 0; i < sizeof(live)/sizeof(live[0]); i++)
    {
        launch_check(!live[i], "ticket/preparation/handoff allocation leaked");
    }
    accounting = 0;
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    launch_check(terminal.destroyed == 1 && provider.destroyed == 1, "allocation refusal leaked root owners");
    puts("PASS launch constructor rollback and handoff allocation refusal");
    return 0;
}
