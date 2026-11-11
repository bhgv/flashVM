/* Copyright (c) 2026 bhgv
 * EN: Real native FIFO admission racing queued cancellation and active completion.
 * RU: Нативная FIFO допуска при гонке отмены очереди и завершения активной операции.
 */
#include "file_gate.h"
#include "disvm/native_context.h"
#include "thread_port.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define WORKERS 32 /* Native contenders per independently owned gate. */
#define WAVES 16 /* Complete create/cancel/drain/destroy lifetimes. */


/* EN: Caller-owned records, protected by sync or thread join as documented.
 * RU: Записи вызывающего; поля защищены sync либо завершением через join. */
typedef struct Wave
{
    DVM_FileGate *gate; /* Borrowed by workers/canceller until all joins. */
    DVM_FileTicket tickets[WORKERS]; /* Stable storage through worker/canceller join. */
    pthread_mutex_t sync; /* Protects handshake and admission observations. */
    pthread_cond_t changed; /* Predicate notification, never a timing oracle. */
    int first_cancelled; /* First queued ticket removed before controller leave. */
    int go; /* Controller allows cancellation/admission to race. */
    int active; /* Handshake phase: workers zero/ four have actually entered. */
    int active_checked; /* Last native admission phase checked by the canceller. */
    int order[WORKERS]; /* Observed admission order, written under sync. */
    int count; /* Number of admitted worker tickets. */
    int cancelled[WORKERS]; /* Canceller-owned outcomes; read only after join. */
    int admitted[WORKERS]; /* Each worker owns its slot; read only after join. */
} Wave;


/* EN: Borrowed wave and identity for one worker, stable through join.
 * RU: Заимствованные wave и номер работника, живые до join. */
typedef struct Worker
{
    Wave *wave; /* Shared native gate and handshake storage. */
    int index; /* One unique ticket and output slot. */
} Worker;


/* EN: Terminate with a specific semantic diagnostic on condition failure.
 * RU: Завершить с точной диагностикой при ложном condition. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL file gate churn: %s\n", message);
        exit(1);
    }
}


/* EN: Bind a native context, await FIFO admission, then complete exactly once.
 * argument borrows Worker until join. Workers zero and four hold their
 * admissions across cancellation checks; no gate mutex
 * is held during those native waits.
 * RU: Привязать контекст, получить допуск FIFO и завершить ровно один раз.
 * argument заимствует Worker до join. Работник ноль держит активную операцию
 * до проверки отмены, не удерживая при этом мьютекс шлюза.
 */
static void execute(void *argument)
{
    Worker *worker = argument;
    Wave *wave = worker->wave;
    int index = worker->index;
    DVM_NativeContext *context = dvm_native_context_create(8, NULL);
    require(context && dvm_native_context_bind(context), "worker context binds");
    int admitted = dvm_file_gate_enter(wave->gate, &wave->tickets[index]);
    wave->admitted[index] = admitted;
    if (admitted)
    {
        pthread_mutex_lock(&wave->sync);
        wave->order[wave->count++] = index;
        if (!index || index == 4)
        {
            int phase = index ? 2 : 1;
            wave->active = phase;
            pthread_cond_broadcast(&wave->changed);
            while (wave->active_checked < phase)
            {
                pthread_cond_wait(&wave->changed, &wave->sync);
            }
        }
        pthread_mutex_unlock(&wave->sync);
        require(dvm_file_gate_leave(wave->gate, &wave->tickets[index]), "admitted worker completes once");
        require(!dvm_file_gate_leave(wave->gate, &wave->tickets[index]), "duplicate completion is refused");
    }
    require(dvm_native_context_unbind(context) && dvm_native_context_destroy(context), "worker context retires");
}


/* EN: Cancel a known queued ticket, refuse active cancellation, then race peers.
 * argument borrows Wave until join; cancelled[] is written only on this thread.
 * RU: Отменить заведомо ожидающий билет, отказать отмене активного, затем
 * конкурировать с допуском остальных. Wave живёт до join; cancelled[] пишет
 * только этот поток.
 */
