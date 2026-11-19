/* Copyright (c) 2026 bhgv
 * EN: Real dedicated guest loads applications/dependencies from two live granted image providers before any host fallback.
 * RU: Настоящий dedicated guest загружает приложения/зависимости из двух живых image-providers без host fallback.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runqueue.h"
#include "vfs/launch/runtime.h"
#include "vfs/launch/service.h"
#include "vfs/launch/route.h"
#include "vfs/resource.h"
#include "vfs/export.h"
#include "../../../native/tests/launch/backend.h"
#include "images.h"
#include "vfs/loader.h"
#include "contracts.h"
#include "environment.h"
#include "binding.h"
#include "vfs/launch/context.h"
#include "module_interface/launch.h"
#define CLIENT_OWNER (CONTROL == 17 || CONTROL == 19 || CONTROL == 21) /* Actual terminal-source owner scenarios, including changed caller bind. */
static ImageSource terminal_images; /* Native fixture bytes live through every guest/closer owner. */
static __thread int route_selecting; /* Startup controller explicitly selects caller side before any guest exists. */
static unsigned provider_lookups; /* Atomic observed caller-side load selections, including initial controller. */
static unsigned terminal_lookups; /* Atomic observed accepted client-caller selections. */
static ImageSource execution_images; /* Actual provider-first code source. */
static unsigned checked_targets; /* Actual named-target checks on ACTIVE parent/child owners. */
static unsigned refused_targets; /* Actual guard refusals before ordinary linking, not inferred from a nil load. */
static __thread unsigned contract_allocation; /* Test-only countdown for actual adapter snapshot calloc refusal. */


/* EN/RU: Original calloc returns owned zeroed count*size bytes/NULL without test recursion.
 * Исходный calloc возвращает свой zeroed count*size storage/NULL, без test recursion.
 */
extern void *__real_calloc(size_t count, size_t size);


/* EN/RU: Fail only the selected target-contract snapshot allocation on this permanent owner; forward all other allocations.
 * Отказать только выбранной target-contract snapshot allocation этого постоянного owner; остальные переслать.
 */
void *__wrap_calloc(size_t count, size_t size)
{
    if (contract_allocation && --contract_allocation == 0)
    {
        return NULL;
    }
    if (binding_allocation && --binding_allocation == 0)
    {
        return NULL;
    }
    return __real_calloc(count, size);
}


/* [GC:] EN/RU: Original guard consumes the owned Module and returns the same owner/NULL without replacement parsing.
 * Исходный guard поглощает owned Module и возвращает тот же owner/NULL, без replacement parsing.
 */
extern Module *__real_dvm_launch_module_checked(const char *, Module *);


/* [GC:] EN/RU: Observe actual inherited-contract checks; inject only native snapshot OOM, never fabricate a Module/refusal.
 * Наблюдать actual inherited-contract checks; вводить лишь native snapshot OOM, не подменять Module/refusal.
 */
Module *__wrap_dvm_launch_module_checked(const char *path, Module *module)
{
    Prog *process = currun();
    DVM_LaunchContext *context = process ? ((DVM_RuntimeProcess *)process)->launch_context : NULL;
    int target = module && module != H && context && context->contract && path && !strcmp(path, context->module);
    if (target)
    {
        __atomic_add_fetch(&checked_targets, 1, __ATOMIC_RELAXED);
        if (CONTROL == 22 || CONTROL == 23)
        {
            contract_allocation = CONTROL - 21;
        }
    }
    Module *answer = __real_dvm_launch_module_checked(path, module);
    if (target && !answer)
    {
        __atomic_add_fetch(&refused_targets, 1, __ATOMIC_RELAXED);
    }
    launch_check(!contract_allocation, "target contract allocation refusal was not reached");
    return answer;
}



/* EN/RU: Original startup borrows module and moves exclusive handoff before process publication.
 * Исходный startup заимствует module и переносит exclusive handoff до публикации process.
 */
extern Prog *__real_dvm_runtime_launch_module(Module *, DVM_LaunchHandoff *);


/* EN/RU: Original final namespace shutdown, all guest FD/module operations already retired.
 * Исходный final namespace shutdown, все guest FD/module operations уже завершены.
 */
extern void __real_dvm_namespace_boot_shutdown(void);


/* EN/RU: Original private pre-publication adoption, no guest instructions run during this call.
 * Исходный private adoption до publication, guest instructions во время вызова не исполняются.
 */
extern int __real_dvm_launch_process_adopt(Prog *, DVM_LaunchHandoff *);


