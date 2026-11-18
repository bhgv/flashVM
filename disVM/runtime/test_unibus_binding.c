/* Copyright (c) 2026 bhgv
 * EN: Import a native Unibus binding through the real pre-init launch transaction.
 * RU: Принять native Unibus binding через настоящую транзакцию запуска до init.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "kernel.h"
#include "runt.h"
#include "sys_capture.h"
#include "runqueue.h"
#include "disvm/unibus_binding.h"
#include "../native/tests/launch/backend.h"
#include "vfs/launch/runtime.h"
#include <stddef.h>
static LaunchState terminal; /* Native roots survive all guest owners. */
static LaunchState execution; /* Native execution environment fixture. */
static unsigned attached; /* Atomic ordinary attach callbacks. */
static unsigned detached; /* Atomic ordinary detach callbacks. */
static unsigned retired; /* Atomic native root retirement callbacks. */
static unsigned destroyed; /* Atomic final native binding disposal callbacks. */
static unsigned refused_disposed; /* Deliberately reused binding cleanup, separate from successful launch. */
Prog *__real_dvm_runtime_launch_module(Module *, DVM_LaunchHandoff *);
void __real_dvm_namespace_boot_shutdown(void);


/* EN: Run the same generated .m ABI on the actual calling guest owner; native fixture values
 * stand for launch configuration, not remote rendering/RPC execution. Retain no managed inputs.
 * RU: Использовать настоящий .m ABI на своём guest owner. Это native-конфигурация запуска,
 * не проверка удалённого renderer/RPC. Managed-аргументы не сохраняются.
 */
static int invoke(void *state, int pid, unsigned method, void *frame)
{
    (void)state;
    launch_check(pid > 0, "current process identity");
    if (method == UNIBUS_ROWS || method == UNIBUS_LINES || method == UNIBUS_FRESH)
    {
        F_Unibus_rows call; /* These three zero-argument scalar signatures have identical native layout. */
        WORD value = method == UNIBUS_ROWS ? 47 : method == UNIBUS_LINES ? 1 : CONTROL != 0;
        dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Unibus_rows, ret), sizeof(*call.ret));
        dvm_sys_scalar_result(&value, call.ret, sizeof(value));
        return 1;
    }
    if (method == UNIBUS_PATH)
    {
        F_Unibus_path call; /* Owner-local mapped string result. */
        dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Unibus_path, ret), sizeof(*call.ret));
        retstr(CONTROL >= 2 ? "job-native-revoke" : "job-native", call.ret);
        return 1;
    }
    if (method == UNIBUS_ATTACH)
    {
        F_Unibus_attach call; /* Validate actual typed guest arguments without retaining them. */
        WORD value = 0; /* Successful fixture registration. */
        dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Unibus_attach, ret), sizeof(*call.ret));
        launch_check(call.input != H && call.output != H && call.input != call.output,
            "root typed attach frame");
        __atomic_add_fetch(&attached, 1, __ATOMIC_RELAXED);
        dvm_sys_scalar_result(&value, call.ret, sizeof(value));
        return 1;
    }
    if (method == UNIBUS_DETACH)
    {
        __atomic_add_fetch(&detached, 1, __ATOMIC_RELAXED);
        if (CONTROL >= 2)
        {
            Prog *process = currun(); /* Revoke on the same actual native identity used by retirement. */
            unibusretire(((DVM_RuntimeProcess *)process)->unibus, (uintptr_t)process);
        }
        return 1;
    }
    return 0;
}


/* EN/RU: Count root retirement only, with no managed access or guest exception.
 * Считать только root retirement, без managed-доступа и guest exception.
 */
static void retire(void *state)
{
    (void)state;
    __atomic_add_fetch(&retired, 1, __ATOMIC_RELAXED);
}


/* EN/RU: Count final native state disposal after all retained descendants finish.
 * Считать освобождение native-состояния после завершения всех retained потомков.
 */
static void dispose(void *state)
{
    (void)state;
    __atomic_add_fetch(&destroyed, 1, __ATOMIC_RELAXED);
}


/* EN/RU: Count disposal of a refused capability without touching the successful job counters.
 * Считать освобождение отказанной capability отдельно от успешного задания.
 */
static void refused_dispose(void *state)
{
    (void)state;
    refused_disposed++;
}


