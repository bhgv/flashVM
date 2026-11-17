/* Copyright (c) 2026 bhgv
 * EN: Actual startup handoff import, constructor rollback and inherited channel lifetime.
 * RU: Настоящий startup handoff import, rollback конструктора и lifetime inherited каналов.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "fixtures/terminal-init/native.h"
#include "vfs/launch/runtime.h"
#include "vfs/sysevents.h"
#include "vfs/boot.h"
#include "owner_messages.h"
static int fail_context; /* One selected native channel-descriptor allocation refusal. */
static int fail_events; /* One selected unpublished namespace inbox refusal. */
static int fail_publication; /* One message endpoint registration refusal after adoption. */
static DVM_LaunchHandoff *publication_handoff; /* Borrowed through one synchronous publication attempt. */
static DVM_Namespace *adopting; /* Borrowed exclusively owned group during constructor tests. */
static int observing; /* Observe native staging lifetime during one refused constructor only. */
static void *staged_frame; /* Borrowed native scratch address, never a managed pointer. */
static void *staged_extent; /* Borrowed unpublished native stack allocation address. */
static int frame_freed; /* Matching scratch allocation reached real free. */
static int extent_freed; /* Matching unpublished stack allocation reached real free. */
void *__real_malloc(size_t size);
void __real_free(void *pointer);
void __real_newstack(Prog *process);
DVM_SysEvents *__real_dvm_sysevents_create(DVM_Namespace *group);
Prog *__real_dvm_runtime_launch_module(Module *module, DVM_LaunchHandoff *handoff);
DVM_MessageEndpoint *__real_dvm_messages_register(DVM_MessageRouter *router, uintptr_t id);


/* EN/RU: Refuse one process-publication endpoint after private environment adoption.
 * Отказать одному publication endpoint после private environment adoption. */
DVM_MessageEndpoint *__wrap_dvm_messages_register(DVM_MessageRouter *router, uintptr_t id)
{
    if (fail_publication && publication_handoff && !publication_handoff->group)
    {
        fail_publication = 0;
        return NULL;
    }
    return __real_dvm_messages_register(router, id);
}


/* EN/RU: Observe actual newstack input/output allocation addresses, without retaining process/frames.
 * Наблюдать actual newstack input/output addresses, не удерживая process/frames. */
void __wrap_newstack(Prog *process)
{
    Frame *frame;
    memmove(&frame, DVM_R.s, sizeof(frame));
    __real_newstack(process);
    if (observing)
    {
        staged_frame = frame;
        staged_extent = process->R.EX;
        frame_freed = 0;
        extent_freed = 0;
    }
}


/* EN/RU: Observe consuming native cleanup; allocator semantics and ownership remain unchanged.
 * Наблюдать consuming native cleanup; allocator semantics/ownership не меняются. */
void __wrap_free(void *pointer)
{
    if (observing && pointer)
    {
        if (pointer == staged_frame)
        {
            frame_freed = 1;
        }
        if (pointer == staged_extent)
        {
            extent_freed = 1;
        }
    }
    __real_free(pointer);
}


/* EN/RU: Refuse one context-sized allocation while fault injection is explicitly armed.
 * Отказать одной context-sized allocation только при включённой fault injection. */
void *__wrap_malloc(size_t size)
{
    if (fail_context && size == sizeof(DVM_TerminalChannels))
    {
        fail_context = 0;
        return NULL;
    }
    return __real_malloc(size);
}


/* EN/RU: Reject only the new handoff inbox, leaving ordinary process initialization intact.
 * Отказать только новому handoff inbox, не обычной process initialization. */
DVM_SysEvents *__wrap_dvm_sysevents_create(DVM_Namespace *group)
{
    if (fail_events && group == adopting)
    {
        fail_events = 0;
        return NULL;
    }
    return __real_dvm_sysevents_create(group);
}


/* EN/RU: Execute one expected constructor refusal through the real VM error boundary;1/0.
 * Module/handoff borrowed; failure must preserve handoff ownership and controller scratch.
 * Выполнить expected refusal через VM error boundary; module/handoff borrowed, return1/0. */
static int refused(Module *module, DVM_LaunchHandoff *handoff)
{
    observing = 1;
    staged_frame = NULL;
    staged_extent = NULL;
    frame_freed = 0;
    extent_freed = 0;
    if (waserror())
    {
        observing = 0;
        launch_check(staged_frame && staged_extent && frame_freed && extent_freed,
            "refused startup leaked frame or unpublished stack");
        return 1;
    }
    (void)__real_dvm_runtime_launch_module(module, handoff);
    poperror();
    observing = 0;
    return 0;
}


/* EN/RU: Explicitly grant only this test's local package directory for module loads.
 * New namespace is unpublished; root no-native-fallback remains enabled.
 * Явно выдать только test package dir для load; namespace private, implicit fallback запрещён. */