/* EN/RU: Variant13 retains real VFS grants but clears only the native execution role before any resident publication.
 * It proves a terminal context cannot turn mounted code into implicit local execution; no Module result is mocked.
 * Variant13 сохраняет настоящие VFS grants, но убирает native execution-role до публикации resident.
 * Проверяет запрет неявного исполнения mounted-code терминалом, не подменяя Module result.
 */
int __wrap_dvm_launch_process_adopt(Prog *process, DVM_LaunchHandoff *handoff)
{
    int result = __real_dvm_launch_process_adopt(process, handoff); /* All normal preparation/ownership remains actual. */
    if (CONTROL == 13 && result)
    {
        DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)process; /* Still exclusive unpublished constructor state. */
        launch_check(!owner->messages && !owner->dedicated && !owner->native_owner, "execution role set before publication only");
        owner->provider_execution = 0;
    }
    return result;
}


/* EN/RU: Original caller-relative source selection returns an actual owned plan/0; no parser/load result is mocked.
 * Исходный caller-relative selection выдаёт настоящий owned plan/0; parser/load result не подменяется.
 */
extern int __real_dvm_resource_plan_call(DVM_Namespace *, const char *, unsigned, DVM_ResourcePlan *);


/* EN/RU: Observe actual loader's ACTIVE caller role, never infer side from module names or change guest state.
 * Наблюдать роль настоящего ACTIVE caller loader, не выводить сторону из имени и не менять guest state.
 */
int __wrap_dvm_resource_plan_call(DVM_Namespace *group, const char *path, unsigned local_role, DVM_ResourcePlan *plan)
{
    Prog *caller = currun(); /* Current permanent owner, or initial controller before any guest. */
    unsigned expected = caller ? ((DVM_RuntimeProcess *)caller)->module_local_role :
        route_selecting && CLIENT_OWNER ? DVM_ORIGIN_TERMINAL : DVM_ORIGIN_PROVIDER;
    launch_check(local_role == expected, "actual module loader did not select its caller-local side");
    __atomic_add_fetch(local_role == DVM_ORIGIN_TERMINAL ? &terminal_lookups : &provider_lookups, 1, __ATOMIC_RELAXED);
    return __real_dvm_resource_plan_call(group, path, local_role, plan);
}


/* EN/RU: Own real compiled native-cell images under one opt-in live root; no Module or parser result is fabricated.
 * Владеть настоящими compiled native-cell images под opt-in live root; Module/parser results не подменяются.
 */
static DVM_Provider *source(ImageSource *images, int terminal, DVM_LaunchEndpoint *endpoint)
{
    images->endpoint = dvm_launch_endpoint_clone(endpoint);
    launch_check(images->endpoint != NULL, "advertised image source endpoint pin");
    images->terminal = terminal;
    images->app = image_read("app.dis");
    images->first = image_read(terminal ? "terminal.dis" : "first.dis");
    images->next = image_read(terminal ? "terminal.dis" : "next.dis");
    if (!terminal && CONTROL == 5)
    {
        memset(images->first.bytes, 0, images->first.size); /* Correct byte count, intentionally invalid Dis image. */
    }
    launch_check(terminal || images->first.size == images->next.size, "same-length live provider image fixture");
    return dvm_provider_create_routed(&image_operations, images, NULL, 1,
        DVM_PROVIDER_CANCEL_PATH | DVM_PROVIDER_CANCEL_LAUNCH, image_launch);
}


/* EN/RU: Native admission handler owns only launch SDK values, never a Module/Prog or managed pointer.
 * Native admission handler владеет лишь launch SDK values, без Module/Prog и managed pointers.
 */
typedef struct Admission
{
    DVM_LaunchService *service; /* Borrowed owner, main joins handler before closing/freeing. */
    DVM_LaunchOptions *options; /* Borrowed immutable granted root/source association. */
    DVM_LaunchContract *exports; /* Borrowed immutable actual owner exports through native handler join. */
    DVM_LaunchContract *wrong; /* Borrowed real compiled incompatible exports, never code executed by this thread. */
    size_t required_count; /* Actual caller members; empty unused interfaces legitimately accept extra exports. */
    DVM_LaunchHandoff *handoff; /* Owned result moved back to the startup controller after join. */
} Admission;


/* EN/RU: Block on actual admission queue, prepare/take one environment on its native handler, never execute guest code.
 * Блокироваться на actual admission queue, prepare/take одного окружения на native handler, без guest execution.
 */
