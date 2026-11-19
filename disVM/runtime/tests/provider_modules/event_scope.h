/* Copyright (c) 2026 bhgv
 * EN: Precise control for the former module-pctl bug that rebound the caller's base event inbox.
 * RU: Точный контроль прежней ошибки pctl модуля, перенастраивавшей базовую очередь вызывающего.
 */
#ifndef PROVIDER_MODULE_EVENT_SCOPE_H
#define PROVIDER_MODULE_EVENT_SCOPE_H
#include "../../vfs/sysevents.h"
#include "../../vfs/sysevents_internal.h"
static unsigned misdirected_scopes; /* Actual intercepted scope replacement; observed after the single guest owner retires. */
static unsigned refused_scopes; /* Actual native subscription-allocation refusal followed by a successful guest retry. */
static DVM_SysEvents *module_inbox; /* Borrowed while the permanent process owns its native binding slot. */
static unsigned background_reports; /* Actual source reports delivered while the caller, not this module, executes. */


/* EN/RU: Original contextual open borrows events/mode and returns an owned handle in empty output.
 * Исходное открытие заимствует events/mode и возвращает свой handle в пустом output.
 */
extern FVM_Result __real_dvm_sysevents_open(DVM_SysEvents *events, const char *mode, DVM_VfsHandle *output);


/* EN: Observe the actual successful module FD's inbox; preserve all device selection and ownership.
 * RU: Запомнить очередь настоящего открытого FD модуля, сохранив выбор устройства и владение.
 */
FVM_Result __wrap_dvm_sysevents_open(DVM_SysEvents *events, const char *mode, DVM_VfsHandle *output)
{
    FVM_Result result = __real_dvm_sysevents_open(events, mode, output);
    if (CONTROL >= 39 && result == FVM_OK && dvm_module_environment_current(currun()))
    {
        module_inbox = events;
    }
    return result;
}


/* EN/RU: Original Sys.bind borrows its rooted builtin frame through return and publishes the actual native result.
 * Исходный Sys.bind заимствует укоренённый кадр до возврата и публикует настоящий результат.
 */
extern void __real_Sys_bind(void *frame);


/* EN: After the caller's real bind, publish through a real subscribed source journal without entering the module.
 * Pin the source under the inbox lock; notify outside it and require immediate automatic native delivery.
 * RU: После настоящего bind вызывающего послать уведомление настоящего источника без входа в модуль.
 * Удержать источник под блокировкой очереди, уведомить вне неё и проверить автоматическую доставку.
 */
void __wrap_Sys_bind(void *frame)
{
    __real_Sys_bind(frame);
    if (CONTROL >= 39 && module_inbox && !dvm_module_environment_current(currun()))
    {
        launch_check(module_inbox && !dvm_module_environment_current(currun()), "caller executes outside module scope");
        fvm_port_sync_lock(module_inbox->lock);
        DVM_Provider *provider = module_inbox->watches ? module_inbox->watches->provider : NULL;
        size_t before = module_inbox->pending;
        uint64_t generation = module_inbox->watches ? module_inbox->watches->last_generation + 1 : 0;
        launch_check(provider && dvm_provider_retain(provider), "pin actual subscribed source");
        fvm_port_sync_unlock(module_inbox->lock);
        DVM_ChangeEvent event = {dvm_changes_session(dvm_provider_changes(provider)), generation, DVM_CHANGE_CONTENT, 0, 0};
        launch_check(dvm_changes_notify(dvm_provider_changes(provider), &event) == 1, "actual background provider report");
        fvm_port_sync_lock(module_inbox->lock);
        launch_check(module_inbox->pending > before, "module notification must not require module execution/read");
        fvm_port_sync_unlock(module_inbox->lock);
        dvm_provider_release(provider);
        background_reports++;
    }
}


/* EN/RU: Original native scope replacement borrows both arguments; retain/purge rules remain unchanged.
 * Исходная смена области заимствует аргументы; правила удержания и очистки не меняются.
 */
extern int __real_dvm_sysevents_group(DVM_SysEvents *events, DVM_Namespace *group);


/* EN: Forward all ordinary changes; control41 alone restores the old wrong-inbox argument for an active bound module.
 * Control42 refuses one real subscription allocation, requiring a normal guest retry without partial commit.
 * No result or event is fabricated; the real replacement and real device reads must expose the isolation violation.
 * RU: Переслать обычные изменения; только контроль41 восстанавливает неверный аргумент для активного модуля.
 * Контроль42 отклоняет одно выделение подписки и требует обычной повторной попытки гостя без частичной фиксации.
 * События и результаты не подменяются: настоящая смена области и чтение устройства должны выявить нарушение.
 */
int __wrap_dvm_sysevents_group(DVM_SysEvents *events, DVM_Namespace *group)
{
    Prog *process = currun();
    if (CONTROL == 42 && !refused_scopes && process && dvm_module_environment_current(process))
    {
        binding_allocation = 1;
        int result = __real_dvm_sysevents_group(events, group);
        launch_check(!result && !binding_allocation, "actual subscription allocation refusal before scope replacement");
        refused_scopes++;
        return result;
    }
    if (CONTROL == 41 && process && dvm_module_environment_current(process))
    {
        launch_check(events != ((DVM_RuntimeProcess *)process)->system_events, "module must select an independent inbox");
        events = ((DVM_RuntimeProcess *)process)->system_events;
        misdirected_scopes++;
    }
    return __real_dvm_sysevents_group(events, group);
}
#endif
