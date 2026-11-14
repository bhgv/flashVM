/* Copyright (c) 2026 bhgv
 * EN: Lifecycle and wakeups for permanently assigned native process threads.
 * RU: Жизненный цикл и пробуждения закреплённых за процессами нативных потоков.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "disvm/native_process.h"
#include "disvm/native_owner.h"
#include "managed_workers.h"
#include "runtime_gc.h"
#include "debugger/debugger.h"
#include "runqueue.h"
#include "table_leases.h"
#include <limits.h>
#include <pthread.h>
#include "channel_sync.h"
#include "thread_port.h"
#include "thread_registrar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "resident_threads/observation_counter.h"
#include "resident_threads/worker_state.inc"
#include "resident_threads/worker_wait.inc"
#include "resident_threads/worker_debug.inc"
#include "resident_threads/worker_location.inc"
#include "resident_threads/worker_execute.inc"
#include "resident_threads/worker_start.inc"
#include "resident_threads/worker_observe.inc"


/* EN: Start the resident registry on the bound active controller.
 * diagnostics selects lifecycle output. turn and cancel_waits are borrowed
 * callbacks through stop. Return zero on invalid state or native allocation.
 * RU: Запустить реестр закреплённых потоков на активном контроллере.
 * Обратные вызовы заимствованы до stop. При ошибке вернуть ноль.
 */
int dvm_resident_threads_start(int diagnostics, int (*turn)(void), void (*cancel_waits)(void))
{
    DVM_NativeContext *context = dvm_native_context_current();
    DVM_HeapOwnerStats state;
    if (!turn || !cancel_waits || service.sync || !context || !up || up->prog || dvm_native_context_process(context) ||
        dvm_native_context_turn(context) || !dvm_native_owner_statistics(dvm_native_owner_current(), &state) ||
        state.phase != DVM_HEAP_ACTIVE || state.pending_progress || !dvm_native_owner_world(dvm_native_owner_current()))
    {
        return 0;
    }
    memset(&service, 0, sizeof(service));
    service.sync = fvm_port_sync_create();
    service.turn = turn;
    service.cancel_waits = cancel_waits;
    service.world = dvm_native_owner_world(dvm_native_owner_current());
    service.diagnostics = diagnostics;
    service.peak = 1;
    if (!service.sync)
    {
        return 0;
    }
    service.registrar = dvm_thread_registrar_create();
    if (!service.registrar)
    {
        fvm_port_sync_destroy(service.sync);
        memset(&service, 0, sizeof(service));
        return 0;
    }
    return 1;
}


/* EN: Start borrowed rooted guest through the live service. Caller owns resident startup roots;
 * return zero on missing service/guest, stop or startup refusal. Wait outside admission.
 * RU: Запустить заимствованный укоренённый guest через сервис. Вызывающий владеет стартовыми
 * корнями; вернуть ноль при отсутствии сервиса/guest, остановке или отказе старта. Ожидать вне
 * допуска.
 */
int dvm_managed_workers_start_guest(Prog *guest)
{
    return service.sync && guest && !stopping() && start_helper(guest);
}


/* EN: Return this thread's borrowed permanent Prog or NULL for controller. No arguments, mutation
 * or ownership transfer; never choose another resident.
 * RU: Вернуть заимствованный постоянный Prog потока либо NULL для контроллера. Нет аргументов,
 * мутации и передачи владения; другой процесс не выбирается.
 */
Prog *dvm_managed_workers_guest(void)
{
    return current_guest ? current_guest->guest : NULL;
}


/* EN: Read atomic transit marker for borrowed live guest after FIFO checks. Caller pins guest and
 * dedicated slot; return zero when absent, without consuming a reply.
 * RU: Прочитать атомарную отметку transit живого guest после проверки FIFO. Вызывающий удерживает
 * guest и слот; вернуть ноль при отсутствии, ответ не извлекать.
 */
int dvm_managed_workers_reply_in_transit(Prog *guest)
{
    ManagedWorker *worker = ((DVM_RuntimeProcess *)guest)->dedicated;
    return worker && __atomic_load_n(&worker->reply_in_transit, __ATOMIC_ACQUIRE);
}


/* EN: Take the current resident's staged native message only for borrowed matching guest. Transfer
 * message to caller, keep transit until applied; return NULL if no matching reply. Owner thread
 * only.
 * RU: Забрать извлечённое нативное сообщение текущего процесса для совпадающего guest. Передать
 * сообщение вызывающему, сохранить transit до применения; вернуть NULL при отсутствии. Только
 * поток владельца.
 */
DVM_OwnerMessage *dvm_managed_workers_take_reply(Prog *guest)
{
    if (!current_guest || current_guest->guest != guest || !current_guest->staged_reply)
    {
        return NULL;
    }
    DVM_OwnerMessage *reply = current_guest->staged_reply;
    current_guest->staged_reply = NULL;
    return reply;
}


/* EN: Clear transit after applying the reply for borrowed matching guest. Owner thread only;
 * mismatches do nothing, no result or slot lifetime extension.
 * RU: Снять transit после применения ответа для совпадающего guest. Только поток владельца;
 * несовпадение ничего не меняет, результата и продления жизни слота нет.
 */
