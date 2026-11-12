# Copyright (c) 2026 bhgv
# EN: Channel queue ownership and cancellation adapters.
# RU: Владение очередями каналов и адаптеры отмены.


# EN: Keep the vendor finalizer for standalone builds.
# source/body: borrowed immutable C text; return adapted text, raise on drift.
# No runtime ownership or locks; generation runs before compilation.
# RU: Подключить учёт уничтожения канала.
# source/body: неизменяемый текст C; вернуть адаптацию, при расхождении — исключение.
# Ресурсами VM не владеет; генерация выполняется до компиляции.
def adapt_channel_finalizer(source):
    """Keep the vendor finalizer for standalone builds."""
    old = (
        "\tif(c->mover == movtmp)\n\t\tfreetype(c->mid.t);\n"
        "\tkillcomm(&c->send);\n\tkillcomm(&c->recv);\n"
        "\tif (!swept && c->buf != H)\n\t\tdestroy(c->buf);"
    )
    if source.count(old) != 1:
        raise ValueError("channel finalizer boundary drift")
    return source.replace(
        old,
        "\textern void dvm_runtime_channel_finalize(Channel *, int);\n"
        "\tdvm_runtime_channel_finalize(c, swept);",
        1,
    )


# EN: Give every published hosted queue node one native Prog hold.
# source/body: borrowed immutable C text; return adapted text, raise on drift.
# No runtime ownership or locks; generation runs before compilation.
# RU: Закрепить процессы на время нахождения в очередях.
# source/body: неизменяемый текст C; вернуть адаптацию, при расхождении — исключение.
# Ресурсами VM не владеет; генерация выполняется до компиляции.
def adapt_queue_holds(source):
    """Give every published hosted queue node one native Prog hold."""
    start = source.index("void\ncqadd(Progq **q, Prog *p)")
    end = source.index("/* Resolve a checked channel/peer", start)
    body = source[start:end]
    changes = (
        (
            "\t\ttail->prog = p;",
            """        if (!dvm_runqueue_pin(p))
            error(exHeap);
        tail->prog = p;""",
        ),
        (
            "\tn->prog = p;",
            """    if (!dvm_runqueue_pin(p))
    {
        free(n);
        error(exHeap);
    }
    n->prog = p;""",
        ),
        ("\tProgq *head, *next;", "\tProgq *head, *next;\n    Prog *peer;"),
        ("\thead = vmchanhead(q);", "\thead = vmchanhead(q);\n    peer = head->prog;"),
        (
            "\t\thead->prog = nil;",
            """        head->prog = nil;
        if (peer)
            dvm_runqueue_unpin(peer);""",
        ),
        ("\tfree(head);", "\tfree(head);\n    dvm_runqueue_unpin(peer);"),
        (
            "\t\t\treturn;\n\t\t}\n\t\tf = *q;",
            """            dvm_runqueue_unpin(p);
            return;
        }
        f = *q;""",
        ),
        ("\t\tfree(f);", "\t\tfree(f);\n        dvm_runqueue_unpin(p);"),
    )
    for old, new in changes:
        if body.count(old) != 1:
            raise ValueError("channel queue ownership drift: " + old)
        body = body.replace(old, new, 1)
    declarations = (
        "extern int dvm_runqueue_pin(Prog *process);\n"
        "extern void dvm_runqueue_unpin(Prog *process);\n\n"
    )
    for name, parameters in (
        ("cqadd", "Progq **q, Prog *p"),
        ("cqdel", "Progq **q"),
        ("cqdelp", "Progq **q, Prog *p"),
    ):
        signature = f"void\n{name}({parameters})"
        if body.count(signature) != 1:
            raise ValueError("channel queue wrapper drift: " + name)
        body = body.replace(signature, f"static void\n{name}_locked({parameters})", 1)
    tail = source[end:]
    for state, queue, cell in (("Psend", "send", "s"), ("Precv", "recv", "d")):
        old = f"\t\tp = currun();\n\t\tcqadd(&c->{queue}, p);"
        start_wait = tail.index(old)
        end_wait = tail.index("DVM_R.t = 1;", start_wait) + len("DVM_R.t = 1;")
        tail = (
            tail[:start_wait]
            + f"        park_channel_waiter(c, &c->{queue}, DVM_R.{cell}, {state});"
            + tail[end_wait:]
        )
    return source[:start] + declarations + body + QUEUE_WRAPPERS + tail