static void cancel_queued(void *argument)
{
    Wave *wave = argument;
    wave->cancelled[1] = dvm_file_gate_cancel(wave->gate, &wave->tickets[1]);
    require(wave->cancelled[1], "queued ticket cancellation succeeds before release");
    pthread_mutex_lock(&wave->sync);
    wave->first_cancelled = 1;
    pthread_cond_broadcast(&wave->changed);
    while (!wave->go || !wave->active)
    {
        pthread_cond_wait(&wave->changed, &wave->sync);
    }
    pthread_mutex_unlock(&wave->sync);
    require(!dvm_file_gate_cancel(wave->gate, &wave->tickets[0]), "active operation cannot be cancelled by its FIFO");
    pthread_mutex_lock(&wave->sync);
    wave->active_checked = 1;
    pthread_cond_broadcast(&wave->changed);
    while (wave->active < 2)
    {
        pthread_cond_wait(&wave->changed, &wave->sync);
    }
    pthread_mutex_unlock(&wave->sync);
    wave->cancelled[3] = dvm_file_gate_cancel(wave->gate, &wave->tickets[3]);
    require(!wave->cancelled[3], "completed operation cannot be cancelled again");
    require(!dvm_file_gate_cancel(wave->gate, &wave->tickets[4]), "second active operation cannot be cancelled by its FIFO");
    pthread_mutex_lock(&wave->sync);
    wave->active_checked = 2;
    pthread_cond_broadcast(&wave->changed);
    pthread_mutex_unlock(&wave->sync);
    for (int index = 5; index < WORKERS; index += 2)
    {
        wave->cancelled[index] = dvm_file_gate_cancel(wave->gate, &wave->tickets[index]);
    }
}


/* EN: Own one gate, stable ticket array and all threads through final retirement.
 * Return the number of accepted cancellations; any contract violation terminates.
 * RU: Владеть шлюзом, билетами и потоками до окончательного закрытия.
 * Вернуть число принятых отмен; нарушение контракта завершает тест.
 */
static unsigned run_wave(void)
{
    Wave wave = {0};
    Worker workers[WORKERS];
    FVM_PortThread *threads[WORKERS];
    DVM_FileTicket blocker;
    unsigned cancelled = 0;
    require(!pthread_mutex_init(&wave.sync, NULL) && !pthread_cond_init(&wave.changed, NULL), "handshake initializes");
    wave.gate = dvm_file_gate_create();
    require(wave.gate != NULL, "gate allocated");
    dvm_file_gate_reserve(wave.gate, &blocker);
    require(dvm_file_gate_enter(wave.gate, &blocker), "controller holds first operation");
    for (int index = 0; index < WORKERS; index++)
    {
        dvm_file_gate_reserve(wave.gate, &wave.tickets[index]);
        workers[index].wave = &wave;
        workers[index].index = index;
    }
    for (int index = WORKERS - 1; index >= 0; index--)
    {
        threads[index] = fvm_port_thread_start(execute, &workers[index]);
        require(threads[index] != NULL, "worker starts");
    }
    FVM_PortThread *canceller = fvm_port_thread_start(cancel_queued, &wave);
    require(canceller != NULL, "canceller starts");
    pthread_mutex_lock(&wave.sync);
    while (!wave.first_cancelled)
    {
        pthread_cond_wait(&wave.changed, &wave.sync);
    }
    wave.go = 1;
    pthread_cond_broadcast(&wave.changed);
    pthread_mutex_unlock(&wave.sync);
    require(!dvm_file_gate_destroy(wave.gate), "busy gate stays live during cancellation");
    require(dvm_file_gate_leave(wave.gate, &blocker), "controller releases the FIFO");
    fvm_port_thread_join(canceller);
    for (int index = 0; index < WORKERS; index++)
    {
        fvm_port_thread_join(threads[index]);
        require(wave.admitted[index] + wave.cancelled[index] == 1, "each request is admitted or cancelled exactly once");
        cancelled += (unsigned)wave.cancelled[index];
    }
    int previous = -1;
    for (int index = 0; index < wave.count; index++)
    {
        require(wave.order[index] > previous, "surviving requests retain FIFO order");
        previous = wave.order[index];
    }
    size_t pending = 0;
    dvm_file_gate_snapshot(wave.gate, &pending, NULL);
    require(!pending && wave.count + (int)cancelled == WORKERS, "all requests drained before retirement");
    require(dvm_file_gate_destroy(wave.gate), "empty gate retires after all joins");
    require(!pthread_cond_destroy(&wave.changed) && !pthread_mutex_destroy(&wave.sync), "handshake retires");
    return cancelled;
}


/* EN: Exercise 512 requests and exact context/gate retirement with real OS threads.
 * No arguments; return zero on success. No managed allocation or heap is mocked.
 * RU: Проверить 512 запросов и очистку контекстов/шлюзов настоящими OS-потоками.
 * Без аргументов; ноль при успехе. Управляемая память и куча не подменяются.
 */
int main(void)
{
    DVM_NativeContext *context = dvm_native_context_create(8, NULL);
    require(context && dvm_native_context_bind(context), "controller context binds");
    unsigned cancelled = 0;
    for (unsigned wave = 0; wave < WAVES; wave++)
    {
        cancelled += run_wave();
    }
    require(dvm_native_context_unbind(context) && dvm_native_context_destroy(context), "controller context retires");
    printf("PASS file gate churn requests=%u cancelled=%u active-refusals=%u FIFO=1 drained=1\n", WORKERS * WAVES, cancelled, WAVES * 2);
    return 0;
}
