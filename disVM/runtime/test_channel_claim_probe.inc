/* Copyright (c) 2026 bhgv
 * [GC:] EN: Actual channel claim loss, unsupported transport and hosted close delivery.
 * RU: Настоящий отказ захвата канала, чужой транспорт и доставка hosted-закрытия.
 */
static Channel *waiting_channel; /* Independently retained real Limbo receive channel, under sync. */


void __real_dvm_runtime_channel_waiter_published(void);


int __real_dvm_runtime_channel_claim_observed(Heap *, Array *, Type **, int, int *);


extern int dvm_runtime_channel_post_close(Channel *channel);


extern int dvm_runtime_channel_is_quiet(Channel *channel);


extern void dvm_runtime_channel_queue_guard(int (*operation)(void *), void *argument);


/* [GC:] EN: A capture borrows the publisher and owns any returned channel
 * reference. Only the actual queue mutex protects process state against wake;
 * the fixture mutex supplies no such permission.
 * RU: Запрос заимствует публикующий процесс и владеет возвращённой ссылкой
 * канала. Состояние процесса от wake защищает лишь настоящий мьютекс очереди,
 * не мьютекс теста. */
typedef struct WaitingChannelCapture
{
    Prog *process;    /* Publisher-owned identity through this callback. */
    Channel *channel; /* Independent strong owner returned on a receive. */
} WaitingChannelCapture;


/* [GC:] EN: Check request's publisher state under queue_guard; the earlier
 * ready-channel send may already have been woken. The fixture's sole receive
 * cannot close until this capture is rooted and published to the parent.
 * Retain before unlocking; root publication follows outside the queue lock.
 * RU: Под queue_guard проверить состояние публикующего request: прежний send
 * ready-канала уже мог пробудиться. Единственный receive теста не закрывается
 * до публикации захвата с корнем родителю. Удержать ссылку до unlock, затем
 * опубликовать корень вне мьютекса очереди. */
static int capture_waiting_channel(void *argument)
{
    WaitingChannelCapture *request = argument;
    if (request->process && request->process->state == Precv)
    {
        Channel *channel = request->process->chan;
        require(channel && channel != H && dvm_refcount_retain_u(&D2H(channel)->ref) == DVM_REFCOUNT_LIVE,
                "real registered receive channel retained under queue mutex");
        request->channel = channel;
    }
    return 1;
}


/* [GC:] EN: Observe the third guest's committed receive and retain its actual
 * channel before parent-side diagnostics. No registration or waiter is invented.
 * RU: Наблюдать опубликованный receive третьего гостя и удержать его настоящий
 * канал для родителя. Регистрация и ожидающий процесс не подменяются. */
void __wrap_dvm_runtime_channel_waiter_published(void)
{
    __real_dvm_runtime_channel_waiter_published();
    if (IsChannel)
    {
        Prog *process = dvm_native_context_process(dvm_native_context_current());
        pthread_mutex_lock(&sync);
        int pending = !waiting_channel;
        pthread_mutex_unlock(&sync);
        if (pending && process && process->pid > 3)
        {
            WaitingChannelCapture request = {process, NULL};
            dvm_runtime_channel_queue_guard(capture_waiting_channel, &request);
            if (request.channel)
            {
                pthread_mutex_lock(&sync);
                if (!waiting_channel)
                {
                    require(dvm_runtime_root_add(D2H(request.channel)), "independent waiter owner gets its root");
                    waiting_channel = request.channel;
                }
                else
                {
                    require(dvm_refcount_drop_nonfinal_u(&D2H(request.channel)->ref),
                            "duplicate capture returns its extra owner");
                }
                pthread_mutex_unlock(&sync);
            }
        }
    }
}


/* [GC:] EN: Fail a genuine hosted-waiter claim, post close to its owner mailbox,
 * and wait under shared admission for that owner to withdraw its real queue
 * registration. The later close sample must be empty; the original reason stays.
 * Release the diagnostic owner through RC before restoring the native caller.
 * RU: Получить настоящий отказ из-за hosted-ожидания, отправить закрытие владельцу
 * и под shared-допуском дождаться снятия им регистрации. Поздняя проверка пуста,
 * но исходная причина сохраняется. Передать диагностическую ссылку в RC до
 * восстановления допуска родителя. */