ALT_EXTERNAL_CANCEL = r"""

extern void dvm_runtime_channel_with_claim(Prog *peer, int (*match)(void *), void *request);

/* External cancellation keeps every registration intact on preflight failure. */
static int cancel_claimed_alt(void *argument)
{
    Prog *peer = argument;
    dvm_runtime_alt_preflight(peer->R.s);
    dvm_runtime_altgone_claimed(peer);
    return 1;
}


/* Acquire the same completion claim as SEND and RECV. The scheduler owns
 * the input Prog and NativeWorld protects the queues through cancellation. */
void altgone(Prog *peer)
{
    if (peer->state == Palt)
    {
        dvm_runtime_channel_with_claim(peer, cancel_claimed_alt, peer);
    }
}
"""


QUEUE_WRAPPERS = r"""
extern void dvm_runtime_channel_queue_guard(int (*operation)(void *), void *argument);

typedef struct ChannelQueueRequest
{
    Progq **queue; /* Live queue owner cell. */
    Prog *peer; /* Live input for registration or cancellation. */
} ChannelQueueRequest;


extern void dvm_runtime_channel_waiter_published(void);

typedef struct ChannelPark
{
    Channel *channel; /* Borrowed live channel. */
    Progq **queue;    /* Queue belonging to channel. */
    void *slot;      /* Borrowed operand in the waiting frame. */
    int state;       /* Psend or Precv. */
    int published;   /* Queue and waiter state committed. */
} ChannelPark;


/* EN: Publish the queue node and parked state under the same mutex.
 * RU: Опубликовать узел очереди и состояние ожидания под одним мьютексом. */
static int publish_channel_park(void *argument)
{
    ChannelPark *request = argument;
    Prog *peer = currun();
    if (dvm_runtime_guest_unbounded)
    {
        int receive = request->state == Precv;
        if (dvm_runtime_channel_wait_state(request->channel) ||
            vmchanready(request->channel, receive))
        {
            if (!dvm_runtime_scalar_retry(receive ? IRECV : ISEND))
            {
                error("channel readiness changed repeatedly before parking");
            }
            return 1;
        }
    }
    cqadd_locked(request->queue, peer);
    peer->ptr = request->slot;
    peer->chan = request->channel;
    DVM_R.IC = 1;
    DVM_R.t = 1;
    delrun(request->state);
    request->published = 1;
    return 1;
}


/* EN: Commit this thread's wait and operand before a peer can claim it.
 * RU: Опубликовать ожидание и операнд до захвата другим потоком. */
static void park_channel_waiter(Channel *channel, Progq **queue, void *slot, int state)
{
    ChannelPark request = {channel, queue, slot, state, 0};
    dvm_runtime_channel_queue_guard(publish_channel_park, &request);
    if (request.published)
    {
        dvm_runtime_channel_waiter_published();
    }
}


/* Publish a queue node with its native hold. */
void dvm_runtime_alt_register_locked(Progq **queue, Prog *peer)
{
    cqadd_locked(queue, peer);
}

static int queue_add(void *argument)
{
    ChannelQueueRequest *request = argument;
    cqadd_locked(request->queue, request->peer);
    return 1;
}

static int queue_delete(void *argument)
{
    cqdel_locked(argument);
    return 1;
}

static int queue_delete_peer(void *argument)
{
    ChannelQueueRequest *request = argument;
    cqdelp_locked(request->queue, request->peer);
    return 1;
}


/* ALT may already be detached. Check the head under the same guard used
 * to remove its registration so a concurrent close cannot free it first. */
static int queue_unmark(void *argument)
{
    ChannelQueueRequest *request = argument;
    if (*request->queue && (*request->queue)->prog)
        cqdelp_locked(request->queue, request->peer);
    return 1;
}

void dvm_runtime_channel_unmark(Progq **queue, Prog *peer)
{
    ChannelQueueRequest request = {queue, peer};
    dvm_runtime_channel_queue_guard(queue_unmark, &request);
}

void cqadd(Progq **queue, Prog *peer)
{
    ChannelQueueRequest request = {queue, peer};
    dvm_runtime_channel_queue_guard(queue_add, &request);
}

void cqdel(Progq **queue)
{
    dvm_runtime_channel_queue_guard(queue_delete, queue);
}

void cqdelp(Progq **queue, Prog *peer)
{
    ChannelQueueRequest request = {queue, peer};
    dvm_runtime_channel_queue_guard(queue_delete_peer, &request);
}


"""