static void package(DVM_Namespace *group)
{
    DVM_ResourcePlan source;
    launch_check(dvm_resource_plan(dvm_namespace_boot_group(), ".", 0, &source), "package source selection");
    DVM_NamespaceBinding *binding = calloc(1, sizeof(*binding));
    launch_check(binding != NULL, "package binding allocation");
    binding->anchor = dvm_anchor_open(source.anchor, source.relative);
    launch_check(binding->anchor != NULL, "package anchor grant");
    strcpy(binding->source, source.names.native);
    strcpy(binding->on, "/pkg");
    binding->identity = binding->head_identity = ++group->next_binding;
    binding->next = group->bindings;
    group->bindings = binding;
    group->version++;
    dvm_resource_close(&source);
}


/* EN/RU: Supply prepared launch directly to real constructor before any guest code or publication.
 * Test rollback first, then prove one-use moves; native caller frees remaining handoff metadata.
 * Передать prepared launch constructor до guest code/publication; rollback и one-use moves. */
Prog *__wrap_dvm_runtime_launch_module(Module *module, DVM_LaunchHandoff *unused)
{
    (void)unused;
    terminal.terminal = 1;
    execution.value = 'P';
    DVM_LaunchOptions options = launch_options(&terminal, &execution);
    DVM_TerminalChannels channels = {0};
    if (CONTROL != 2)
    {
        channels.provider = dvm_provider_create_native(&ops, NULL, NULL, 1, DVM_PROVIDER_CANCEL_PATH);
        strcpy(channels.receive, CONTROL == 1 ? "ctl" : "rx");
        strcpy(channels.send, CONTROL == 1 ? "ctl" : "tx");
    }
    DVM_LaunchSpec spec = {0};
    spec.policy = options.policy;
    spec.terminal = options.terminal;
    spec.terminal_origin = options.terminal_origin;
    strcpy(spec.expected_provider, options.provider_origin.system);
    strcpy(spec.module, "imported-terminal-app");
    spec.flashvm = channels.provider ? &channels : NULL;
    DVM_LaunchRequest *request = NULL;
    DVM_LaunchTicket *ticket = dvm_launch_request_create(&spec, &request);
    launch_check(ticket && dvm_launch_request_prepare(request, options.provider, &options.provider_origin), "import prepare");
    DVM_LaunchHandoff *handoff = dvm_launch_request_take(request);
    launch_check(handoff != NULL, "import take");
    package(handoff->group);
    adopting = handoff->group;
    handoff->description.terminal_channels = !channels.provider;
    launch_check(refused(module, handoff) && handoff->group == adopting &&
        handoff->flashvm.provider == channels.provider, "invalid adoption consumed handoff");
    handoff->description.terminal_channels = !!channels.provider;
    if (channels.provider)
    {
        fail_context = 1;
        launch_check(refused(module, handoff) && !fail_context && handoff->group == adopting &&
            handoff->flashvm.provider == channels.provider, "context OOM consumed handoff");
    }
    fail_events = 1;
    launch_check(refused(module, handoff) && !fail_events && handoff->group == adopting &&
        handoff->flashvm.provider == channels.provider, "inbox OOM consumed handoff");
    DVM_LaunchHandoff *later = calloc(1, sizeof(*later));
    launch_check(later != NULL, "publication failure fixture allocation");
    later->description = handoff->description;
    later->provider_origin = handoff->provider_origin;
    later->group = dvm_namespace_copy(handoff->group);
    launch_check(later->group && dvm_provider_retain(channels.provider), "independent publication fixture grants");
    later->flashvm = channels;
    publication_handoff = later;
    fail_publication = 1;
    launch_check(refused(module, later), "publication failure did not reject constructor");
    launch_check(!fail_publication, "publication refusal was not reached");
    launch_check(!later->group && !later->flashvm.provider, "post-adoption failure restored moved ownership");
    publication_handoff = NULL;
    dvm_launch_handoff_free(later);
    Prog *process = __real_dvm_runtime_launch_module(module, handoff);
    launch_check(!handoff->group && !handoff->flashvm.provider && !dvm_launch_process_adopt(process, handoff),
        "import did not move owners once");
    dvm_launch_handoff_free(handoff);
    dvm_launch_ticket_free(ticket);
    dvm_launch_request_free(request);
    dvm_provider_release(channels.provider);
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    return process;
}


/* EN/RU: Real shutdown must drain process copies, managed grants and independently owned FDs.
 * Real shutdown обязан освободить process copies, managed grants и independent FD owners. */
void __wrap_dvm_namespace_boot_shutdown(void)
{
    __real_dvm_namespace_boot_shutdown();
    launch_check(opened == closed && retired == (CONTROL == 2 ? 0U : 1U) &&
        terminal.destroyed == 1 && execution.destroyed == 1, "imported process/root/FD lifetime leak");
    puts("PASS launch import constructor rollback and native lifetimes");
}
