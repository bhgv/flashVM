/* Copyright (c) 2026 bhgv
 * EN: Real native FIFO cancellation from an unbound host thread at exact admission.
 * RU: Отмена настоящей нативной FIFO непривязанным потоком на границе допуска.
 */
#include "lib9.h"
#include "dat.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "disvm/native_context.h"
#include "file_gate.h"
#include "channel_sync.h"
#include "file_port.h"
#include "filesystem_port.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static pthread_mutex_t sync = PTHREAD_MUTEX_INITIALIZER; /* Native fixture predicate lock. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Head release notification. */
static DVM_FileGate *head_gate; /* Borrowed through the admitted head's rooted FD. */
static int head_active; /* Head is inside the controlled native I/O barrier. */
static int tail_sleeping; /* Queued enter reached its real port condition wait, under sync. */
static _Thread_local int entering; /* Only the actual file-gate enter scope of this resident. */
static int released; /* Tail has cancelled and retired its native wait record. */
static unsigned heads; /* Completed barrier announcements, protected by sync. */
static _Thread_local DVM_FileGate *current_gate; /* Current call's borrowed admission gate. */
static _Thread_local int observed_cancel; /* Adapter scope observed a real cancelled admission. */
static _Thread_local int cancelling; /* This owner's admission belongs to the queued tail. */

int __real_dvm_file_gate_try_release(DVM_FileGate *, DVM_FileTicket *);
int __real_dvm_file_gate_enter(DVM_FileGate *, DVM_FileTicket *);
void __real_fvm_port_sync_wait(FVM_PortSync *);
FVM_Result __real_fvm_port_file_write(FVM_PortFile *, const void *, size_t);
FVM_Result __real_fvm_port_directory_read(FVM_PortDirectory *, FVM_FileInfo **, size_t *);
FVM_Result __real_dvm_metadata_fd_info(Sys_FD *, int, FVM_FileInfo *);
FVM_Result __real_dvm_metadata_directory_read(Sys_FD *, int, FVM_FileInfo **, size_t *, int (*)(void *), void *);
int dvm_runtime_request_guest_cancel(int);
int dvm_runtime_cancel_guest_wait(int);


/* EN: Fail with a precise native diagnostic; message is borrowed static text.
 * RU: Завершить с точной нативной диагностикой; message — заимствованный текст. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL native cancellation: %s\n", message);
        exit(11);
    }
}


/* EN: Hold an admitted native call without a gate/VM mutex; controller can progress.
 * No parameters/result. Lifetime is retained by the real caller's FD root.
 * RU: Удерживать допущенный вызов без мьютекса шлюза/VM, давая контроллеру ход.
 * Без параметров/результата. Жизнь держится настоящим корнем FD вызывающего. */
static void hold_head(void)
{
    pthread_mutex_lock(&sync);
    require(!head_active && current_gate != NULL, "one actual admitted head owns this phase");
    head_active = 1;
    released = 0;
    tail_sleeping = 0;
    head_gate = current_gate;
    fprintf(stderr, "NATIVE_CANCEL_HEAD %u\n", ++heads);
    while (!released)
    {
        pthread_cond_wait(&changed, &sync);
    }
    head_active = 0;
    head_gate = NULL;
    pthread_mutex_unlock(&sync);
}


/* EN: Cancel the exact registered PID from an actual unbound native thread.
 * argument borrows an int through join. Duplicate delivery coalesces while linked.
 * RU: Отменить точный зарегистрированный PID настоящим непривязанным потоком.
 * argument заимствует int до join. Повтор объединяется, пока запись связана. */
static void *cancel_tail(void *argument)
{
    int pid = *(int *)argument;
    if (CONTROL)
    {
        pthread_mutex_lock(&sync);
        while (!tail_sleeping)
        {
            pthread_cond_wait(&changed, &sync);
        }
        pthread_mutex_unlock(&sync);
    }
    require(dvm_native_context_current() == NULL, "host cancellation has no bound Prog");
    require(dvm_runtime_request_guest_cancel(pid), "registered queued operation accepts cancellation");
    require(dvm_runtime_request_guest_cancel(pid), "duplicate linked cancellation coalesces");
    return NULL;
}


/* EN: Coordinate host cancellation before enter or at its actual condition wait.
 * Forward the real gate/ticket once; no alternative admission is introduced.
 * RU: Согласовать отмену перед enter либо на его настоящем condition wait.
 * Передать настоящие gate/ticket один раз, без другого алгоритма допуска. */
int __wrap_dvm_file_gate_enter(DVM_FileGate *gate, DVM_FileTicket *ticket)
{
    if (!cancelling)
    {
        return __real_dvm_file_gate_enter(gate, ticket);
    }
    Prog *process = dvm_native_context_process(dvm_native_context_current());
    int pid = process->pid;
    pthread_t thread;
    require(!pthread_create(&thread, NULL, cancel_tail, &pid), "host cancellation thread starts");
    if (!CONTROL)
    {
        require(!pthread_join(thread, NULL), "early host cancellation thread joins");
    }
    entering = 1;
    int admitted = __real_dvm_file_gate_enter(gate, ticket);
    entering = 0;
    if (CONTROL)
    {
        require(!pthread_join(thread, NULL), "parked host cancellation thread joins");
    }
    return admitted;
}


