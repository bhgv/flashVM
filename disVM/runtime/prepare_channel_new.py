"""Select the rooted channel constructor without modifying the vendor snapshot."""
import hashlib
import re


CHANNELS = (
    (r"Channel\*\nnewc\(Type \*t, void \(\*mover\)\(void\)\)\n\{.*?\n\}",
     "c7fc974386282afcc682a2e207059ed1ad0b3769d5968acd8ca3ffcf1390d548",
     """WORD capacity;
        if (!guarded_capacity(dvm_native_execution()->arithmetic_retry_opcode, &capacity))
        {
            return nil;
        }
        return dvm_runtime_channel_new(DVM_R.d, t, capacity, mover, 0);"""),
    (r"OP\(newcm\)\n\{.*?\n\}",
     "7e5ae37285bc157aa61d2c7c85699904f568ea0e01026f003038f19d60483329",
     """WORD size, capacity;
        if (!guarded_channel_input(INEWCM, DVM_R.s, &size, sizeof(size)))
        {
            return;
        }
        if (size < 0)
        {
            error(exNegsize);
        }
        if ((uvlong)size > 0x7fffffff)
        {
            error(exHeap);
        }
        if (!guarded_capacity(INEWCM, &capacity))
        {
            return;
        }
        dvm_runtime_raw_channel_new(DVM_R.d, capacity, size, movm);
        return;"""),
    (r"OP\(newcmp\)\n\{.*?\n\}",
     "2c512e9f036513e88149c561700a9da4db032c012504a2fe517425d935cfd9ce",
     """WORD index, capacity;
        Type *type;
        if (!guarded_channel_input(INEWCMP, DVM_R.s, &index, sizeof(index)))
        {
            return;
        }
        type = moduletype(DVM_R.M, index);
        if (!guarded_capacity(INEWCMP, &capacity))
        {
            return;
        }
        dvm_runtime_channel_new(DVM_R.d, type, capacity, movtmp, 0);
        return;"""),
)

HELPERS = r"""
/* Require the same runtime admission as other guarded constructors. */
static int guarded_channel_selected(void)
{
    int mode;
    if (!dvm_runtime_guarded_copies)
    {
        return 0;
    }
    mode = dvm_runtime_shared_mode();
    if (!mode)
    {
        error(exHeap);
    }
    return mode == 2;
}


/* Borrow the decoded operand and copy it to private output before channel
 * allocation or queue effects. Return zero only after scheduling a retry. */
static int guarded_channel_input(int opcode, const void *operand, void *output, size_t size)
{
    DVM_CellResult claim;
    const char *problem = dvm_runtime_bytes_snapshot_attempt(operand, output, size, &claim);
    if (claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(opcode))
    {
        return 0;
    }
    if (problem)
    {
        error((char *)problem);
    }
    return 1;
}


/* An absent capacity aliases the destination and does not read its contents. */
static int guarded_capacity(int opcode, WORD *capacity)
{
    if (DVM_R.m == DVM_R.d)
    {
        *capacity = 0;
        return 1;
    }
    return guarded_channel_input(opcode, DVM_R.m, capacity, sizeof(*capacity));
}

"""


def adapt(source):
    """Preserve legacy entry points and guard against changes to their bodies."""
    for index, (pattern, digest, dispatch) in enumerate(CHANNELS):
        match = re.search(pattern, source, re.S)
        if not match or hashlib.sha256(match.group().encode()).hexdigest() != digest:
            raise ValueError("channel constructor drift: " + pattern)
        body = match.group()
        offset = body.index("\n{") + 2
        selection = "\n    if (guarded_channel_selected())\n    {\n        " + dispatch + "\n    }\n"
        helpers = HELPERS if index == 0 else ""
        source = (source[:match.start()] + helpers + body[:offset] + selection
                  + body[offset:] + source[match.end():])
    return source


