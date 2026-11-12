# Copyright (c) 2026 bhgv
# EN: Channel waiter publication and claimed completion.
# RU: Публикация ожидания каналов и завершение под захватом.


# EN: Finish direct and buffered transfers after dequeue and ALT cancellation.
# source/body: borrowed immutable C text; return adapted text, raise on drift.
# No runtime ownership or locks; generation runs before compilation.
# RU: Завершить передачу и пробудить снятого с очереди участника.
# source/body: неизменяемый текст C; вернуть адаптацию, при расхождении — исключение.
# Ресурсами VM не владеет; генерация выполняется до компиляции.
def adapt_wait_completion(source):
    """Finish direct and buffered transfers after dequeue and ALT cancellation."""
    anchor = "OP(isend)\n{"
    start = source.index(anchor)
    end = source.index("typedef struct CsendAlt", start)
    body = source[start:end]
    for old, peer_receives in (
        ("\tc->mover();\n\taddrun(p);", 1),
        ("\t\tc->mover();\n\t}\n\taddrun(p);", 0),
    ):
        if body.count(old) != 1:
            raise ValueError("direct channel completion drift")
        indent = "\t\t" if old.startswith("\t\t") else "\t"
        body = body.replace(
            old,
            indent
            + f"channel_move_or_wake_error(c, p, {peer_receives});"
            + old[len(indent + "c->mover();") :],
            1,
        )
    # The refill coordinator consumes the temporary channel retain and wakes
    # the sender. The direct branch still completes its own wake.
    old = (
        "\t\tok = kprotect(channelrefill, &refill);\n"
        "\t\tdestroy(c);\n\t\tif(!ok)\n\t\t\tkrethrow();"
    )
    wake = "\t\tchannel_move_or_wake_error(c, p, 0);\n\t}\n\taddrun(p);"
    if body.count(old) != 1 or body.count(wake) != 1 or body.count("\tint ok;\n") != 1:
        raise ValueError("buffered channel completion drift")
    body = body.replace("\tint ok;\n", "", 1)
    body = body.replace(
        old,
        "\t\tdvm_runtime_channel_finish_refill(c, p, channelrefill, &refill, &refill.committed);",
        1,
    )
    body = body.replace(wake, "\t\tchannel_move_or_wake_error(c, p, 0);\n\t\taddrun(p);\n\t}", 1)
    if body.count("addrun(p);") != 2:
        raise ValueError("channel wake boundary drift")
    body = body.replace("addrun(p);", "dvm_runtime_channel_wake(p, nil);")
    body = adapt_match_claims(body)
    source = source[:start] + WAIT_COMPLETION + body + source[end:]
    refill_adapters = (
        ("struct ChannelRefill\n{", "struct ChannelRefill\n{\n    int committed;"),
        (
            "static int cputb(Channel*, void*);",
            "static int cputb(Channel*, void*);\nstatic int refill_put(Channel*, void*, int*, Prog*);",
        ),
        (
            "cputb(refill->channel, refill->source);",
            "refill_put(refill->channel, refill->source, &refill->committed, refill->sender);",
        ),
        ("refill.channel = c;", "refill.committed = 0;\n        refill.channel = c;"),
    )
    for old, new in refill_adapters:
        if source.count(old) != 1:
            raise ValueError("channel refill publication boundary drift")
        source = source.replace(old, new, 1)

    anchor = "static Prog*\nchannelpeer(Channel *c, int receive)"
    end_peer = source.index("\n}", source.index(anchor)) + 2
    source = source[:end_peer] + QUEUE_PEER_READER + source[end_peer:]
    for direction in (0, 1):
        old = f"channelpeer(c, {direction})"
        if source.count(old) != 1:
            raise ValueError("channel head reader drift")
        owner_cell = "DVM_R.s" if direction else "DVM_R.d"
        source = source.replace(old, f"dvm_runtime_channel_peer(c, {direction}, {owner_cell})", 1)
    return source


