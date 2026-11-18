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
#include "vfs/resource.h"
#include "../../../native/tests/launch/backend.h"
#include "images.h"
static ImageSource terminal_images; /* Native fixture bytes live through every guest/closer owner. */
static unsigned provider_lookups; /* Atomic observed caller-side load selections, including initial controller. */
static unsigned terminal_lookups; /* Atomic observed accepted client-caller selections. */
static ImageSource execution_images; /* Actual provider-first code source. */


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
    unsigned expected = caller ? ((DVM_RuntimeProcess *)caller)->module_local_role : DVM_ORIGIN_PROVIDER;
    launch_check(local_role == expected, "actual module loader did not select its caller-local side");
    __atomic_add_fetch(local_role == DVM_ORIGIN_TERMINAL ? &terminal_lookups : &provider_lookups, 1, __ATOMIC_RELAXED);
    return __real_dvm_resource_plan_call(group, path, local_role, plan);
}


/* EN/RU: Own real compiled native-cell images under one opt-in live root; no Module or parser result is fabricated.
 * Владеть настоящими compiled native-cell images под opt-in live root; Module/parser results не подменяются.
 */
static DVM_Provider *source(ImageSource *images, int terminal)
{
    images->terminal = terminal;
    images->app = image_read("app.dis");
    images->first = image_read(terminal ? "terminal.dis" : "first.dis");
    images->next = image_read(terminal ? "terminal.dis" : "next.dis");
    if (!terminal && CONTROL == 5)
    {
        memset(images->first.bytes, 0, images->first.size); /* Correct byte count, intentionally invalid Dis image. */
    }
    launch_check(terminal || images->first.size == images->next.size, "same-length live provider image fixture");
    return dvm_provider_create_native(&image_operations, images, NULL, 1, DVM_PROVIDER_CANCEL_PATH);
}


/* EN/RU: Native admission handler owns only launch SDK values, never a Module/Prog or managed pointer.
 * Native admission handler владеет лишь launch SDK values, без Module/Prog и managed pointers.
 */
typedef struct Admission
{
    DVM_LaunchService *service; /* Borrowed owner, main joins handler before closing/freeing. */
    DVM_LaunchOptions *options; /* Borrowed immutable granted root/source association. */
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
    launch_check(dvm_launch_request_prepare(request, admission->options->provider, &admission->options->provider_origin),
        "native admission prepares provider module environment");
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
    LaunchState terminal = {0}; /* Reuse existing checked source-association fixture only. */
    LaunchState execution = {0}; /* Temporary local factories are disposed before image roots publish. */
    DVM_LaunchOptions options = launch_options(&terminal, &execution);
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    options.terminal = source(&terminal_images, 1);
    options.provider = source(&execution_images, 0);
    DVM_LaunchSpec spec = {0}; /* Existing typed launch ticket, no new network schema. */
    spec.policy = options.policy;
    spec.terminal = options.terminal;
    spec.terminal_origin = options.terminal_origin;
    strcpy(spec.expected_provider, options.provider_origin.system);
    strcpy(spec.module, "/app.dis");
    const unsigned char arguments[] = {0, 255, 1, 128}; /* Binary startup bytes are independent of display. */
    spec.arguments = arguments;
    spec.argument_bytes = sizeof(arguments);
    DVM_LaunchService *service = dvm_launch_service_create(options.provider_origin.system, 1);
    DVM_LaunchEndpoint *endpoint = dvm_launch_service_endpoint(service);
    launch_check(service && endpoint, "provider module admission service");
    Admission admission = {.service = service, .options = &options}; /* Native values, no managed roots leave controller. */
    pthread_t handler; /* Dedicated native admission thread, not a guest executor. */
    launch_check(!pthread_create(&handler, NULL, prepare, &admission), "native admission handler construction");
    DVM_LaunchTicket *ticket = dvm_launch_service_submit(endpoint, &spec);
    launch_check(ticket && !pthread_join(handler, NULL), "native admission request and handler join");
    DVM_LaunchHandoff *handoff = admission.handoff; /* Independent owned environment, no initial display context. */
    if (CONTROL == 17)
    {
        handoff->local_role = DVM_ORIGIN_TERMINAL; /* Trusted host chooses its local lookup side before process publication. */
    }
    dvm_launch_service_free(service);
    dvm_launch_endpoint_free(endpoint);
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
    launch_check(terminal_images.destroyed == 1 && execution_images.destroyed == 1 &&
        terminal_images.opened == (CONTROL == 17 ? 2U : 0U) &&
        execution_images.opened == (CONTROL == 13 || CONTROL == 17 ? 0U : CONTROL == 16 ? 2U : CONTROL ? 1U : 4U), "provider code source and lifetime isolation");
    launch_check(allocated_images == freed_images && (CONTROL == 13 || allocated_images > 0), "actual provider image-buffer ownership drain");
    launch_check(provider_lookups && (CONTROL == 17 ? terminal_lookups != 0 : terminal_lookups == 0),
        "actual caller-side load coverage");
    puts("PASS provider module source selection and native ownership drain");
}