def adapt_channel_operand_replay(source):
    """Capture SEND/RECV channel cells before looking up or claiming a peer."""
    for operation, opcode, operand in (('isend', 'ISEND', 'd'), ('irecv', 'IRECV', 's')):
        start = source.index(f'OP({operation})')
        end = source.index('\n}', start)
        body = source[start:end]
        capture = f'\tmemmove(&c, DVM_R.{operand}, sizeof(c));'
        if body.count(capture) != 1:
            raise ValueError('channel operand capture drift: '+operation)
        body = body.replace(capture, f"""    if (guarded_channel_selected())
    {{
        if (!guarded_channel_input({opcode}, DVM_R.{operand}, &c, sizeof(c)))
        {{
            return;
        }}
    }}
    else
    {{
{capture}
    }}""", 1)
        receive = operation == 'irecv'
        mover = 'cgetb(c, DVM_R.d)' if receive else 'cputb(c, DVM_R.s)'
        value = 'DVM_R.d' if receive else 'DVM_R.s'
        if body.count(mover) != 1:
            raise ValueError('direct channel buffer route drift: '+operation)
        body = body.replace(mover, f'transfer_channel_buffer(c, {value}, {int(receive)}, {opcode})', 1)
        source = source[:start]+body+source[end:]
    return source


def adapt_wait_completion(source):
    """Finish direct and buffered transfers after dequeue and ALT cancellation."""
    anchor = "OP(isend)\n{"
    start = source.index(anchor)
    end = source.index("typedef struct CsendAlt", start)
    body = source[start:end]
    for old in ("\tc->mover();\n\taddrun(p);",
                "\t\tc->mover();\n\t}\n\taddrun(p);"):
        if body.count(old) != 1:
            raise ValueError("direct channel completion drift")
        indent = "\t\t" if old.startswith("\t\t") else "\t"
        body = body.replace(old, indent + "channel_move_or_wake_error(c, p);" +
                            old[len(indent + "c->mover();"):], 1)
    # The refill coordinator consumes the temporary channel retain and wakes
    # the sender. The direct branch still completes its own wake.
    old = ("\t\tok = kprotect(channelrefill, &refill);\n"
           "\t\tdestroy(c);\n\t\tif(!ok)\n\t\t\tkrethrow();")
    wake = "\t\tchannel_move_or_wake_error(c, p);\n\t}\n\taddrun(p);"
    if body.count(old) != 1 or body.count(wake) != 1 or body.count("\tint ok;\n") != 1:
        raise ValueError("buffered channel completion drift")
    body = body.replace("\tint ok;\n", "", 1)
    body = body.replace(old, "\t\tdvm_runtime_channel_finish_refill(c, p, channelrefill, &refill);", 1)
    body = body.replace(wake, "\t\tchannel_move_or_wake_error(c, p);\n\t\taddrun(p);\n\t}", 1)
    if body.count("addrun(p);") != 2:
        raise ValueError("channel wake boundary drift")
    body = body.replace("addrun(p);", "dvm_runtime_channel_wake(p, nil);")
    body = adapt_match_claims(body)
    source = source[:start] + WAIT_COMPLETION + body + source[end:]
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
        callback = f"""/* Complete a matched {opcode} while the caller owns the wait claim. */
static int channel_match_{opcode}(void *argument)
{{
    ChannelMatch *request = argument;
    Channel *c = request->channel;
    Prog *p = request->peer;
{local}    if (p->state == Palt)
        dvm_runtime_alt_preflight(p->R.s);
{match}
    return 1;
}}

"""
        dispatch = f"""    request.channel = c;
    request.peer = p;
    request.queue = &c->{direction};
    dvm_runtime_channel_with_queue_claim(p, validate_channel_head, channel_match_{opcode}, &request);"""
        prefix = prefix.replace("\tProg *p;\n", "\tProg *p;\n    ChannelMatch request;\n", 1)
        body = body[:start] + callback + prefix + dispatch + body[end:]
    return body


WAIT_COMPLETION = r"""
extern void dvm_runtime_alt_preflight(Alt *alt);
extern void dvm_runtime_channel_with_queue_claim(Prog *peer, void (*validate)(void *),
    int (*match)(void *), void *request);

typedef struct ChannelMatch
{
    Channel *channel; /* Borrowed under NativeWorld. */
    Prog *peer; /* Expected waiter, checked before dereference. */
    Progq **queue; /* Queue owner protected by the channel mutex. */
} ChannelMatch;


/* Validate the expected identity before acquiring its completion hold. */
static void validate_channel_head(void *argument)
{
    ChannelMatch *request = argument;
    if (vmchanhead(request->queue)->prog != request->peer)
        error(exHeap);
}

extern void dvm_runtime_channel_wake(Prog *peer, char *cause);
extern void dvm_runtime_channel_finish_refill(Channel *channel, Prog *peer,
    int (*refill)(void *), void *request);

/* Invoke the selected mover inside the current owner's error frame. */
static int channel_move(void *argument)
{
    Channel *channel = argument;
    channel->mover();
    return 1;
}


/* The peer has already left its channel queues. On transfer failure wake it
 * with an error before unwinding the initiating instruction. NativeWorld
 * keeps the borrowed Prog live across this transition. */
static void channel_move_or_wake_error(Channel *channel, Prog *peer)
{
    if (!kprotect(channel_move, channel))
    {
        dvm_runtime_channel_wake(peer, "channel transfer failed");
        krethrow();
    }
}

"""