/* EN: Announce the actual gate wait, then forward its real atomic unlock/sleep.
 * sync_port remains owned by production. The host releases fixture sync before
 * acquiring cancellation/gate locks, so no fixture-to-gate lock cycle exists.
 * RU: Сообщить о настоящем ожидании шлюза и выполнить его unlock/sleep.
 * sync_port принадлежит runtime. Хост снимает мьютекс теста до захвата отмены
 * и шлюза, поэтому цикла блокировок тест-шлюз нет. */
void __wrap_fvm_port_sync_wait(FVM_PortSync *sync_port)
{
    if (CONTROL && entering && cancelling)
    {
        pthread_mutex_lock(&sync);
        tail_sleeping = 1;
        pthread_cond_broadcast(&changed);
        pthread_mutex_unlock(&sync);
    }
    __real_fvm_port_sync_wait(sync_port);
}


/* EN: Capture this call's gate and require cancelled admission to retire exactly once.
 * Only the head's queued peer releases its barrier; actual VM code stays intact.
 * RU: Захватить gate вызова и проверить однократное снятие отменённого ожидания.
 * Только ожидающий сосед головы снимает барьер; настоящий код VM не заменяется. */
int __wrap_dvm_file_gate_try_release(DVM_FileGate *gate, DVM_FileTicket *ticket)
{
    current_gate = gate;
    pthread_mutex_lock(&sync);
    cancelling = head_active && gate == head_gate;
    pthread_mutex_unlock(&sync);
    int result = __real_dvm_file_gate_try_release(gate, ticket);
    if (cancelling)
    {
        Prog *process = dvm_native_context_process(dvm_native_context_current());
        require(result == -2, "queued cancellation is distinct from resource refusal");
        observed_cancel = 1;
        require(!dvm_runtime_cancel_guest_wait(process->pid), "completed native cancellation record is gone");
        pthread_mutex_lock(&sync);
        released = 1;
        pthread_cond_broadcast(&changed);
        pthread_mutex_unlock(&sync);
        cancelling = 0;
    }
    return result;
}


/* EN: Hold only the admitted one-byte 'a' operation; forward genuine file writes.
 * file/bytes remain borrowed and stable until the actual port returns.
 * RU: Задержать только допущенную запись 'a'; выполнить настоящий файловый порт.
 * file/bytes заимствованы и стабильны до возврата настоящего порта. */
FVM_Result __wrap_fvm_port_file_write(FVM_PortFile *file, const void *bytes, size_t size)
{
    if (size == 1 && *(const char *)bytes == 'a')
    {
        hold_head();
    }
    return __real_fvm_port_file_write(file, bytes, size);
}


/* EN: Hold child-owned directory reads, forwarding their real native batch.
 * Root-controller recovery reads are not held; borrowed outputs follow port rules.
 * RU: Задержать чтение каталога ребёнком, затем получить настоящий batch.
 * Контроллер не задерживается; заимствованные outputs следуют правилам порта. */
FVM_Result __wrap_fvm_port_directory_read(FVM_PortDirectory *directory, FVM_FileInfo **entries, size_t *count)
{
    Prog *process = dvm_native_context_process(dvm_native_context_current());
    if (process && process->pid > 1)
    {
        hold_head();
    }
    return __real_fvm_port_directory_read(directory, entries, count);
}


/* EN: Audit the actual metadata return classification after a registered cancel.
 * fd/descriptor/output follow the production adapter's borrowed/rooted contract.
 * RU: Проверить настоящий статус метаданных после зарегистрированной отмены.
 * fd/descriptor/output следуют контракту настоящего адаптера и корней. */
FVM_Result __wrap_dvm_metadata_fd_info(Sys_FD *fd, int descriptor, FVM_FileInfo *output)
{
    observed_cancel = 0;
    FVM_Result result = __real_dvm_metadata_fd_info(fd, descriptor, output);
    if (observed_cancel)
    {
        require(result == FVM_ERROR, "cancelled metadata must not be reported as out of memory");
    }
    return result;
}


/* EN: Audit cancelled directory admission without replacing publication or ports.
 * Borrow fd/descriptor/entries/count/publish/argument exactly as the real adapter.
 * RU: Проверить отменённый допуск каталога без подмены публикации или портов.
 * fd/descriptor/entries/count/publish/argument заимствованы по контракту адаптера. */
FVM_Result __wrap_dvm_metadata_directory_read(Sys_FD *fd, int descriptor, FVM_FileInfo **entries,
    size_t *count, int (*publish)(void *), void *argument)
{
    observed_cancel = 0;
    FVM_Result result = __real_dvm_metadata_directory_read(fd, descriptor, entries, count, publish, argument);
    if (observed_cancel)
    {
        require(result == FVM_ERROR, "cancelled directory read must not be reported as out of memory");
    }
    return result;
}