void dvm_managed_workers_finish_reply(Prog *guest)
{
    if (current_guest && current_guest->guest == guest)
    {
        __atomic_store_n(&current_guest->reply_in_transit, 0, __ATOMIC_RELEASE);
    }
}


/* EN: Owner thread clears its guest pointer after Prog destruction and records retirement. pid
 * must equal the permanent ID or abort; every retiring PID releases debug credits.
 * RU: Поток владельца снимает guest после удаления Prog и отмечает завершение. pid обязан
 * совпадать с постоянным ID, иначе abort; каждый retiring PID освобождает debug credits.
 */
void dvm_managed_workers_guest_retired(int pid)
{
    if (current_guest && current_guest->guest_pid != pid)
    {
        abort();
    }
    retire_debug_requester(pid);
    if (current_guest)
    {
        dvm_diagnostic_close(current_guest->debug);
        current_guest->guest = NULL;
        current_guest->guest_retired = 1;
    }
}


/* EN: Advance readiness generation and signal under service.sync when live. No arguments/result or
 * managed pointer transfer; caller guarantees service lifetime through signal.
 * RU: Увеличить поколение готовности и подать сигнал под service.sync живого сервиса. Нет
 * аргументов, результата и передачи управляемого указателя; вызывающий гарантирует жизнь сервиса
 * до сигнала.
 */
void dvm_managed_workers_wake(void)
{
    if (service.sync)
    {
        fvm_port_sync_lock(service.sync);
        service.generation++;
        fvm_port_sync_signal_all(service.sync);
        fvm_port_sync_unlock(service.sync);
    }
}


/* EN: Signal borrowed live guest's dedicated condition and service generation. Caller pins
 * guest/slot through publication; no execution, ownership transfer or result.
 * RU: Подать сигнал условию живого guest и поколению сервиса. Вызывающий удерживает guest/слот до
 * публикации; исполнения, передачи владения и результата нет.
 */
void dvm_managed_workers_wake_guest(Prog *guest)
{
    ManagedWorker *worker = ((DVM_RuntimeProcess *)guest)->dedicated;
    if (worker)
    {
        fvm_port_sync_lock(worker->wake);
        fvm_port_sync_signal_all(worker->wake);
        fvm_port_sync_unlock(worker->wake);
    }
    dvm_managed_workers_wake();
}


/* EN: Yield a rootless controller interval; idle permits pacing. Return one without a service,
 * otherwise the reaping/wait result. No Prog migration; join waits occur outside admission.
 * RU: Уступить интервал контроллера без корней; idle разрешает паузу. Вернуть единицу без сервиса,
 * иначе результат сборки/ожидания. Prog не переносится; join ожидается вне допуска.
 */
int dvm_managed_workers_yield(int idle)
{
    return !service.sync || yield_empty(idle, 1, NULL);
}


/* EN: Park rootless controller while optional borrowed pending reports no work. Return one without
 * service, otherwise yield result; callback and service outlive wait.
 * RU: Усыпить контроллер без корней, пока заимствованный pending не сообщает работу. Вернуть
 * единицу без сервиса, иначе результат yield; callback и сервис живут до конца ожидания.
 */
int dvm_managed_workers_park(int (*pending)(void))
{
    return !service.sync || yield_empty(1, 0, pending);
}


/* EN: Record one controller turn under service.sync if live. Controller-only, no arguments/result,
 * heap access or lifecycle change.
 * RU: Учесть ход контроллера под service.sync живого сервиса. Только контроллер; нет аргументов,
 * результата, доступа к куче и смены жизненного цикла.
 */
void dvm_managed_workers_record(void)
{
    if (service.sync)
    {
        fvm_port_sync_lock(service.sync);
        service.controller++;
        fvm_port_sync_unlock(service.sync);
    }
}


/* EN: Count one file call under service.sync if live. No arguments/result; no file I/O or heap
 * admission while holding the metadata mutex.
 * RU: Учесть файловый вызов под service.sync живого сервиса. Нет аргументов и результата; файловый
 * I/O и допуск к куче под мьютексом не выполняются.
 */
void dvm_managed_workers_file_call(void)
{
    if (service.sync)
    {
        fvm_port_sync_lock(service.sync);
        service.files++;
        fvm_port_sync_unlock(service.sync);
    }
}


/* EN: Return whether current active turn belongs to a registered helper, counting its UI request
 * under service.sync. No arguments; borrow TLS/slots, reject inactive or foreign owners before
 * scanning, never wait on UI here.
 * RU: Вернуть принадлежность текущего активного хода зарегистрированному потоку, учитывая
 * UI-запрос под service.sync. Без аргументов; заимствовать TLS/слоты, отвергать неактивного/чужого
 * владельца до поиска; UI здесь не ожидать.
 */
