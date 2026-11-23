# Copyright (c) 2026 bhgv
# EN: Generate proxy ALT dispatch, including protected mixed guard probes.
# RU: Генерация диспетчера ALT с прокси и защищённой проверкой смешанных веток.
REMOTE_ALT_DISPATCH = r"""
#ifdef DVM_TERMINAL_LAUNCH
extern void dvm_runtime_mixed_alt_probe_end(void);
extern int dvm_runtime_mixed_alt_probe_owned(void);
extern uint64_t dvm_runtime_mixed_alt_deadline(void);

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
    int complete; /* One branch or the nonblocking fallback was selected. */
    unsigned count; /* Number of flattened, non-nil channel guards. */
    unsigned proxies; /* Number of captured proxy guards. */
    DVM_HeapHold current; /* Owned top-level channel or array during capture. */
    DVM_HeapHold member; /* Owned array member during capture. */
    DVM_HeapHold owners[DVM_CHANNEL_BUS_SLOTS]; /* Owned guards until publication. */
    DVM_ProxyGuard guards[DVM_CHANNEL_BUS_SLOTS]; /* Channel operations borrowed from owners. */
    RemoteAltResult results[DVM_CHANNEL_BUS_SLOTS]; /* Ordinal-to-branch correspondence. */
} RemoteAlt;


/* EN: Probe captured guards once in randomized order. A retry probes local
 * guards before releasing its permit, then considers remote guards.
 * Return count when no branch matched.
 * RU: Проверить захваченные ветки в случайном порядке. При повторе проверить
 * локальные ветки до снятия допуска, затем удалённые. Если ни одна не готова,
 * вернуть count. */
static unsigned mixed_alt(DVM_ProxyGuard *guards, unsigned count, int block)
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
    int local_first = block && dvm_runtime_mixed_alt_probe_owned();
    if (local_first)
    {
        for (unsigned i = 0; i < count; i++)
        {
            DVM_ProxyGuard *guard = &guards[order[i]];
            if (!dvm_call_channel_is_proxy(guard->channel) &&
                dvm_runtime_alt_try_transfer(guard->channel, guard->operand, guard->receive))
            {
                dvm_runtime_mixed_alt_probe_end();
                return guard->index;
            }
        }
        dvm_runtime_mixed_alt_probe_end();
        DVM_ProxyGuard offers[DVM_CHANNEL_BUS_SLOTS];
        unsigned offered = 0;
        for (unsigned i = 0; i < count; i++)
        {
            DVM_ProxyGuard *guard = &guards[order[i]];
            if (dvm_call_channel_is_proxy(guard->channel))
            {
                offers[offered++] = *guard;
            }
        }
        return dvm_call_channel_alt_window(offers, offered, count);
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
    int receiving = branch >= state->sends;
    for (unsigned i = 0; i < index; i++)
    {
        if (state->guards[i].channel == channel && state->guards[i].receive != receiving)
        {
            error(exAlt);
        }
    }
    DVM_HeapHold retained = *hold;
    *hold = (DVM_HeapHold){0};
    state->owners[index] = retained;
    state->guards[index] = (DVM_ProxyGuard){.channel = channel, .operand = operand,
        .index = index, .receive = receiving};
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


/* [GC:] EN: Capture borrowed guards on the owning resident, attempt selection
 * and publish the actual Dis branch and optional member index. An unmatched
 * blocking mixed ALT leaves these cells untouched for a subsequent local wait.
 * RU: Захватить ветки на их исполнителе, выбрать одну и опубликовать ветку Dis
 * с индексом элемента. Если ни одна ветка смешанного блокирующего ALT не
 * готова, ячейки сохраняются до следующего локального ожидания. */
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
    unsigned selected;
    if (state->proxies == state->count)
    {
        dvm_runtime_mixed_alt_probe_end();
        selected = dvm_call_channel_alt(state->guards, state->count, state->block);
    }
    else
    {
        selected = mixed_alt(state->guards, state->count, state->block);
    }
    if (state->block && selected == state->count)
    {
        return 1;
    }
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
    state->complete = 1;
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


/* [GC:] EN: Attempt selection and release every capture on all paths. Return
 * one when result received a branch, zero when a mixed ALT must wait locally.
 * RU: Попробовать выбрать ветку и освободить все ссылки. Вернуть один, если
 * ветка записана в result, ноль, если смешанный ALT должен ждать локально. */
static int remote_alt(Alt *alt, int sends, int receives, int block, void *result)
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
    return state.complete;
}
#endif


"""


# EN: Exclude proxy operands from standard queue registration and matching.
# source is borrowed generated C. Return rewritten text, raising on generator drift.
# RU: Исключить прокси из обычной регистрации в очередях и сопоставления.
# source — заимствованный текст C. Вернуть изменённый текст либо ошибку расхождения.
def adapt_proxy_registration(source):
    start = source.index("altmark(Channel *c,")
    mark_end = source.index("typedef struct AltSelection", start)
    end = source.index("/* Transfer only captured operands", mark_end)
    mark, body = source[start:mark_end], source[mark_end:end]
    old_member = "if(c != H && c != nil) {"
    if mark.count(old_member) != 1:
        raise ValueError("mixed ALT array queue boundary drift")
    mark = mark.replace(old_member, "if(c != H && c != nil && !alt_is_proxy(c)) {")
    changes = (
        ("\t\tt = D2H(c)->t;\n\t\tif(t == &Tarray){\n\t\t\tadded",
         "\t\tt = D2H(c)->t;\n\t\tif(t == &Tchannel && alt_is_proxy(c)) continue;\n"
         "\t\tif(t == &Tarray){\n\t\t\tadded", 1),
        ("\t\tif(altreceives(c, p)){", "\t\tif(alt_is_proxy(c)) continue;\n\t\tif(altreceives(c, p)){", 1),
        ("&& which-- == 0", "&& !alt_is_proxy(c) && which-- == 0", 3),
    )
    for old, new, count in changes:
        if body.count(old) != count:
            raise ValueError("mixed ALT queue boundary drift: " + old)
        body = body.replace(old, new)
    return source[:start] + mark + body + source[end:]
