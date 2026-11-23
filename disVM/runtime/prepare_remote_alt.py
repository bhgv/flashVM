# Copyright (c) 2026 bhgv
# EN: Generate guarded proxy ALT dispatch and a mixed nonblocking probe.
# RU: Генерация прокси-диспетчера ALT и неблокирующей проверки смешанных веток.
REMOTE_ALT_DISPATCH = r"""
#ifdef DVM_TERMINAL_LAUNCH
/* EN: Identify the Dis branch and optional array member for one wire ordinal.
 * RU: Определить ветку Dis и элемент массива по номеру сетевого варианта. */
typedef struct RemoteAltResult
{
    WORD branch; /* Original Dis branch. */
    WORD member; /* Array index, when tuple is present. */
    void *tuple; /* Borrowed mapped destination for the member index. */
} RemoteAltResult;


/* [GC:] EN: Capture ALT operands and keep an owner-local pin per channel.
 * RU: Захватить операнды ALT и удерживать ссылку своего исполнителя на канал. */
typedef struct RemoteAlt
{
    Alt *alt; /* Borrowed Dis guard table. */
    void *result; /* Borrowed Dis branch result slot. */
    int sends; /* Number of leading send branches. */
    int branches; /* Total number of Dis branches. */
    int block; /* True when the instruction has no default branch. */
    unsigned count; /* Number of flattened, non-nil channel guards. */
    unsigned proxies; /* Number of captured proxy guards. */
    DVM_HeapHold current; /* Owned top-level channel or array during capture. */
    DVM_HeapHold member; /* Owned array member during capture. */
    DVM_HeapHold owners[DVM_CHANNEL_BUS_SLOTS]; /* Owned guards until publication. */
    DVM_ProxyGuard guards[DVM_CHANNEL_BUS_SLOTS]; /* Channel operations borrowed from owners. */
    RemoteAltResult results[DVM_CHANNEL_BUS_SLOTS]; /* Ordinal-to-branch correspondence. */
} RemoteAlt;


/* EN: Probe captured guards once in randomized order. Return count for default.
 * RU: Проверить захваченные ветки в случайном порядке. Для default вернуть count. */
static unsigned mixed_alt(DVM_ProxyGuard *guards, unsigned count)
{
    static _Thread_local uint32_t random = 0x20342U; /* Per-resident ordering state. */
    unsigned order[DVM_CHANNEL_BUS_SLOTS];
    for (unsigned i = 0; i < count; i++)
    {
        order[i] = i;
    }
    for (unsigned i = count; i > 1; i--)
    {
        random = random * 1103515245U + 12345U;
        unsigned chosen = (random >> 8) % i;
        unsigned temporary = order[i - 1];
        order[i - 1] = order[chosen];
        order[chosen] = temporary;
    }
    for (unsigned i = 0; i < count; i++)
    {
        DVM_ProxyGuard *guard = &guards[order[i]];
        if (dvm_call_channel_is_proxy(guard->channel))
        {
            DVM_ProxyGuard probe = *guard;
            probe.index = 0;
            if (dvm_call_channel_alt(&probe, 1, 0) == 0)
            {
                return guard->index;
            }
        }
        else if (dvm_runtime_alt_try_transfer(guard->channel, guard->operand, guard->receive))
        {
            return guard->index;
        }
    }
    return count;
}


/* [GC:] EN: Consume one captured channel in hold. Keep the operand and tuple
 * borrowed from the active Dis frame. Reject an oversized flattened request.
 * RU: Принять владение захваченным каналом hold. Операнд и кортеж заимствуются
 * из активного кадра Dis. Отклонить запрос при превышении числа вариантов. */
static void remote_alt_append(RemoteAlt *state, DVM_HeapHold *hold,
    int branch, void *operand, void *tuple, WORD member)
{
    unsigned index = state->count;
    if (index == DVM_CHANNEL_BUS_SLOTS)
    {
        error("remote alt requires at most eight eligible channel guards");
    }
    Channel *channel = hold->value;
    DVM_HeapHold retained = *hold;
    *hold = (DVM_HeapHold){0};
    state->owners[index] = retained;
    state->guards[index] = (DVM_ProxyGuard){.channel = channel, .operand = operand,
        .index = index, .receive = branch >= state->sends};
    state->results[index] = (RemoteAltResult){.branch = branch, .member = member, .tuple = tuple};
    state->proxies += dvm_call_channel_is_proxy(channel);
    state->count++;
}


/* [GC:] EN: Snapshot non-nil members from the rooted receive array. Each
 * member keeps its original array index and writes after the tuple index.
 * RU: Захватить отличные от nil элементы удерживаемого массива приёма.
 * Каждый сохраняет исходный индекс и записывает значение после индекса кортежа. */
static void remote_alt_array(RemoteAlt *state, Array *array, int branch, void *tuple)
{
    if (branch < state->sends || !altarrayvalid(array) || !altresultvalid(tuple))
    {
        error(exBounds);
    }
    void *operand = (uchar *)tuple + sizeof(WORD);
    for (WORD member = 0; member < array->len; member++)
    {
        const void *cell = array->data + (uintptr)member * sizeof(Channel *);
        const char *problem = dvm_runtime_heap_hold(cell, &Tchannel, 1, &state->member);
        if (problem)
        {
            error((char *)problem);
        }
        if (state->member.value == H)
        {
            dvm_runtime_heap_finish(&state->member, 1);
            continue;
        }
        remote_alt_append(state, &state->member, branch, operand, tuple, member);
    }
}


/* [GC:] EN: Capture the borrowed Dis guard table under managed cell guards.
 * Submit one proxy transaction or probe a mixed default ALT, then publish the
 * Dis branch and optional member index. Runs on the resident owning this ALT.
 * RU: Захватить заимствованную таблицу Dis под защитой ячеек. Отправить одну
 * операцию с прокси либо проверить смешанный ALT с default и опубликовать
 * ветку Dis с индексом элемента. Работает на исполнителе, владеющем ALT. */
static int remote_alt_select(void *argument)
{
    RemoteAlt *state = argument;
    for (int branch = 0; branch < state->branches; branch++)
    {
        Altc *entry = (Altc *)((uchar *)state->alt + offsetof(Alt, ac)) + branch;
        void *operand;
        const char *problem = dvm_runtime_bytes_snapshot((uchar *)entry + offsetof(Altc, ptr),
            &operand, sizeof(operand));
        if (!problem)
        {
            problem = dvm_runtime_heap_hold((uchar *)entry + offsetof(Altc, c), NULL, 0, &state->current);
        }
        if (problem)
        {
            error((char *)problem);
        }
        void *object = state->current.value;
        if (D2H(object)->t == &Tarray)
        {
            remote_alt_array(state, object, branch, operand);
            dvm_runtime_heap_finish(&state->current, 1);
        }
        else if (D2H(object)->t == &Tchannel)
        {
            remote_alt_append(state, &state->current, branch, operand, NULL, 0);
        }
        else
        {
            error(exHeap);
        }
    }
    if (!state->proxies)
    {
        error("remote alt guards changed during capture");
    }
    if (state->block && state->proxies != state->count)
    {
        error("mixed remote alt requires a fallback");
    }
    unsigned selected = state->proxies == state->count ?
        dvm_call_channel_alt(state->guards, state->count, state->block) :
        mixed_alt(state->guards, state->count);
    RemoteAltResult result = {.branch = state->branches};
    if (selected < state->count)
    {
        result = state->results[selected];
    }
    const char *problem = altpublish(state->result, result.branch, result.tuple, result.member);
    if (problem)
    {
        error((char *)problem);
    }
    return 1;
}


/* [GC:] EN: Release one optional hold on the resident under kprotect.
 * RU: Освободить одну необязательную ссылку на её исполнителе под kprotect. */
static int remote_alt_release(void *argument)
{
    DVM_HeapHold *hold = argument;
    if (hold->roots)
    {
        dvm_runtime_heap_finish(hold, 1);
    }
    return 1;
}


/* [GC:] EN: Execute a detected remote ALT and release every capture on success
 * or exception. The borrowed result receives the original Dis branch number.
 * RU: Выполнить удалённый ALT и освободить все ссылки при успехе и исключении.
 * В заимствованный результат записывается исходный номер ветки Dis. */
static void remote_alt(Alt *alt, int sends, int receives, int block, void *result)
{
    RemoteAlt state = {.alt = alt, .result = result, .sends = sends,
        .branches = sends + receives, .block = block};
    char first_error[ERRMAX];
    int failure = !kprotect(remote_alt_select, &state);
    if (failure)
    {
        kgerrstr(first_error, sizeof(first_error));
    }
    DVM_HeapHold *holds[DVM_CHANNEL_BUS_SLOTS + 2] = {&state.current, &state.member};
    for (unsigned i = 0; i < state.count; i++)
    {
        holds[i + 2] = &state.owners[i];
    }
    for (unsigned i = 0; i < state.count + 2; i++)
    {
        if (!kprotect(remote_alt_release, holds[i]) && !failure)
        {
            kgerrstr(first_error, sizeof(first_error));
            failure = 1;
        }
    }
    if (failure)
    {
        error(first_error);
    }
}
#endif


"""