QUEUE_PEER_READER = r"""

extern void dvm_runtime_channel_queue_guard(int (*operation)(void *), void *argument);

extern int dvm_runtime_channel_wait_guest(Channel *channel);
extern void dvm_runtime_channel_require_wait(int outcome);
int dvm_runtime_channel_wait_state(Channel *channel);

typedef struct ChannelPeerRead
{
    Channel *channel;
    int receive;
    int busy;
    void *owner_cell; /* Saved decoded channel operand. */
    Prog *peer; /* Borrowed under the surrounding NativeWorld turn. */
} ChannelPeerRead;


/* Inspect readiness and the head while queue mutation is excluded. */
static int read_channel_peer(void *argument)
{
    ChannelPeerRead *read = argument;
    read->busy = dvm_runtime_channel_wait_state(read->channel);
    if (!read->busy)
        read->peer = channelpeer(read->channel, read->receive);
    return 1;
}

/* Release the world only for a foreign reservation, then recheck readiness. */
static int wait_and_read_channel_peer(void *argument)
{
    ChannelPeerRead *read = argument;
    do
    {
        if (read->busy != 1)
        {
            error("channel buffer transfer already active");
        }
        dvm_runtime_channel_require_wait(dvm_runtime_channel_wait_guest(read->channel));
        {
            Channel *current;
            const char *problem = dvm_runtime_bytes_snapshot(read->owner_cell, &current, sizeof(current));
            if (problem)
            {
                error((char *)problem);
            }
            if (current != read->channel)
                error("channel operand changed while waiting");
        }
        dvm_runtime_channel_queue_guard(read_channel_peer, read);
    } while (read->busy);
    return 1;
}

/* Resolve a peer under NativeWorld. owner_cell identifies the captured
 * channel operand and must stay live through any native suspension. */
Prog *dvm_runtime_channel_peer(Channel *channel, int receive, void *owner_cell)
{
    ChannelPeerRead read = {channel, receive, 0, owner_cell, nil};
    DVM_HeapHold hold = {0};
    const char *problem;
    int completed;
    dvm_runtime_channel_queue_guard(read_channel_peer, &read);
    if (!read.busy)
        return read.peer;
    if (read.busy != 1 || !guarded_channel_selected())
        error("channel buffer transfer already active");
    problem = dvm_runtime_heap_hold(&channel, &Tchannel, 0, &hold);
    if (problem)
        error((char *)problem);
    completed = kprotect(wait_and_read_channel_peer, &read);
    dvm_runtime_heap_finish(&hold, completed);
    return read.peer;
}
"""


# EN: Place all post-preflight queue mutations inside a bounded waiter claim.
# source/body: borrowed immutable C text; return adapted text, raise on drift.
# No runtime ownership or locks; generation runs before compilation.
# RU: Перенести проверки геометрии и передачу под захват канала.
# source/body: неизменяемый текст C; вернуть адаптацию, при расхождении — исключение.
# Ресурсами VM не владеет; генерация выполняется до компиляции.
def adapt_match_claims(body):
    """Place all post-preflight queue mutations inside a bounded waiter claim."""
    for opcode, direction in (("isend", "recv"), ("irecv", "send")):
        start = body.index(f"OP({opcode})\n{{")
        end = body.index("\n}", start)
        original = body[start:end]
        dequeue = f"\tcqdel(&c->{direction});"
        offset = original.index(dequeue)
        prefix, match = original[:offset], original[offset:]
        local = "\tChannelRefill refill;\n" if opcode == "irecv" else ""
        if local:
            if prefix.count(local) != 1:
                raise ValueError("channel refill ledger drift")
            prefix = prefix.replace(local, "", 1)
        # EN: Ring geometry belongs to the channel match, not the peer lookup.
        # RU: Геометрия кольца принадлежит захвату канала, а не поиску пары.
        preflight_start = prefix.index(
            "\t/* Resolve temporary owner failures"
            if opcode == "irecv"
            else "\tif(c->buf != H && c->size > 0)"
        )
        preflight = prefix[preflight_start:]
        prefix = prefix[:preflight_start]
        if opcode == "irecv":
            if prefix.count("\tHeap *h;\n") != 1:
                raise ValueError("channel receive preflight declaration drift")
            prefix = prefix.replace("\tHeap *h;\n", "", 1)
            local += "\tHeap *h;\n"
        callback = f"""/* Complete a matched {opcode} while the caller owns the wait claim. */
static int channel_match_{opcode}(void *argument)
{{
    ChannelMatch *request = argument;
    Channel *c = request->channel;
    Prog *p = request->peer;
{local}{preflight}    if (p->state == Palt)
        dvm_runtime_alt_preflight(p->R.s);
{match}
    return 1;
}}

"""
        dispatch = f"""    request.channel = c;
    request.peer = p;
    request.queue = &c->{direction};
    if (!dvm_runtime_channel_with_queue_claim(p, validate_channel_head, channel_match_{opcode}, &request, c) &&
        !dvm_runtime_scalar_retry({"ISEND" if opcode == "isend" else "IRECV"}))
    {{
        error("channel waiter changed before transfer");
    }}"""
        prefix = prefix.replace("\tProg *p;\n", "\tProg *p;\n    ChannelMatch request;\n", 1)
        body = body[:start] + callback + prefix + dispatch + body[end:]
    return body + ALT_CHANNEL_ATTEMPT