def adapt_alt_completion(source):
    """Fail the detached waiter if publishing its selection is refused."""
    predicate = "slot == nil || slot == H || (uintptr)slot > (uintptr)-1-sizeof(WORD)"
    anchor = "static void\naltresult(void *slot)"
    old = ("\t\taltresult(selected.result);\n\t\tif(selected.array){\n"
           "\t\t\taltresult(selected.tuple);")
    if (source.count(anchor) != 1 or source.count(predicate) != 1
            or source.count(old) != 1):
        raise ValueError("ALT publication boundary drift")
    source = source.replace(predicate, "!altresultvalid(slot)", 1)
    source = source.replace(anchor, ALT_RESULT_VALID.replace("PREDICATE", predicate) + anchor, 1)
    wake = '\t\tp->kill = "alt channel hungup";\n\t\taddrun(p);'
    if source.count(wake) != 1:
        raise ValueError("ALT hangup wake boundary drift")
    source = source.replace(wake, '\t\tdvm_runtime_channel_wake(p, "alt channel hungup");', 1)
    return source.replace(old, """        /* Cancellation above has removed every registration. Only now
         * may a failed publication return the peer to the scheduler. */
        if (!altresultvalid(selected.result) ||
            (selected.array && !altresultvalid(selected.tuple)))
        {
            dvm_runtime_channel_wake(p, "channel transfer failed");
            error(exBounds);
        }
        if (selected.array) {""", 1)


ALT_RESULT_VALID = r"""
extern void dvm_runtime_channel_wake(Prog *peer, char *cause);

/* Apply one geometry check to immediate and detached selection results. */
static int altresultvalid(void *slot)
{
    return !(PREDICATE);
}


"""


def adapt_alt_reservations(source):
    """Prefer ready guards whose channel has no active buffer transfer."""
    changes = (
        ("altmark(Channel *c, Prog *p)", "altmark(Channel *c, Prog *p, int *available)"),
        ("altrdy(Alt *a, Prog *p)", "altrdy(Alt *a, Prog *p, int *available)"),
        ("added = altmark(c, p);", "added = altmark(c, p, available);"),
        ("altchoose(Alt *a, int which, AltSelection *selected)",
         "altchoose(Alt *a, int which, AltSelection *selected, int skip_reserved)"),
        ("&& which-- == 0",
         "&& (!skip_reserved || !dvm_runtime_channel_buffer_busy(c)) && which-- == 0"),
        ("int ready; // Number of ready guard alternatives.",
         "int ready; // Number of ready guard alternatives.\n"
         "    int available; // Number ready without waiting for a reservation.\n"
         "    int skip_reserved; // Immediate selection excludes active transfers."),
        ("r->ready = altrdy(r->alt, r->prog);",
         "r->available = 0;\n    r->ready = altrdy(r->alt, r->prog, &r->available);"),
        ("altchoose(r->alt, r->choice, &selected);",
         "altchoose(r->alt, r->choice, &selected, r->skip_reserved);"),
        ("altvalidate(a, nsend, nrecv, 0);",
         "altvalidate(a, nsend, nrecv, 0);\n"
         "    if (block && altwaitshared(a, nsend, nrecv))\n"
         "    {\n"
         "        altvalidate(a, nsend, nrecv, 0);\n"
         "    }"),
        ("p = currun();\n\n\ta = DVM_R.s;", "p = currun();\nretry_alt:\n\ta = DVM_R.s;"),
        ("delrun(Palt);", "delrun(Palt);\n            dvm_runtime_channel_waiter_published();"),
        ("nrdy = ready.ready;",
         "if (block && dvm_runtime_guarded_copies && !ready.available && altpending(a, nsend + nrecv))\n"
         "    {\n"
         "        altdone(a, p, nil, -1);\n"
         "        dvm_runtime_channel_require_wait(dvm_runtime_channel_wait_change());\n"
         "        goto retry_alt;\n"
         "    }\n"
         "    ready.skip_reserved = ready.available != 0 || !block;\n"
         "    nrdy = ready.skip_reserved ? ready.available : ready.ready;"),
    )
    for old, new in changes:
        expected = 3 if old == "&& which-- == 0" else 1
        if source.count(old) != expected:
            raise ValueError("ALT reservation boundary drift: " + old)
        source = source.replace(old, new)
    source, count = re.subn(
        r"(?m)^([ \t]*)nrdy\+\+;",
        r"\1nrdy++;\n\1*available += !dvm_runtime_channel_buffer_busy(c);",
        source,
    )
    if count != 3 or source.count("extern\tOP(isend);") != 1:
        raise ValueError("ALT reservation count/declaration boundary drift")
    source = source.replace(
        "extern\tOP(isend);",
        "/* NativeWorld keeps reservations stable from readiness through selection. */\n"
        "extern int dvm_runtime_channel_buffer_busy(Channel *channel);\n\nextern\tOP(isend);",
        1,
    )
    anchor = "/* Cancel this process's registrations"
    if source.count(anchor) != 1:
        raise ValueError("ALT reservation entry boundary drift")
    return source.replace(anchor, ALT_SHARED_WAIT + anchor, 1)


