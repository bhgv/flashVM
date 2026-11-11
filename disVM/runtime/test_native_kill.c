/* Copyright (c) 2026 bhgv
 * EN: Hold real native port calls until a queued/admitted owner's kill is observed.
 * RU: Удерживать настоящие портовые вызовы до kill ожидающего/допущенного владельца.
 */
#define _GNU_SOURCE
#include "lib9.h"
#include "dat.h"
#include "interp.h"
#include "disvm/native_context.h"
#include "channel_sync.h"
#include "file_gate.h"
#include "file_port.h"
#include "filesystem_port.h"
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static pthread_mutex_t sync = PTHREAD_MUTEX_INITIALIZER; /* Protect native barrier state. */
static DVM_FileGate *head_gate; /* Borrowed through the real admitted caller's FD root. */
static FVM_PortFile *held_file; /* Native file borrowed until the real port returns. */
static FVM_PortDirectory *held_directory; /* Native directory borrowed until port return. */
static unsigned heads; /* Numbered native barriers, protected by sync. */
static unsigned kills; /* Accepted parent killwait requests, protected by sync. */
static _Thread_local DVM_FileGate *current_gate; /* This native call's borrowed cursor gate. */
static _Thread_local int entering; /* True only around real file-gate enter. */
static _Thread_local int announced; /* Avoid duplicate markers on spurious condition wakeups. */

int __real_dvm_file_gate_try_release(DVM_FileGate *, DVM_FileTicket *);
int __real_dvm_file_gate_enter(DVM_FileGate *, DVM_FileTicket *);
void __real_fvm_port_sync_wait(FVM_PortSync *);
int __real_dvm_runtime_prepare_killwait(Prog *, int);
void __real_fvm_port_file_close(FVM_PortFile *);
void __real_fvm_port_directory_close(FVM_PortDirectory *);
FVM_Result __real_fvm_port_file_write(FVM_PortFile *, const void *, size_t);
FVM_Result __real_fvm_port_file_read(FVM_PortFile *, void *, size_t, size_t *);
FVM_Result __real_fvm_port_file_seek(FVM_PortFile *, long, int, long *);
FVM_Result __real_fvm_port_file_info(FVM_PortFile *, FVM_FileInfo *);
FVM_Result __real_fvm_port_directory_info(FVM_PortDirectory *, FVM_FileInfo *);
FVM_Result __real_fvm_port_directory_read(FVM_PortDirectory *, FVM_FileInfo **, size_t *);


/* EN: Reject a fixture protocol violation with a precise native diagnostic.
 * RU: Завершить при нарушении протокола с точной нативной диагностикой. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL native kill: %s\n", message);
        exit(12);
    }
}


/* EN: Hold child-owned I/O, with no production mutex held, on an inherited pipe.
 * file/directory borrow the actual handle; return 1 only for held child calls.
 * Root/controller calls pass through; real handles/outputs
 * stay borrowed and rooted by the production adapter until its port returns.
 * RU: Держать I/O ребёнка на унаследованном pipe без мьютексов runtime.
 * file/directory заимствуют handle; вернуть 1 только для задержанного ребёнка.
 * Контроллер проходит; handles/outputs удерживаются
 * настоящим адаптером до возврата порта. */
static int hold_io(FVM_PortFile *file, FVM_PortDirectory *directory)
{
    Prog *process = dvm_native_context_process(dvm_native_context_current());
    if (!process || process->pid <= 1)
    {
        return 0;
    }
    pthread_mutex_lock(&sync);
    require(!head_gate && current_gate, "one admitted native caller owns the barrier");
    head_gate = current_gate;
    held_file = file;
    held_directory = directory;
    fprintf(stderr, "NATIVE_KILL_IO %u\n", ++heads);
    pthread_mutex_unlock(&sync);
    const char *descriptor = getenv("DVM_NATIVE_KILL_RELEASE");
    require(descriptor != NULL, "inherited native release pipe exists");
    char permit;
    ssize_t count;
    do
    {
        count = read(atoi(descriptor), &permit, 1);
    } while (count < 0 && errno == EINTR);
    require(count == 1 && permit == '1', "native release follows the exact kill boundary");
    pthread_mutex_lock(&sync);
    head_gate = NULL;
    pthread_mutex_unlock(&sync);
    return 1;
}


/* EN: Release the port's actual handle borrow after the real operation returned.
 * held is this call's result from hold_io. Root port calls retain no test borrow.
 * RU: Снять заимствование handle после возврата настоящей портовой операции.
 * held — результат hold_io этого вызова. Контроллер не держит тестовый handle. */
static void finish_io(int held)
{
    if (held)
    {
        pthread_mutex_lock(&sync);
        held_file = NULL;
        held_directory = NULL;
        pthread_mutex_unlock(&sync);
    }
}


/* EN: Capture gate for the production call without changing its admission.
 * RU: Захватить gate настоящего вызова, не меняя его допуск. */
int __wrap_dvm_file_gate_try_release(DVM_FileGate *gate, DVM_FileTicket *ticket)
{
    current_gate = gate;
    return __real_dvm_file_gate_try_release(gate, ticket);
}


/* EN: Scope the real enter so only an actual queued gate wait is announced.
 * gate/ticket follow production ownership; return its genuine result once.
 * RU: Отметить настоящий enter для наблюдения ожидания шлюза.
 * gate/ticket следуют владению runtime; вернуть его настоящий результат один раз. */
