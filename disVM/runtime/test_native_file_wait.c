/* Copyright (c) 2026 bhgv
 * EN: Observe real same-FD FIFO sleep while a native port write is held.
 * RU: Наблюдать настоящее FIFO-ожидание того же FD при задержанном port write.
 */
#include "file_gate.h"
#include "file_port.h"
#include "channel_sync.h"
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static pthread_mutex_t probe = PTHREAD_MUTEX_INITIALIZER; /* Native test evidence only. */
static DVM_FileGate *head_gate;                           /* Borrowed through the held port write and joined guests. */
static DVM_FileGate *waiting_gate;                        /* Borrowed through an actual FIFO condition wait. */
static int reported;                                      /* Same-gate evidence emitted once, under probe. */
static int held;                                          /* Atomic one-time native write barrier. */
static __thread DVM_FileGate *entering;                   /* This owner's current gate admission call. */
static __thread DVM_FileGate *admitted;                   /* This owner's last successful non-stdio gate. */


/* EN: Original native interfaces; borrowed arguments keep their production roles.
 * RU: Исходные native-интерфейсы; роли и владение аргументами не меняются. */
int __real_dvm_file_gate_enter(DVM_FileGate *gate, DVM_FileTicket *ticket);


void __real_fvm_port_sync_wait(FVM_PortSync *sync);


FVM_Result __real_fvm_port_file_write(FVM_PortFile *file, const void *bytes, size_t size);


/* EN: Record borrowed gate identity for head/waiter, selected by waiting. Emit
 * once only when both are the same non-NULL gate. No VM memory or gate mutex is
 * accessed; this native test mutex cannot serialize guest execution globally.
 * RU: Сохранить заимствованный gate головы/ожидающего по waiting. Сообщить один
 * раз лишь о совпадении ненулевых gate. Память VM и mutex gate не читаются;
 * частный native-mutex не сериализует исполнение гостей. */
static void observe(DVM_FileGate *gate, int waiting)
{
    pthread_mutex_lock(&probe);
    if (waiting)
    {
        waiting_gate = gate;
    }
    else
    {
        head_gate = gate;
    }
    if (head_gate && head_gate == waiting_gate && !reported)
    {
        reported = 1;
        fprintf(stderr, "FILE_FIFO_BLOCKED same_gate=1\n");
    }
    pthread_mutex_unlock(&probe);
}


/* EN: Observe this thread's borrowed gate/ticket around real FIFO admission.
 * Preserve its exact result and blocking; no managed resource is acquired.
 * RU: Наблюдать gate/ticket своего потока вокруг настоящего допуска FIFO.
 * Сохранить результат и ожидание; managed-ресурсы не захватываются. */
int __wrap_dvm_file_gate_enter(DVM_FileGate *gate, DVM_FileTicket *ticket)
{
    entering = gate;
    int result = __real_dvm_file_gate_enter(gate, ticket);
    entering = NULL;
    if (result && gate)
    {
        admitted = gate;
    }
    return result;
}


/* EN: Observe an actual gate condition-wait immediately before calling it.
 * sync stays borrowed and locked under the real port contract; other waits pass
 * through unchanged. Reporting neither skips nor substitutes the real wait.
 * RU: Наблюдать настоящий condition-wait gate непосредственно перед вызовом.
 * sync заимствован и захвачен по исходному контракту; иные ожидания неизменны.
 * Сообщение не пропускает и не подменяет настоящее ожидание. */
void __wrap_fvm_port_sync_wait(FVM_PortSync *sync)
{
    if (entering)
    {
        observe(entering, 1);
    }
    __real_fvm_port_sync_wait(sync);
}


/* EN: Hold only the first real file write in an inherited pipe read. The file,
 * bytes and size keep their native ownership through the genuine port call.
 * Announce the held head before waiting; no VM state is read outside admission.
 * Return the exact port result. A broken test pipe exits 10. This is a controlled
 * scheduling boundary, not a claim of measured slow disk behavior.
 * RU: Задержать первую настоящую файловую запись на чтении унаследованного pipe.
 * Владение file/bytes/size сохраняется до настоящего port-вызова. Сообщить о
 * голове до ожидания; вне допуска поля VM не читаются. Вернуть исходный результат.
 * Нарушение pipe завершает с 10. Это управляемая граница, не замер медленного диска. */
FVM_Result __wrap_fvm_port_file_write(FVM_PortFile *file, const void *bytes, size_t size)
{
    if (!__atomic_exchange_n(&held, 1, __ATOMIC_RELAXED))
    {
        const char *selection = getenv("DVM_TEST_FILE_BARRIER");
        char permit;
        ssize_t received;
        if (!selection || !admitted)
        {
            fprintf(stderr, "FAIL file barrier: missing pipe or admitted gate\n");
            exit(10);
        }
        observe(admitted, 0);
        fprintf(stderr, "FILE_NATIVE_BLOCKED\n");
        do
        {
            received = read(atoi(selection), &permit, 1);
        } while (received < 0 && errno == EINTR);
        if (received != 1 || permit != '1')
        {
            fprintf(stderr, "FAIL file barrier: invalid release\n");
            exit(10);
        }
    }
    return __real_fvm_port_file_write(file, bytes, size);
}