ALT_SHARED_WAIT = r"""
extern Prog *dvm_runtime_channel_peer(Channel *channel, int receive, void *owner_cell);
extern int dvm_runtime_channel_wait_state(Channel *channel);
extern int dvm_runtime_channel_wait_change(void);
extern void dvm_runtime_channel_require_wait(int outcome);
extern void dvm_runtime_channel_waiter_published(void);
extern int dvm_runtime_guarded_copies;

/* Classify the validated members of an array receive guard while NativeWorld
 * protects its storage. Nil Dis members are disabled guards. */
static int altarraypending(Array *array)
{
    WORD index;
    Channel *member;
    int state, pending = 0;

    for (index = 0; index < array->len; index++)
    {
        memmove(&member, array->data + (uintptr)index*sizeof(member), sizeof(member));
        if (member != H)
        {
            state = dvm_runtime_channel_wait_state(member);
            if (state < 0)
            {
                return state;
            }
            pending |= state;
        }
    }
    return pending;
}


/* Scan validated guards before suspension. A mover may not park behind
 * another reservation while it owns one itself. No array storage or member
 * pointer is used after leaving NativeWorld; ALT revalidates on retry. */
static int altpending(Alt *alt, int count)
{
    uchar *cell = (uchar *)alt + offsetof(Alt, ac) + offsetof(Altc, c);
    Channel *channel;
    int index, state, pending = 0;

    for (index = 0; index < count; index++)
    {
        memmove(&channel, cell + (uintptr)index*sizeof(Altc), sizeof(channel));
        state = altobject(channel) ? altarraypending((Array *)channel) :
            dvm_runtime_channel_wait_state(channel);
        if (state < 0)
        {
            return 0;
        }
        pending |= state;
    }
    return pending;
}


/* Match the validated concrete guard cells against the captured channel.
 * The caller holds NativeWorld and supplies the unchanged guard count. */
static int altguardsmatch(uchar *first, int count, Channel *channel)
{
    int index;
    Channel *candidate;

    for (index = 1; index < count; index++)
    {
        memmove(&candidate, first + (uintptr)index*sizeof(Altc), sizeof(candidate));
        if (candidate != channel)
        {
            return 0;
        }
    }
    return 1;
}


/* A same-direction ALT on one concrete channel has no independent branch to
 * service while its buffer is reserved. Wait before registering any guard.
 * The peer reader roots the channel and rechecks the first guard cell.
 * Return one after waiting, so the caller revalidates the queue geometry. */
static int altwaitshared(Alt *alt, int nsend, int nrecv)
{
    uchar *first = (uchar *)alt + offsetof(Alt, ac) + offsetof(Altc, c);
    int count = nsend + nrecv;
    int sends, receives;
    Channel *channel;

    if (!count || (nsend && nrecv))
    {
        return 0;
    }
    memmove(&channel, first, sizeof(channel));
    if (altobject(channel) || !dvm_runtime_channel_buffer_busy(channel) ||
        !altguardsmatch(first, count, channel))
    {
        return 0;
    }
    dvm_runtime_channel_peer(channel, nrecv != 0, first);
    if (!altheader(alt, &sends, &receives) || sends != nsend || receives != nrecv)
    {
        error(exBounds);
    }
    if (!altguardsmatch(first, count, channel))
    {
        error("channel operand changed while waiting");
    }
    return 1;
}


"""