int __wrap_dvm_file_gate_enter(DVM_FileGate *gate, DVM_FileTicket *ticket)
{
    entering = 1;
    announced = 0;
    int result = __real_dvm_file_gate_enter(gate, ticket);
    entering = 0;
    return result;
}


/* EN: Announce the queued resident at its atomic unlock/sleep boundary.
 * Forward borrowed port_sync untouched. The host then permits the parent kill.
 * RU: Сообщить об ожидающем потоке на границе atomic unlock/sleep.
 * Передать заимствованный port_sync без изменений; хост разрешает kill родителю. */
void __wrap_fvm_port_sync_wait(FVM_PortSync *port_sync)
{
    if (entering && !announced)
    {
        pthread_mutex_lock(&sync);
        if (head_gate == current_gate)
        {
            fprintf(stderr, "NATIVE_KILL_QUEUED %u\n", heads);
            announced = 1;
        }
        pthread_mutex_unlock(&sync);
    }
    __real_fvm_port_sync_wait(port_sync);
}


/* EN: Announce an accepted parent retirement wait before native I/O is released.
 * caller/pid remain borrowed; use the real process mailbox/authorization path.
 * RU: Сообщить о принятом ожидании завершения до разрешения I/O.
 * caller/pid заимствованы; использовать настоящий ящик и проверку родительства. */
int __wrap_dvm_runtime_prepare_killwait(Prog *caller, int pid)
{
    int result = __real_dvm_runtime_prepare_killwait(caller, pid);
    if (!result)
    {
        pthread_mutex_lock(&sync);
        fprintf(stderr, "NATIVE_KILL_WAIT %u\n", ++kills);
        pthread_mutex_unlock(&sync);
    }
    return result;
}


/* EN: Hold then forward borrowed file/bytes/size to the actual write port.
 * RU: Задержать, затем передать file/bytes/size настоящему порту записи. */
FVM_Result __wrap_fvm_port_file_write(FVM_PortFile *file, const void *bytes, size_t size)
{
    int held = hold_io(file, NULL);
    FVM_Result result = __real_fvm_port_file_write(file, bytes, size);
    finish_io(held);
    return result;
}


/* EN: Hold then forward actual input into the adapter's native bytes/received.
 * RU: Задержать и выполнить настоящий ввод в нативные bytes/received адаптера. */
FVM_Result __wrap_fvm_port_file_read(FVM_PortFile *file, void *bytes, size_t size, size_t *received)
{
    int held = hold_io(file, NULL);
    FVM_Result result = __real_fvm_port_file_read(file, bytes, size, received);
    finish_io(held);
    return result;
}


/* EN: Hold then forward the borrowed file and copied seek offset/origin/output.
 * RU: Задержать и передать file и скопированные offset/origin/output настоящему seek. */
FVM_Result __wrap_fvm_port_file_seek(FVM_PortFile *file, long offset, int origin, long *position)
{
    int held = hold_io(file, NULL);
    FVM_Result result = __real_fvm_port_file_seek(file, offset, origin, position);
    finish_io(held);
    return result;
}


/* EN: Hold then forward file/output to the actual metadata port.
 * RU: Задержать и передать file/output настоящему порту метаданных. */
FVM_Result __wrap_fvm_port_file_info(FVM_PortFile *file, FVM_FileInfo *output)
{
    int held = hold_io(file, NULL);
    FVM_Result result = __real_fvm_port_file_info(file, output);
    finish_io(held);
    return result;
}


/* EN: Hold then forward borrowed directory/output to the actual metadata port.
 * RU: Задержать и передать directory/output настоящему порту метаданных. */
FVM_Result __wrap_fvm_port_directory_info(FVM_PortDirectory *directory, FVM_FileInfo *output)
{
    int held = hold_io(NULL, directory);
    FVM_Result result = __real_fvm_port_directory_info(directory, output);
    finish_io(held);
    return result;
}


/* EN: Hold then forward directory/entries/count to the actual cursor operation.
 * RU: Задержать и передать directory/entries/count настоящей операции курсора. */
FVM_Result __wrap_fvm_port_directory_read(FVM_PortDirectory *directory, FVM_FileInfo **entries, size_t *count)
{
    int held = hold_io(NULL, directory);
    FVM_Result result = __real_fvm_port_directory_read(directory, entries, count);
    finish_io(held);
    return result;
}


/* EN: Refuse an actual close while a child still borrows its file inside a port.
 * file follows production close ownership; forward NULL and unborrowed handles.
 * RU: Запретить закрытие, пока ребёнок заимствует файл внутри порта.
 * file следует владению настоящего close; передать NULL и свободные handles. */
void __wrap_fvm_port_file_close(FVM_PortFile *file)
{
    pthread_mutex_lock(&sync);
    require(!file || file != held_file, "file closed before its native operation returned");
    pthread_mutex_unlock(&sync);
    __real_fvm_port_file_close(file);
}


/* EN: Refuse actual directory close before the native operation returns.
 * directory follows production close ownership; the fixture never closes it itself.
 * RU: Запретить close каталога до возврата его нативной операции.
 * directory следует настоящему владению; тест сам его не закрывает. */
void __wrap_fvm_port_directory_close(FVM_PortDirectory *directory)
{
    pthread_mutex_lock(&sync);
    require(!directory || directory != held_directory, "directory closed before its native operation returned");
    pthread_mutex_unlock(&sync);
    __real_fvm_port_directory_close(directory);
}