static void *prepare(void *state)
{
    Admission *admission = state;
    DVM_LaunchReview *review = NULL;
    launch_check(dvm_launch_service_wait(admission->service, &review) == DVM_LAUNCH_WAIT_REQUEST,
        "native admission receives one provider module request");
    DVM_LaunchRequest *request = dvm_launch_review_request(review);
    DVM_LaunchDescription description;
    launch_check(dvm_launch_describe(request, &description) && description.typed &&
        !dvm_launch_request_prepare(request, admission->options->provider, &admission->options->provider_origin),
        "actual typed request bypassed interface validation");
    if (admission->required_count)
    {
        launch_check(!dvm_launch_request_check(request, admission->wrong),
            "actual Limbo int/big interface mismatch was accepted");
    }
    launch_check(dvm_launch_request_check(request, admission->exports) &&
        dvm_launch_request_prepare(request, admission->options->provider, &admission->options->provider_origin),
        "native admission checks actual exports before environment preparation");
    admission->handoff = dvm_launch_request_take(request);
    launch_check(admission->handoff != NULL, "native admission one-use take");
    dvm_launch_review_free(review);
    return NULL;
}


/* EN/RU: Build an actual pre-init namespace from both granted roots and invoke the unchanged production startup.
 * Собрать настоящий namespace до init из двух granted roots, вызвать неизменённый production startup.
 */
Prog *__wrap_dvm_runtime_launch_module(Module *module, DVM_LaunchHandoff *unused)
{
    (void)unused;
    interface_adapter_checks();
    LaunchState terminal = {0}; /* Reuse existing checked source-association fixture only. */
    LaunchState execution = {0}; /* Temporary local factories are disposed before image roots publish. */
    DVM_LaunchOptions options = launch_options(&terminal, &execution);
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    DVM_LaunchService *server = dvm_launch_service_create(options.provider_origin.system, 1);
    DVM_LaunchService *client = dvm_launch_service_create(options.terminal_origin.system, 1);
    DVM_LaunchEndpoint *server_endpoint = dvm_launch_service_endpoint(server);
    DVM_LaunchEndpoint *client_endpoint = dvm_launch_service_endpoint(client);
    launch_check(server && client && server_endpoint && client_endpoint, "image source owner services");
    options.terminal = source(&terminal_images, 1, client_endpoint);
    options.provider = source(&execution_images, 0, server_endpoint);
    DVM_LaunchSpec spec = {0}; /* Existing typed launch ticket, no new network schema. */
    spec.policy = options.policy;
    spec.terminal = options.terminal;
    spec.terminal_origin = options.terminal_origin;
    strcpy(spec.expected_provider, options.provider_origin.system);
    strcpy(spec.module, "/app.dis");
    const unsigned char arguments[] = {0, 255, 1, 128}; /* Binary startup bytes are independent of display. */
    spec.arguments = arguments;
    spec.argument_bytes = sizeof(arguments);
    DVM_Namespace *caller_namespace = dvm_launch_prepare(&options); /* Caller view selects source before owner admission. */
    launch_check(caller_namespace != NULL, "image routing caller namespace");
    DVM_LaunchOptions callee = options; /* Borrowed input roots stay owned by original options through handler join. */
    if (CLIENT_OWNER)
    {
        callee.provider = options.terminal; /* Selected client source supplies its own execution root, not provider's root. */
        callee.provider_origin = options.terminal_origin;
        callee.provider_origin.role = DVM_ORIGIN_PROVIDER; /* That source is now the callee's local provider role. */
    }
    DVM_LaunchContract *required = required_interface(module);
    interface_binding_checks(module, required);
    DVM_LaunchContract *exports = image_interface(CLIENT_OWNER ? &terminal_images.app : &execution_images.app,
        "/native-contract-owner");
    Image wrong_image = image_read("wrong.dis");
    DVM_LaunchContract *wrong = image_interface(&wrong_image, "/native-contract-wrong");
    free(wrong_image.bytes);
    spec.contract = required;
    Admission admission = {.service = CLIENT_OWNER ? client : server, .options = &callee,
        .exports = exports, .wrong = wrong, .required_count = dvm_launch_contract_count(required)};
    pthread_t handler; /* Dedicated native admission thread, never a guest executor. */
    launch_check(!pthread_create(&handler, NULL, prepare, &admission), "native admission handler construction");
    strcpy(spec.module, "/caller-label-must-not-be-used");
    memset(spec.expected_provider, 'x', sizeof(spec.expected_provider)); /* Address must supply actual owner identity. */
    route_selecting = 1;
    DVM_LaunchAddress *address = dvm_launch_route_resolve(caller_namespace, "/app.dis",
        CLIENT_OWNER ? DVM_ORIGIN_TERMINAL : DVM_ORIGIN_PROVIDER);
    route_selecting = 0;
    launch_check(address != NULL, "provider module deferred owner resolution");
    DVM_LaunchServiceInfo info;
    launch_check(dvm_launch_service_info(admission.service, &info) && !info.queued && !info.outstanding,
        "provider module resolution reserved launch quota");
    if (CONTROL == 18 || CONTROL == 19)
    {
        DVM_Provider *opposite = CLIENT_OWNER ? options.provider : options.terminal;
        launch_check(dvm_stack_mount(caller_namespace, opposite, "/", 0, 0) == 1,
            "replace caller bind before using resolved module owner");
    }
    dvm_namespace_release(caller_namespace); /* Resolved destination must work after caller view retirement. */
    DVM_LaunchTicket *ticket = dvm_launch_address_submit(address, &spec);
    dvm_launch_address_free(address);
    dvm_launch_contract_free(required); /* Request owns its independent snapshot through owner review. */
    launch_check(ticket && !pthread_join(handler, NULL), "native resolved request and handler join");
    dvm_launch_contract_free(exports);
    dvm_launch_contract_free(wrong);
    DVM_LaunchHandoff *handoff = admission.handoff; /* Callee owns an independent prepared environment, no display needed. */
    if (CLIENT_OWNER)
    {
        handoff->local_role = DVM_ORIGIN_TERMINAL;
    }
    if (CONTROL == 20 || CONTROL == 21)
    {
        ImageSource *changed = CLIENT_OWNER ? &terminal_images : &execution_images;
        free(changed->app.bytes);
        changed->app = image_read("wrong.dis"); /* Change only after native owner accepted the original interface. */
    }
    if (CONTROL == 24)
    {
        free(execution_images.next.bytes);
        execution_images.next = image_read("wrong.dis"); /* Parent explicitly switches future opens before child loads. */
    }
    dvm_launch_service_free(server);
    dvm_launch_service_free(client);
    dvm_launch_endpoint_free(server_endpoint);
    dvm_launch_endpoint_free(client_endpoint);
    environment_prepare(&options);
    Prog *process = __real_dvm_runtime_launch_module(module, handoff);
    launch_check(process && !handoff->group, "image-root environment adopted before first guest dispatch");
    dvm_launch_handoff_free(handoff);
    dvm_launch_ticket_free(ticket);
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    return process;
}