def adapt_alt_preflight(source):
    """Reuse guard traversal with cancellation-specific queue checks."""
    changes = (
        ("altvalidate(Alt *a, int nsend, int nrecv)",
         "altvalidate(Alt *a, int nsend, int nrecv, int cancelling)"),
        ("altvalidate(a, nsend, nrecv);", "altvalidate(a, nsend, nrecv, 0);"),
        ("\t\treceive = guard >= nsend;\n\t\tif(!altobject(c)){",
         "\t\treceive = guard >= nsend;\n"
         "\t\tif(cancelling && (c == nil || c == H)) continue;\n"
         "\t\tif(!altobject(c)){"),
        ("altchannel(c, receive);", "altcheckguard(c, receive, cancelling);"),
        ("if(member != H)\n\t\t\t\taltchannel(member, 1);",
         "if(member != H && (!cancelling || member != nil))\n"
         "\t\t\t\taltcheckguard(member, 1, cancelling);"),
    )
    for old, new in changes:
        if source.count(old) != 1:
            raise ValueError("ALT preflight boundary drift: " + old)
        source = source.replace(old, new, 1)
    anchor = "/* Check every guard/member before"
    if source.count(anchor) != 1:
        raise ValueError("ALT traversal declaration drift")
    source = source.replace(anchor, ALT_GUARD_CHECK + anchor, 1)
    anchor = "void\naltgone(Prog *p)"
    if source.count(anchor) != 1:
        raise ValueError("ALT cancellation entry drift")
    source = source.replace(anchor, "void\ndvm_runtime_altgone_claimed(Prog *p)", 1)
    return source + ALT_PREFLIGHT + ALT_EXTERNAL_CANCEL


ALT_GUARD_CHECK = r"""
/* Cancellation checks only the affected queue. A finalized queue may already
 * be detached. Registration still checks both queues and buffer readiness. */
static void altcheckguard(Channel *channel, int receive, int cancelling)
{
    Progq **queue;
    if (!cancelling)
    {
        altchannel(channel, receive);
        return;
    }
    if (altobject(channel))
        error(exHeap);
    queue = receive ? &channel->recv : &channel->send;
    if (*queue)
        vmchanend(queue);
}


"""

ALT_PREFLIGHT = r"""

/* Run before the initiating opcode changes any queue. NativeWorld protects
 * the checked chains through subsequent cancellation without callbacks. */
void dvm_runtime_alt_preflight(Alt *alt)
{
    int sends, receives;
    if (!altheader(alt, &sends, &receives))
        error(exBounds);
    altvalidate(alt, sends, receives, 1);
}

/* Check the closing chain and all ALT siblings before any waiter is detached.
 * The caller keeps NativeWorld until detachment and completion. */
Progq *dvm_runtime_channel_close_preflight(Progq **queue)
{
    Progq *entry;
    Progq *tail = vmchanend(queue);
    for (entry = *queue; entry; entry = entry->next)
    {
        if (entry->prog && entry->prog->state == Palt)
            dvm_runtime_alt_preflight(entry->prog->R.s);
    }
    return tail;
}

"""



def adapt_channel_finalizer(source):
    """Keep the vendor finalizer for standalone builds."""
    old = ("\tif(c->mover == movtmp)\n\t\tfreetype(c->mid.t);\n"
           "\tkillcomm(&c->send);\n\tkillcomm(&c->recv);\n"
           "\tif (!swept && c->buf != H)\n\t\tdestroy(c->buf);")
    if source.count(old) != 1:
        raise ValueError("channel finalizer boundary drift")
    return source.replace(old, "\textern void dvm_runtime_channel_finalize(Channel *, int);\n"
                          "\tdvm_runtime_channel_finalize(c, swept);", 1)