static void probe_real_channel_waiter(void)
{
    require(dvm_runtime_gc_dispatch_share(), "parent shares admission while real waiter progresses");
    unsigned long long deadline = milliseconds() + 5000;
    Channel *channel = NULL;
    while (milliseconds() < deadline)
    {
        pthread_mutex_lock(&sync);
        channel = waiting_channel;
        pthread_mutex_unlock(&sync);
        if (channel)
        {
            break;
        }
        fvm_port_thread_sleep_us(1000);
    }
    require(channel != NULL, "real third guest committed its channel receive");
    Type *mapped = NULL;
    int retry = 0;
    require(!__real_dvm_runtime_channel_claim_observed(D2H(channel), H, &mapped, 0, &retry) && !mapped,
            "real hosted waiter prevents channel ownership withdrawal");
    if (CONTROL < 121 || CONTROL > 124)
    {
        require(retry, "hosted waiter refusal captured under queue mutex");
    }
    require(dvm_runtime_channel_post_close(channel), "actual close message posted to waiting owner");
    deadline = milliseconds() + 5000;
    int quiet = 0;
    while (milliseconds() < deadline)
    {
        quiet = dvm_runtime_channel_is_quiet(channel);
        if (quiet > 0)
        {
            break;
        }
        fvm_port_thread_sleep_us(1000);
    }
    require(quiet > 0 && !dvm_runtime_channel_post_close(channel), "actual owner removes wait before the later sample");
    require(dvm_runtime_rc_defer(channel) && dvm_runtime_root_remove(D2H(channel)),
            "diagnostic waiter reference and root transfer without a gap");
    dvm_runtime_gc_dispatch_finish();
}


/* [GC:] EN: One short unsupported transport tag protected by the real queue mutex.
 * RU: Короткая метка чужого транспорта под настоящим мьютексом очереди. */
typedef struct ChannelAuxProbe
{
    Channel *channel; /* Independently owned target. */
    void *value;      /* Native unsupported tag, or NULL to restore. */
} ChannelAuxProbe;


/* [GC:] EN: Set only the probe-owned channel auxiliary tag under queue_guard.
 * argument borrows the private request; no descriptor or queue owner changes.
 * RU: Под queue_guard изменить только aux собственного канала теста. argument
 * заимствует личный запрос; дескрипторы и владельцы очередей не меняются. */
static int set_channel_probe_aux(void *argument)
{
    ChannelAuxProbe *probe = argument;
    probe->channel->aux = probe->value;
    return 1;
}

#include "test_channel_transient_probe.inc"


/* [GC:] EN: Observe real quiet-channel claim loss with an independent short
 * owner. First verify that an unsupported transport tag stays a hard refusal.
 * Then call the original claim with a real extra reference/root, require its
 * queue-locked retry, and remove that owner before Fast's later sample. All
 * arguments/results and the mover/buffer owners stay production-owned.
 * RU: Проверить настоящий отказ спокойного канала при коротком независимом
 * владельце. Сначала чужой aux должен дать жёсткий отказ. Затем добавить ссылку
 * и корень, вызвать исходный захват, проверить retry и снять временного владельца
 * до позднего чтения Fast. Аргументы, ответы и mover/буфер принадлежат runtime. */
int __wrap_dvm_runtime_channel_claim_observed(Heap *heap, Array *buffer, Type **mapped, int claimed, int *retry)
{
    if (!IsChannel || !selected || heap != target || races)
    {
        return __real_dvm_runtime_channel_claim_observed(heap, buffer, mapped, claimed, retry);
    }
    if (Case >= 35)
    {
        return probe_channel_transient(heap, buffer, mapped, claimed, retry);
    }
    ChannelAuxProbe probe = {H2D(Channel *, heap), &probe};
    dvm_runtime_channel_queue_guard(set_channel_probe_aux, &probe);
    int unsupported_retry = 0;
    Type *unsupported_mapped = NULL;
    int unsupported =
        __real_dvm_runtime_channel_claim_observed(heap, buffer, &unsupported_mapped, 0, &unsupported_retry);
    probe.value = NULL;
    dvm_runtime_channel_queue_guard(set_channel_probe_aux, &probe);
    require(!unsupported && !unsupported_retry && !unsupported_mapped && dvm_refcount_load_u(&heap->ref) == 1,
            "unsupported transport channel remains a hard refusal");
    require(dvm_refcount_retain_u(&heap->ref) == DVM_REFCOUNT_LIVE && dvm_runtime_root_add(heap),
            "actual temporary channel owner before quiet claim");
    int result = __real_dvm_runtime_channel_claim_observed(heap, buffer, mapped, claimed, retry);
    require(!result && retry && *retry && dvm_refcount_load_u(&heap->ref) == 2,
            "channel claim captures temporary refusal under queue mutex");
    require(dvm_runtime_root_remove(heap) && dvm_refcount_drop_nonfinal_u(&heap->ref) &&
                dvm_refcount_load_u(&heap->ref) == 1,
            "temporary channel owner ends before later retry sampling");
    pthread_mutex_lock(&sync);
    races++;
    pthread_mutex_unlock(&sync);
    failure_seen = 1;
    return result;
}