int dvm_managed_workers_current(void)
{
    DVM_NativeContext *context = dvm_native_context_current();
    DVM_HeapOwnerStats state;
    ManagedWorker *worker;
    if (!context || !dvm_native_context_turn(context) ||
        !dvm_native_owner_statistics(dvm_native_owner_current(), &state) || state.phase != DVM_HEAP_ACTIVE ||
        dvm_native_owner_world(dvm_native_owner_current()) != service.world)
    {
        return 0;
    }
    int found = 0;
    fvm_port_sync_lock(service.sync);
    for (worker = service.helpers; worker; worker = worker->next)
    {
        if (worker->identity == (uintptr)context)
        {
            service.ui_requests++;
            found = 1;
            break;
        }
    }
    fvm_port_sync_unlock(service.sync);
    return found;
}


/* EN: Controller closes startup, wakes and joins all remaining slots outside admission, then frees
 * the service. diagnostics selects output. Return one without service or on clean join, zero after
 * latched failure; invalid release/join is fatal. Other service callers must finish before return.
 * RU: Контроллер закрывает запуск, будит и присоединяет слоты вне допуска, затем удаляет сервис.
 * diagnostics задаёт вывод. Вернуть единицу без сервиса/при чистом join, ноль при записанной
 * ошибке; отказ выхода/join фатален. Прочие пользователи сервиса обязаны завершиться до возврата.
 */
int dvm_managed_workers_stop(int diagnostics)
{
    int ok;
    ManagedWorker *worker;
    ManagedWorker *next;
    unsigned long turns = 0;
    unsigned long releases = 0;
    unsigned long reentries = 0;
    if (!service.sync)
    {
        return 1;
    }
    fvm_port_sync_lock(service.sync);
    service.stopping = 1;
    stop_dedicated_waits();
    fvm_port_sync_signal_all(service.sync);
    fvm_port_sync_unlock(service.sync);
    if (!leave_empty(DVM_NW_SITE_JOIN))
    {
        fprintf(stderr, "disvm: cannot release controller for managed worker join\n");
        exit(1);
    }
    for (worker = service.helpers; worker; worker = worker->next)
    {
        if (!dvm_thread_registrar_join(service.registrar, worker->thread))
        {
            fprintf(stderr, "disvm: cannot join registered worker\n");
            abort();
        }
        turns += worker->turns;
        releases += worker->owner.releases;
        reentries += worker->owner.reentries;
    }
    ok = !service.failed;
    if (diagnostics)
    {
        fprintf(
            stderr,
            "NATIVEWORKERS threads=%lu joined=1 failed=%d controller_turns=%lu worker_turns=%lu worker_releases=%lu worker_reentries=%lu serialized_files=%lu ui_requests=%lu\n",
            service.peak,
            service.failed,
            service.controller,
            turns + service.retired_turns,
            releases + service.retired_releases,
            reentries + service.retired_reentries,
            service.files,
            service.ui_requests);
    }
    if (diagnostics)
    {
        for (worker = service.helpers; worker; worker = worker->next)
        {
            fprintf(stderr,
                    "NATIVEWORKERS_THREAD id=%lu joined=1 turns=%lu releases=%lu reentries=%lu handoffs=%lu\n",
                    worker->id,
                    worker->turns,
                    worker->owner.releases,
                    worker->owner.reentries,
                    worker->handoffs);
        }
    }
    for (worker = service.helpers; worker; worker = next)
    {
        next = worker->next;
        if (worker->wake)
        {
            fvm_port_sync_destroy(worker->wake);
        }
        dvm_diagnostic_destroy(worker->debug);
        free(worker);
    }
    if (diagnostics)
    {
        dvm_diagnostic_report();
    }
    dvm_thread_registrar_destroy(service.registrar);
    fvm_port_sync_destroy(service.sync);
    memset(&service, 0, sizeof(service));
    return ok;
}


/* EN/RU: Wake exact native PID readiness and controller metadata, without Prog access.
 * Разбудить exact native PID readiness и controller metadata, без Prog access. */
void dvm_managed_workers_debug_wake(int pid)
{
    ManagedWorker *worker;
    if (!service.sync)
    {
        return;
    }
    fvm_port_sync_lock(service.sync);
    for (worker = service.wake_slots; worker; worker = worker->wake_next)
    {
        if (worker->guest_pid == pid)
        {
            fvm_port_sync_lock(worker->wake);
            fvm_port_sync_signal_all(worker->wake);
            fvm_port_sync_unlock(worker->wake);
        }
    }
    service.generation++;
    fvm_port_sync_signal_all(service.sync);
    fvm_port_sync_unlock(service.sync);
}


/* [GC:] EN/RU: Release rootless source admission for idle debug gate.
 * Освободить rootless source admission для idle debug gate. */
int dvm_managed_workers_debug_leave(void)
{
    return leave_empty(DVM_NW_SITE_IDLE);
}


/* [GC:] EN/RU: Reenter the same idle source owner; never restore from Prog.R.
 * Вернуться на тот же idle source owner; не восстанавливать REG из Prog.R. */
void dvm_managed_workers_debug_enter(void)
{
    DVM_NativeExecution *execution = dvm_native_execution();
    execution->world_entry_site = DVM_NW_SITE_IDLE;
    execution->world_entry_pending = 1;
    if (!dvm_native_owner_enter(dvm_native_owner_current()))
    {
        abort();
    }
    execution->world_entry_pending = 0;
}