def adapt_queue_holds(source):
    """Give every published hosted queue node one native Prog hold."""
    start = source.index("void\ncqadd(Progq **q, Prog *p)")
    end = source.index("/* Resolve a checked channel/peer", start)
    body = source[start:end]
    changes = (
        ("\t\ttail->prog = p;", """        if (!dvm_runqueue_pin(p))
            error(exHeap);
        tail->prog = p;"""),
        ("\tn->prog = p;", """    if (!dvm_runqueue_pin(p))
    {
        free(n);
        error(exHeap);
    }
    n->prog = p;"""),
        ("\tProgq *head, *next;", "\tProgq *head, *next;\n    Prog *peer;"),
        ("\thead = vmchanhead(q);", "\thead = vmchanhead(q);\n    peer = head->prog;"),
        ("\t\thead->prog = nil;", """        head->prog = nil;
        if (peer)
            dvm_runqueue_unpin(peer);"""),
        ("\tfree(head);", "\tfree(head);\n    dvm_runqueue_unpin(peer);"),
        ("\t\t\treturn;\n\t\t}\n\t\tf = *q;", """            dvm_runqueue_unpin(p);
            return;
        }
        f = *q;"""),
        ("\t\tfree(f);", "\t\tfree(f);\n        dvm_runqueue_unpin(p);"),
    )
    for old, new in changes:
        if body.count(old) != 1:
            raise ValueError("channel queue ownership drift: " + old)
        body = body.replace(old, new, 1)
    declarations = ("extern int dvm_runqueue_pin(Prog *process);\n"
                    "extern void dvm_runqueue_unpin(Prog *process);\n\n")
    for name, parameters in (("cqadd", "Progq **q, Prog *p"),
                             ("cqdel", "Progq **q"),
                             ("cqdelp", "Progq **q, Prog *p")):
        signature = f"void\n{name}({parameters})"
        if body.count(signature) != 1:
            raise ValueError("channel queue wrapper drift: " + name)
        body = body.replace(signature, f"static void\n{name}_locked({parameters})", 1)
    tail = source[end:]
    for state in ("Psend", "Precv"):
        anchor = f"p = delrun({state});"
        if tail.count(anchor) != 1:
            raise ValueError("channel waiter publication drift: " + state)
        tail = tail.replace(anchor, anchor + "\n        dvm_runtime_channel_waiter_published();", 1)
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

/* Publish a queue node with its native hold. */
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


def adapt_buffer_reservation(source):
    """Protect ring geometry and reservation while keeping movers unlocked."""
    start = source.index("static int\ncgetb(Channel *c, void *v)\n{")
    end = source.index("/* Reject payload aliases", start)
    body = source[start:end]
    for required in ("dvm_runtime_heap_retain(&h->ref);", "c->front = buffer.front;",
                     "c->size = buffer.size;", "kprotect(channelread, &read);",
                     "DVM_R.d = buffer.slot;", "c->mover();"):
        if required not in body:
            raise ValueError("channel buffer reservation drift: " + required)
    return source[:start] + BUFFER_RESERVATION + source[end:]


