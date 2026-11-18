/* Copyright (c) 2026 bhgv
 * EN: Actual constructor handoff, startup snapshot refusal/ownership and console-only guest launch.
 * RU: Настоящий constructor handoff, отказ/владение startup-копией и console-only guest launch.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "raise.h"
#include "runqueue.h"
#include "vfs/launch/runtime.h"
#include "vfs/launch/context.h"
#include "../../../native/tests/launch/backend.h"
#include <pthread.h>
#include <limits.h>
static LaunchState terminal; /* Native root fixture, lives until final runtime shutdown. */
static LaunchState provider; /* Execution source association, no managed pointers. */
static int refuse_snapshot; /* Controller-only one actual snapshot allocation failure. */
static __thread int refuse_allocation; /* Controller-only next snapshot allocation refusal. */
static pthread_mutex_t lifetime_lock = PTHREAD_MUTEX_INITIALIZER; /* Native snapshot free ledger. */
static DVM_LaunchContext *tracked; /* Native allocation identity only, never read after free. */
static unsigned created; /* Count actual successful snapshots. */
static unsigned retired; /* Count actual snapshot frees. */


/* EN/RU: Actual constructor borrows loaded bootstrap, moves accepted environment; ordinary startup frame unchanged.
 * Настоящий constructor заимствует loaded bootstrap и переносит окружение, startup frame не меняется.
 */
extern Prog *__real_dvm_runtime_launch_module(Module *, DVM_LaunchHandoff *);


/* EN/RU: Actual snapshot allocator, no fake context is supplied to production.
 * Настоящий snapshot allocator, production не получает fake context.
 */
extern DVM_LaunchContext *__real_dvm_launch_context_create(const DVM_LaunchHandoff *);


/* EN/RU: Original allocator accepts requested native size and returns owned storage/NULL.
 * Исходный allocator получает native size и возвращает owned storage/NULL.
 */
extern void *__real_malloc(size_t);


/* EN/RU: Original free consumes native allocation/NULL without fixture recursion.
 * Исходный free поглощает native allocation/NULL, без fixture-рекурсии.
 */
extern void __real_free(void *);


/* EN/RU: Original shutdown joins guest/native services before checking native ownership drain.
 * Исходный shutdown завершает guest/native services до проверки ownership-drain.
 */
extern void __real_dvm_namespace_boot_shutdown(void);


/* EN/RU: Refuse only the armed constructor's next native snapshot allocation, no other workers are live yet.
 * Отказать лишь следующему native snapshot allocation выбранного constructor, workers ещё не опубликованы.
 */
void *__wrap_malloc(size_t size)
{
    if (refuse_allocation)
    {
        refuse_allocation = 0;
        return NULL;
    }
    return __real_malloc(size);
}


/* EN/RU: Track actual native snapshot free, preserving all other production allocation retirement.
 * Учесть настоящий native snapshot free, сохраняя остальные production освобождения.
 */
void __wrap_free(void *value)
{
    pthread_mutex_lock(&lifetime_lock);
    if (value && value == tracked)
    {
        tracked = NULL;
        retired++;
    }
    pthread_mutex_unlock(&lifetime_lock);
    __real_free(value);
}


/* EN/RU: Observe actual allocation and overflow refusal before publication; never alter a live guest's state.
 * Наблюдать allocation и overflow refusal до publication; не менять state работающего guest.
 */
DVM_LaunchContext *__wrap_dvm_launch_context_create(const DVM_LaunchHandoff *handoff)
{
    if (refuse_snapshot)
    {
        refuse_snapshot = 0;
        refuse_allocation = 1;
    }
    DVM_LaunchContext *context = __real_dvm_launch_context_create(handoff);
    if (context)
    {
        launch_check(!tracked, "only one live startup snapshot in this fixture");
        launch_check(context->argument_bytes == handoff->description.argument_bytes &&
            (!context->argument_bytes || !memcmp(context->arguments, handoff->arguments, context->argument_bytes)), "complete native startup bytes");
        context->references = UINT_MAX; /* Private unpublished state only. */
        launch_check(!dvm_launch_context_retain(context) && context->references == UINT_MAX, "snapshot reference overflow refuses");
        context->references = 1;
        pthread_mutex_lock(&lifetime_lock);
        tracked = context;
        created++;
        pthread_mutex_unlock(&lifetime_lock);
    }
    return context;
}