/* EN/RU: After native closer/guest shutdown, prove exact source selection, no denied-code fallback and complete provider drain.
 * После native closer/guest shutdown проверить источник, отсутствие fallback при отказе кода и provider-drain.
 */
void __wrap_dvm_namespace_boot_shutdown(void)
{
    __real_dvm_namespace_boot_shutdown();
    unsigned expected_execution_opens = (CONTROL >= 20 && CONTROL <= 23) || CONTROL == 13 || CLIENT_OWNER ? 0U :
        (CONTROL == 16 || CONTROL == 18 || CONTROL == 27) ? 2U : CONTROL ? 1U : 4U;
    launch_check(terminal_images.destroyed == 1 && execution_images.destroyed == 1 &&
        terminal_images.opened == (CLIENT_OWNER && CONTROL != 21 ? 2U : 0U) &&
        execution_images.opened == expected_execution_opens, "provider code source and lifetime isolation");
    launch_check(allocated_images == freed_images && (CONTROL == 13 || allocated_images > 0), "actual provider image-buffer ownership drain");
    launch_check(terminal_images.advertised == (CLIENT_OWNER ? 1U : 0U) &&
        execution_images.advertised == (CLIENT_OWNER ? 0U : 1U), "actual selected provider advertisement coverage");
    launch_check(provider_lookups && (CLIENT_OWNER ? terminal_lookups != 0 : terminal_lookups == 0),
        "actual caller-side load coverage");
    launch_check((CONTROL == 10 || CONTROL == 13 || checked_targets) &&
        refused_targets == (CONTROL >= 20 && CONTROL <= 24 ? 1U : 0U), "actual inherited target-contract guard coverage");
    launch_check(CONTROL < 25 || environment_bindings == (CONTROL == 27 ? 2U : 1U),
        "actual module instance environment coverage");
    puts("PASS provider module source selection and native ownership drain");
}