/* EN/RU: Borrow attempted module/handoff only through one guarded native construction.
 * Заимствовать module/handoff только на одну защищённую native-попытку.
 */
typedef struct Attempt
{
    Module *module; /* Controller-owned module, retained by the real constructor. */
    DVM_LaunchHandoff *handoff; /* Exclusive source; refusal must preserve its owners. */
} Attempt;


/* EN/RU: Raise through the actual startup constructor on an already claimed binding.
 * Получить настоящий отказ конструктора для уже занятого binding.
 */
static int refused_launch(void *opaque)
{
    Attempt *attempt = opaque;
    (void)__real_dvm_runtime_launch_module(attempt->module, attempt->handoff);
    return 1;
}


/* EN/RU: Import binding before the first bootstrap instruction; preserve real launch ownership.
 * Принять binding до первой инструкции bootstrap, сохранить настоящее владение launch.
 */
Prog *__wrap_dvm_runtime_launch_module(Module *module, DVM_LaunchHandoff *unused)
{
    (void)unused;
    terminal.terminal = 1;
    DVM_LaunchOptions options = launch_options(&terminal, &execution);
    DVM_LaunchSpec spec = {0};
    spec.policy = options.policy;
    spec.terminal = options.terminal;
    spec.terminal_origin = options.terminal_origin;
    strcpy(spec.expected_provider, options.provider_origin.system);
    strcpy(spec.module, "unibus-launch");
    if (CONTROL == 3)
    {
        DVM_LaunchRequest *refused_request = NULL;
        DVM_LaunchTicket *refused_ticket = dvm_launch_request_create(&spec, &refused_request);
        launch_check(refused_ticket &&
            dvm_launch_request_prepare(refused_request, options.provider, &options.provider_origin), "refusal setup");
        DVM_LaunchHandoff *refused = dvm_launch_request_take(refused_request);
        UnibusOps refused_ops = {invoke, NULL, refused_dispose};
        UnibusBinding *used = unibusbinding(&refused_ops, NULL, 0);
        launch_check(refused && used && unibusclaim(used, 1) &&
            dvm_launch_handoff_unibus(refused, used), "deliberately reused native identity");
        unibusrelease(used);
        DVM_Namespace *group = refused->group; /* Exclusive ownership must survive failure unchanged. */
        Attempt attempt = {module, refused};
        launch_check(!kprotect(refused_launch, &attempt) && refused->group == group && refused->unibus == used,
            "refused adoption leaves namespace and handler in source handoff");
        dvm_launch_handoff_free(refused);
        dvm_launch_ticket_free(refused_ticket);
        dvm_launch_request_free(refused_request);
        launch_check(refused_disposed == 1, "refused handoff disposes exactly its own binding");
    }
    DVM_LaunchRequest *request = NULL;
    DVM_LaunchTicket *ticket = dvm_launch_request_create(&spec, &request);
    launch_check(ticket && dvm_launch_request_prepare(request, options.provider, &options.provider_origin),
        "native launch prepare");
    DVM_LaunchHandoff *handoff = dvm_launch_request_take(request);
    UnibusOps ops = {invoke, retire, dispose};
    UnibusBinding *binding = unibusbinding(&ops, NULL, UNIBUS_OWNER_PORTS | (CONTROL ? UNIBUS_INHERIT : 0));
    launch_check(handoff && binding && dvm_launch_handoff_unibus(handoff, binding) &&
        !dvm_launch_handoff_unibus(handoff, binding), "exactly one handler per handoff");
    unibusrelease(binding);
    Prog *process = __real_dvm_runtime_launch_module(module, handoff);
    launch_check(process && !handoff->unibus, "adoption moved native handler before publication");
    dvm_launch_handoff_free(handoff);
    dvm_launch_ticket_free(ticket);
    dvm_launch_request_free(request);
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    return process;
}


/* EN/RU: After actual VM/namespace shutdown, native capability and roots must dispose once.
 * После настоящего VM/namespace shutdown capability и roots освобождаются ровно один раз.
 */
void __wrap_dvm_namespace_boot_shutdown(void)
{
    __real_dvm_namespace_boot_shutdown();
    launch_check(attached == 1 && detached == 1 && retired == 1 && destroyed == 1 &&
        terminal.destroyed == 1 && execution.destroyed == 1, "native Unibus owner lifetime drain");
    puts("PASS pre-init Unibus native ownership drain");
}
