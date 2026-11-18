/* Copyright (c) 2026 bhgv
 * EN: Real pre-init ordered launch handoff, ordinary VFS record files and dedicated guest execution.
 * RU: Настоящий ordered launch handoff до init, обычные VFS record-файлы и dedicated guest execution.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runqueue.h"
#include "vfs/launch/runtime.h"
#include "vfs/export.h"
#include "../native/terminal/order/broker/attach.h"
#include "../native/terminal/order/files/files.h"
#include "../native/launch/request.h"
#include "../native/tests/launch/backend.h"
static LaunchState terminal; /* Unused local fixture root, disposed before remote preparation. */
static LaunchState execution; /* Live execution-provider fixture through all guest namespaces. */
static StyxClient *connection; /* Original native session, independent of broker owners. */
static StyxOrderedFiles *files; /* Explicit lifecycle owner after namespace/FD disposal. */


/* EN/RU: Original fixture client, owned until explicit native cleanup, no guest or simulated protocol.
 * Исходный fixture client, owned до явной native-очистки, без guest/имитации протокола.
 */
extern StyxClient *linkfixture(void);


/* EN/RU: Join actual original server and physical owners after all client refs retire.
 * Завершить настоящий исходный server и physical owners после client refs.
 */
extern void linkfixturefree(void);


/* EN/RU: Original runtime startup, module borrowed; moves exclusive handoff before first guest publication.
 * Исходный runtime startup, module borrowed; перенос exclusive handoff до первой публикации guest.
 */
extern Prog *__real_dvm_runtime_launch_module(Module *, DVM_LaunchHandoff *);


/* EN/RU: Original final namespace shutdown after all guest execution/FD owners retire.
 * Исходный final namespace shutdown после всех guest execution/FD owners.
 */
extern void __real_dvm_namespace_boot_shutdown(void);


/* EN/RU: Map one owned filtered file into a default-deny application view, then release caller factory ref.
 * Добавить owned filtered file в default-deny application view, освободить исходный factory-ref.
 */
static void mapfile(DVM_View *view, const char *path, unsigned long tag, unsigned access)
{
    DVM_Provider *provider = styxorderedfile(files, tag, access); /* Ordinary retained single-file capability. */
    launch_check(provider && dvm_view_map(view, path, provider, ".", access, 0), "map filtered Unibus record file");
    dvm_provider_release(provider);
}


/* EN/RU: Prepare actual remote-root ticket, take once, construct ordered broker, install file grants and invoke real startup.
 * Подготовить настоящий remote-root ticket, одноразовый take, ordered broker и file grants, вызвать настоящий startup.
 */
Prog *__wrap_dvm_runtime_launch_module(Module *module, DVM_LaunchHandoff *unused)
{
    (void)unused;
    connection = linkfixture();
    launch_check(connection != NULL, "real original server/client fixture");
    DVM_LaunchOptions options = launch_options(&terminal, &execution); /* Real private live-root preparation. */
    dvm_provider_release(options.terminal);
    DVM_LaunchSpec spec = {0}; /* Captured terminal channels and execution source are independently authorized. */
    spec.policy = options.policy;
    spec.terminal_origin = options.terminal_origin;
    strcpy(spec.expected_provider, options.provider_origin.system);
    strcpy(spec.module, "provider/unibus-files.dis");
    StyxTerminalPaths paths = {"records", "records", NULL}; /* Existing granted original-Styx endpoints. */
    DVM_LaunchRequest *request = NULL; /* Exclusive provider-side capability. */
    launch_check(styxclientretain(connection), "retained requester client");
    DVM_LaunchTicket *ticket = styxbidirlaunchrequest(connection, &spec, FVM_ACCESS_MASK, &paths, &request);
    launch_check(ticket && request && dvm_launch_request_prepare(request, options.provider, &options.provider_origin), "actual remote request prepare");
    DVM_LaunchHandoff *handoff = dvm_launch_request_take(request); /* Owned actual one-use environment. */
    unsigned long tags[] = {0, 7, 9}; /* Authorized application inboxes only. */
    StyxOrderedOptions ordered = {sizeof(ordered), 128, 16, 71, tags, 3}; /* Same profile as native API acceptance. */
    StyxOrdered *peer = styxbidirorderedlaunch(connection, handoff, &ordered); /* Explicit native connection, no default CLI rewrite. */
    files = styxorderedfiles(peer);
    launch_check(files && !styxorderedstate(peer), "native file-set owner");
    DVM_View *view = dvm_view_create(NULL, NULL, NULL); /* Default deny: raw carrier nodes never enter the guest namespace. */
    launch_check(view != NULL, "filtered application view");
    mapfile(view, "/seven", 7, FVM_ACCESS_READ | FVM_ACCESS_WRITE);
    mapfile(view, "/nine", 9, FVM_ACCESS_READ);
    mapfile(view, "/zero", 0, FVM_ACCESS_READ | FVM_ACCESS_WRITE);
    options.terminal = dvm_view_freeze(view); /* Ordinary live view, no snapshot of remote files or records. */
    DVM_Namespace *filtered = dvm_launch_prepare(&options); /* Native owner chooses the guest-visible grant before init. */
    launch_check(filtered != NULL, "prepare filtered application namespace");
    dvm_namespace_release(handoff->group);
    handoff->group = filtered;
    dvm_provider_release(options.terminal);
    Prog *process = __real_dvm_runtime_launch_module(module, handoff); /* Production adoption and dedicated owner publication. */
    launch_check(process && !handoff->group && !handoff->flashvm.provider, "real pre-init ownership transfer");
    dvm_launch_handoff_free(handoff);
    dvm_launch_ticket_free(ticket);
    dvm_launch_request_free(request);
    dvm_provider_release(options.provider);
    return process;
}


/* EN/RU: Require actual guest namespaces and FDs to release exports before joining native broker/physical owners.
 * Требовать освобождения exports настоящими guest namespaces/FD до join native broker/physical owners.
 */
void __wrap_dvm_namespace_boot_shutdown(void)
{
    __real_dvm_namespace_boot_shutdown();
    launch_check(execution.destroyed == 1 && terminal.destroyed == 1 && styxorderedfilesfree(files), "guest FD/provider lifecycle drain");
    styxclientshutdown(connection);
    styxclientfree(connection);
    linkfixturefree();
    puts("PASS real Dis Unibus file native ownership drain");
}
