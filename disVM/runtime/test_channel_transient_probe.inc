/* Copyright (c) 2026 bhgv
 * [GC:] EN: Real channel match reservations and guarded buffer identity changes.
 * RU: Настоящие резервации сопоставления канала и смена буфера под guard.
 */


/* [GC:] EN: Own a stack match or a temporarily moved buffer reference. The
 * channel stays independently owned; owner is a live native peer identity.
 * RU: Владеть стековой резервацией либо временно переданной ссылкой буфера.
 * Канал удерживается независимо; owner — личность живого native-соседа. */
typedef struct ChannelTransientProbe
{
    Channel *channel;         /* Independently owned target. */
    Array *buffer;            /* Captured buffer whose field owner may temporarily move here. */
    DVM_ChannelMatch match;   /* Real reservation ledger, never a forged busy result. */
    DVM_NativeContext *owner; /* Borrowed peer context, alive through test completion. */
    int restore;              /* End the staged operation when nonzero. */
} ChannelTransientProbe;


/* [GC:] EN: Begin/end a real match or move/restore the buffer field under the
 * actual queue guard. request owns all temporary storage and reference transfers.
 * No payload mover runs, no owner count is forged and no peer is executed here.
 * RU: Под настоящим guard начать/снять match либо передать/вернуть поле буфера.
 * request владеет рабочей памятью и передаваемыми ссылками. Перемещение данных
 * не выполняется, счётчики не подменяются, чужой Prog здесь не исполняется. */
static int stage_channel_transient(void *argument)
{
    ChannelTransientProbe *request = argument;
    if (Case == 35 || Case == 36)
    {
        if (request->restore)
        {
            dvm_channel_match_end(&request->match);
        }
        else
        {
            require(dvm_channel_match_begin(&request->match, request->channel, request->owner),
                    "real conflicting native channel match reservation");
        }
    }
    else if (request->restore)
    {
        require(request->channel->buf == H, "changed buffer field remains staged until restoration");
        request->channel->buf = request->buffer;
    }
    else
    {
        require(request->buffer != H && request->channel->buf == request->buffer &&
                    dvm_refcount_retain_u(&D2H(request->buffer)->ref) == DVM_REFCOUNT_LIVE,
                "actual independent buffer owner before field transfer");
        request->channel->buf = H;
        require(dvm_refcount_drop_nonfinal_u(&D2H(request->buffer)->ref), "old buffer field owner moves to diagnostic");
    }
    return 1;
}


/* [GC:] EN: Cause an actual busy/mismatch refusal, then end that cause before
 * returning the original result to Fast. heap/buffer/mapped/retry remain real
 * production inputs/outputs. Source count stays one, so the later count cannot
 * reconstruct the reason. Buffer ownership returns before packet rollback.
 * RU: Получить настоящий busy/mismatch-отказ и снять его причину до возврата
 * исходного результата Fast. heap/buffer/mapped/retry — входы/выходы runtime.
 * Счётчик источника остаётся один; позднее чтение не восстановит причину.
 * Владение буфером возвращается до отката пакета. */
static int probe_channel_transient(Heap *heap, Array *buffer, Type **mapped, int claimed, int *retry)
{
    ChannelTransientProbe request = {0};
    request.channel = H2D(Channel *, heap);
    request.buffer = buffer;
    pthread_mutex_lock(&sync);
    request.owner = channel_contender;
    pthread_mutex_unlock(&sync);
    require(request.owner && request.owner != dvm_native_context_current(), "actual different native match owner");
    dvm_runtime_channel_queue_guard(stage_channel_transient, &request);
    int result = __real_dvm_runtime_channel_claim_observed(heap, buffer, mapped, claimed, retry);
    request.restore = 1;
    dvm_runtime_channel_queue_guard(stage_channel_transient, &request);
    require(!result, "changed channel buffer must refuse the original snapshot");
    require(retry && *retry && dvm_refcount_load_u(&heap->ref) == 1 && !*mapped,
            "channel reservation refusal captured before the cause disappears");
    pthread_mutex_lock(&sync);
    races++;
    pthread_mutex_unlock(&sync);
    failure_seen = 1;
    return result;
}