/* EN/RU: Enter real constructor error boundary for expected failure; handoff remains exclusively owned by caller.
 * Войти в настоящий constructor error boundary для expected failure; caller сохраняет exclusive handoff.
 */
static void refusal(Module *module, DVM_LaunchHandoff *handoff)
{
    if (waserror())
    {
        launch_check(handoff->group && !tracked, "startup refusal preserves handoff and retires snapshot");
        return;
    }
    (void)__real_dvm_runtime_launch_module(module, handoff);
    poperror();
    launch_check(0, "malformed/OOM startup unexpectedly published process");
}


/* EN/RU: Prepare binary/empty/maximal payloads, reject malformed/OOM handoffs, then launch through actual adoption.
 * Подготовить binary/empty/maximal payload, проверить malformed/OOM handoff и настоящий adoption.
 */
Prog *__wrap_dvm_runtime_launch_module(Module *module, DVM_LaunchHandoff *unused)
{
    (void)unused;
    DVM_LaunchOptions options = launch_options(&terminal, &provider);
    size_t size = CONTROL == 1 ? 0 : CONTROL == 2 ? DVM_LAUNCH_ARGUMENT_LIMIT : 257;
    unsigned char *bytes = malloc(size ? size : 1); /* Source grant is only borrowed by request construction. */
    launch_check(bytes != NULL, "startup argument fixture allocation");
    for (size_t i = 0; i < size; i++)
    {
        bytes[i] = (unsigned char)i;
    }
    DVM_LaunchSpec spec = {0}; /* No FlashVM grant, ordinary console FDs remain valid. */
    spec.policy = options.policy;
    spec.terminal = options.terminal;
    spec.terminal_origin = options.terminal_origin;
    strcpy(spec.expected_provider, options.provider_origin.system);
    strcpy(spec.module, "console/ordinary-module");
    spec.arguments = bytes;
    spec.argument_bytes = size;
    DVM_LaunchRequest *request = NULL; /* Actual admission/take capabilities, not a fake Module result. */
    DVM_LaunchTicket *ticket = dvm_launch_request_create(&spec, &request);
    launch_check(ticket && request, "startup request construction");
    memset(bytes, 0x5a, size);
    free(bytes);
    launch_check(dvm_launch_request_prepare(request, options.provider, &options.provider_origin), "startup request preparation");
    DVM_LaunchHandoff *handoff = dvm_launch_request_take(request);
    launch_check(handoff != NULL, "startup one-use handoff");
    dvm_launch_request_free(request);
    dvm_launch_ticket_free(ticket);
    size_t saved_size = handoff->description.argument_bytes; /* Restore each exclusive invalid input before retry. */
    unsigned char *saved_bytes = handoff->arguments;
    handoff->description.argument_bytes = DVM_LAUNCH_ARGUMENT_LIMIT + 1;
    refusal(module, handoff);
    handoff->description.argument_bytes = 1;
    handoff->arguments = NULL;
    refusal(module, handoff);
    handoff->arguments = saved_bytes;
    handoff->description.argument_bytes = saved_size;
    memset(handoff->description.module, 'x', sizeof(handoff->description.module));
    refusal(module, handoff);
    memset(handoff->description.module, 0, sizeof(handoff->description.module));
    strcpy(handoff->description.module, "console/ordinary-module");
    refuse_snapshot = 1;
    refusal(module, handoff);
    launch_check(!refuse_snapshot && !refuse_allocation, "actual snapshot allocation refusal consumed");
    Prog *process = __real_dvm_runtime_launch_module(module, handoff);
    launch_check(process && !handoff->group && tracked, "startup data adopted before first dispatch");
    if (saved_size)
    {
        memset(handoff->arguments, 0xa5, saved_size);
    }
    memset(handoff->description.module, 0, sizeof(handoff->description.module));
    dvm_launch_handoff_free(handoff);
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    return process;
}


/* EN/RU: After all native owners/children retire, require exact snapshot and root ownership drain.
 * После завершения native owners/children требовать точный snapshot/root ownership-drain.
 */
void __wrap_dvm_namespace_boot_shutdown(void)
{
    __real_dvm_namespace_boot_shutdown();
    launch_check(!tracked && created == 1 && retired == 1 && terminal.destroyed == 1 && provider.destroyed == 1,
        "startup snapshot and namespace ownership drain");
    puts("PASS launch context native snapshot lifetime");
}