ALT_CHANNEL_ATTEMPT = r"""
static int validate_alt_channel_head(void *argument)
{
    ChannelMatch *request = argument;
    return !dvm_runtime_channel_wait_state(request->channel) && validate_channel_head(argument);
}

/* Try the captured channel without publishing a new SEND or RECV wait. */
int dvm_runtime_alt_try_transfer(Channel *channel, void *payload, int receive)
{
    ChannelPeerRead read = {channel, receive, 0, nil, nil};
    dvm_runtime_channel_queue_guard(read_channel_peer, &read);
    if (read.busy)
        return 0;
    void *source = DVM_R.s;
    void *destination = DVM_R.d;
    void *middle = DVM_R.m;
    int done;
    if (!read.peer)
    {
        done = transfer_channel_buffer(channel, payload, receive, CHANNEL_ALT_TRY, NULL, NULL);
    }
    else
    {
        ChannelMatch request = {channel, read.peer, receive ? &channel->send : &channel->recv};
        if (receive)
            DVM_R.d = payload;
        else
            DVM_R.s = payload;
        done = dvm_runtime_channel_with_queue_claim(read.peer, validate_alt_channel_head,
            receive ? channel_match_irecv : channel_match_isend, &request, channel);
    }
    DVM_R.s = source;
    DVM_R.d = destination;
    DVM_R.m = middle;
    return done;
}

"""


WAIT_COMPLETION = r"""
extern void dvm_runtime_alt_preflight(Alt *alt);
extern int dvm_runtime_channel_with_queue_claim(Prog *peer, int (*validate)(void *),
    int (*match)(void *), void *request, Channel *channel);

typedef struct ChannelMatch
{
    Channel *channel; /* Borrowed under NativeWorld. */
    Prog *peer; /* Expected waiter, checked before dereference. */
    Progq **queue; /* Queue owner protected by the channel mutex. */
} ChannelMatch;


/* Validate the expected identity before acquiring its completion hold. */
static int validate_channel_head(void *argument)
{
    ChannelMatch *request = argument;
    return !dvm_runtime_channel_wait_state(request->channel) &&
        vmchanhead(request->queue)->prog == request->peer;
}

extern void dvm_runtime_channel_wake(Prog *peer, char *cause);
extern void dvm_runtime_channel_finish_refill(Channel *channel, Prog *peer,
    int (*refill)(void *), void *request, const int *committed);

typedef struct ChannelMove
{
    Channel *channel;
    Prog *peer;
    int peer_receives;
    int committed;
} ChannelMove;

static int share_channel_move(void *argument)
{
    ChannelMove *move = argument;
    dvm_runtime_mapped_rendezvous(DVM_R.s, DVM_R.d, move->channel->mid.t, &move->committed);
    return 1;
}

/* The completion claim pins the peer until it is woken. The channel hold
 * keeps the mover type alive across the shared publication and retirement. */
static int channel_move(void *argument)
{
    ChannelMove *move = argument;
    Channel *channel = move->channel;
    Type *type = channel->mid.t;
    DVM_HeapHold hold = {0};
    const char *problem;
    int completed;
    if (!dvm_runtime_parallel_channel_buffer || channel->mover != movtmp ||
        !type || type == H || !type->np ||
        !dvm_runtime_rooted_record(&DVM_R, move->peer_receives ? DVM_R.s : DVM_R.d, type) ||
        !dvm_runtime_rooted_record(&move->peer->R,
            move->peer_receives ? DVM_R.d : DVM_R.s, type))
    {
        channel->mover();
        return 1;
    }
    problem = dvm_runtime_heap_hold(&channel, &Tchannel, 0, &hold);
    if (problem)
        error((char *)problem);
    completed = kprotect(share_channel_move, move);
    dvm_runtime_heap_finish(&hold, completed);
    return 1;
}

static void channel_move_or_wake_error(Channel *channel, Prog *peer, int peer_receives)
{
    ChannelMove move = {channel, peer, peer_receives, 0};
    if (!kprotect(channel_move, &move))
    {
        dvm_runtime_channel_wake(peer, move.committed ? nil : "channel transfer failed");
        krethrow();
    }
}

"""