BUFFER_RESERVATION = r"""
extern void dvm_runtime_channel_queue_guard(int (*operation)(void *), void *argument);

extern void dvm_runtime_channel_buffer_notify(void);
#include "disvm/native_context.h"

typedef struct ChannelReservation
{
    ChannelRead read; /* Receive retains the array through its mover. */
    int receive;
    int available;
    int publish_after_move;
    int next_size;
    int next_front;
    Type *transaction_type; /* Validated layout for a guarded built-in transfer. */
    int consumed;
    int completed;
    struct ChannelReservation *next;
    DVM_NativeContext *owner;
    int replay_opcode; /* Only direct buffered SEND or RECV may replay. */
    int deferred; /* BUSY before value or ring metadata changed. */
} ChannelReservation;


/* Stack ledgers are linked only while their operations own a reservation.
 * The queue mutex protects membership. No mover runs with the mutex held. */
static ChannelReservation *active_channel_reservations;

int dvm_runtime_channel_buffer_busy(Channel *channel)
{
    ChannelReservation *entry;
    for (entry = active_channel_reservations; entry; entry = entry->next)
    {
        if (entry->read.channel == channel)
            return 1;
    }
    return 0;
}


/* With the queue mutex or NativeWorld held, classify a reservation.
 * A mover must never park behind another mover. */
int dvm_runtime_channel_wait_state(Channel *channel)
{
    ChannelReservation *entry;
    DVM_NativeContext *current = dvm_native_context_current();
    int busy = 0;
    int owns_reservation = 0;
    for (entry = active_channel_reservations; entry; entry = entry->next)
    {
        busy |= entry->read.channel == channel;
        owns_reservation |= entry->owner == current;
    }
    return busy ? (owns_reservation ? -1 : 1) : 0;
}


/* An unoccupied mapped slot must own no pointers. That makes the guarded
 * send transaction incapable of raising from displaced-owner retirement. */
static int channel_slot_is_empty(ChannelBuffer *buffer)
{
    int byte, bit;
    for (byte = 0; byte < buffer->type->np; byte++)
    {
        for (bit = 0; bit < 8; bit++)
        {
            void *value;
            if (!(buffer->type->map[byte] & (0x80U >> bit)))
                continue;
            memmove(&value, (uchar *)buffer->slot + (byte*8+bit)*sizeof(void *),
                sizeof(value));
            if (value != H)
                return 0;
        }
    }
    return 1;
}


/* Capture the next state. Pointer-free transfers publish it after the copy.
 * NativeWorld still excludes another reservation. */
static int reserve_channel_buffer(void *argument)
{
    ChannelReservation *reservation = argument;
    ChannelRead *read = &reservation->read;
    Channel *channel = read->channel;
    ChannelBuffer buffer;
    int status;
    if (dvm_runtime_channel_buffer_busy(channel))
        error("channel buffer transfer already active");
    status = channelbuffer(channel, reservation->receive, 0, &buffer);
    if (status < 0)
        error(buffer.error);
    if (!status)
        return 1;
    if (buffer.width && !channelspan(read->destination, buffer.width))
        error(read->destination == nil || read->destination == H ? exNilref : exBounds);
    if (buffer.type->np &&
        channel->mover == movtmp && dvm_runtime_guarded_copies)
    {
        if (channel->mid.t != buffer.type ||
            (!reservation->receive && !channel_slot_is_empty(&buffer)))
            error(exHeap);
        reservation->transaction_type = buffer.type;
    }
    else if (reservation->replay_opcode && dvm_runtime_guarded_copies && !buffer.type->np &&
        ((channel->mover == movb && buffer.width == sizeof(BYTE)) ||
         (channel->mover == movw && buffer.width == sizeof(WORD)) ||
         (channel->mover == movl && buffer.width == sizeof(LONG)) ||
         (channel->mover == movf && buffer.width == sizeof(REAL)) ||
         (channel->mover == movm && channel->mid.w == buffer.width)))
    {
        reservation->transaction_type = buffer.type;
    }
    if (reservation->receive)
    {
        Heap *header = D2H(buffer.array);
        ulong refs = dvm_refcount_load_u(&header->ref);
        if (!refs || refs == (ulong)-1)
            error(exHeap);
        dvm_runtime_heap_retain(&header->ref);
        read->array = buffer.array;
    }
    read->slot = buffer.slot;
    reservation->next_size = buffer.size;
    reservation->next_front = buffer.front;
    reservation->publish_after_move = !buffer.type->np || reservation->transaction_type != nil;
    if (!reservation->publish_after_move)
    {
        if (reservation->receive)
            channel->front = buffer.front;
        channel->size = buffer.size;
    }
    reservation->owner = dvm_native_context_current();
    reservation->next = active_channel_reservations;
    active_channel_reservations = reservation;
    reservation->available = 1;
    return 1;
}


/* Publish a committed transfer and remove its reservation on every exit. */
static int finish_channel_buffer(void *argument)
{
    ChannelReservation *reservation = argument;
    Channel *channel = reservation->read.channel;
    ChannelReservation **link = &active_channel_reservations;
    while (*link && *link != reservation)
        link = &(*link)->next;
    if (!*link)
        abort();
    if (reservation->consumed || (reservation->completed && !reservation->deferred && reservation->publish_after_move))
    {
        if (reservation->receive)
            channel->front = reservation->next_front;
        channel->size = reservation->next_size;
    }
    *link = reservation->next;
    dvm_runtime_channel_buffer_notify();
    return 1;
}


/* Move a built-in value out of the ring and clear any mapped source owners. */
static int consume_channel_buffer(void *argument)
{
    ChannelReservation *reservation = argument;
    reservation->deferred = !dvm_runtime_mapped_buffer_transfer(reservation->read.slot,
        reservation->read.destination, reservation->transaction_type,
        reservation->transaction_type->np ? &reservation->consumed : NULL,
        reservation->replay_opcode);
    return 1;
}


/* Keep the receive array live through the mover and its mapped cleanup. */
static int read_buffer_owned(Channel *channel, void *destination, int replay_opcode)
{
    ChannelReservation reservation = {{channel, nil, nil, destination}, 1, 0, 0, 0, 0, nil, 0, 0, nil, nil, replay_opcode, 0};
    dvm_runtime_channel_queue_guard(reserve_channel_buffer, &reservation);
    if (!reservation.available)
        return 0;
    reservation.completed = reservation.transaction_type ? kprotect(consume_channel_buffer, &reservation) :
        kprotect(channelread, &reservation.read);
    dvm_runtime_channel_queue_guard(finish_channel_buffer, &reservation);
    destroy(reservation.read.array);
    if (!reservation.completed)
        krethrow();
    return 1;
}


/* Invoke the mover with the captured geometry outside the queue mutex. */
static int write_channel_buffer(void *argument)
{
    ChannelReservation *reservation = argument;
    ChannelRead *read = &reservation->read;
    Channel *channel = read->channel;
    DVM_R.s = read->destination;
    DVM_R.m = &channel->mid;
    DVM_R.d = read->slot;
    if (reservation->transaction_type)
    {
        reservation->deferred = !dvm_runtime_mapped_buffer_transfer(read->destination,
            read->slot, reservation->transaction_type, NULL, reservation->replay_opcode);
    }
    else
    {
        channel->mover();
    }
    return 1;
}


/* Capture send geometry under the mutex and transfer outside it. */
static int write_buffer_owned(Channel *channel, void *source, int replay_opcode)
{
    ChannelReservation reservation = {{channel, nil, nil, source}, 0, 0, 0, 0, 0, nil, 0, 0, nil, nil, replay_opcode, 0};
    dvm_runtime_channel_queue_guard(reserve_channel_buffer, &reservation);
    if (!reservation.available)
        return 0;
    reservation.completed = kprotect(write_channel_buffer, &reservation);
    dvm_runtime_channel_queue_guard(finish_channel_buffer, &reservation);
    if (!reservation.completed)
        krethrow();
    return 1;
}



typedef struct RootedBufferTransfer
{
    Channel *channel;
    void *operand;
    int receive;
    int transferred;
    int replay_opcode; /* Zero for refill after claiming a peer. */
} RootedBufferTransfer;


/* Report availability separately from the VM checkpoint result. */
static int run_rooted_buffer_transfer(void *argument)
{
    RootedBufferTransfer *transfer = argument;
    transfer->transferred = transfer->receive ?
        read_buffer_owned(transfer->channel, transfer->operand, transfer->replay_opcode) :
        write_buffer_owned(transfer->channel, transfer->operand, transfer->replay_opcode);
    return 1;
}


/* Root the channel outside the queue mutex. Its owning buffer and mover
 * descriptor then survive callbacks through reservation completion. */
static int transfer_channel_buffer(Channel *channel, void *operand, int receive, int replay_opcode)
{
    RootedBufferTransfer transfer = {channel, operand, receive, 0, replay_opcode};
    DVM_HeapHold owner = {0};
    const char *problem;
    int completed;
    if (!guarded_channel_selected())
    {
        run_rooted_buffer_transfer(&transfer);
        return transfer.transferred;
    }
    problem = dvm_runtime_heap_hold(&channel, &Tchannel, 0, &owner);
    if (problem)
        error((char *)problem);
    completed = kprotect(run_rooted_buffer_transfer, &transfer);
    dvm_runtime_heap_finish(&owner, completed);
    return transfer.transferred;
}


static int cgetb(Channel *channel, void *destination)
{
    return transfer_channel_buffer(channel, destination, 1, 0);
}

static int cputb(Channel *channel, void *source)
{
    return transfer_channel_buffer(channel, source, 0, 0);
}

"""
