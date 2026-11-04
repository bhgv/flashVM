"""Select the rooted channel constructor without modifying the vendor snapshot."""
import hashlib
import re


CHANNELS = (
    (r"Channel\*\nnewc\(Type \*t, void \(\*mover\)\(void\)\)\n\{.*?\n\}",
     "c7fc974386282afcc682a2e207059ed1ad0b3769d5968acd8ca3ffcf1390d548",
     "return dvm_runtime_channel_new(DVM_R.d, t, guarded_capacity(), mover, 0);"),
    (r"OP\(newcm\)\n\{.*?\n\}",
     "7e5ae37285bc157aa61d2c7c85699904f568ea0e01026f003038f19d60483329",
     """WORD size = guarded_channel_word(DVM_R.s);
        if (size < 0)
        {
            error(exNegsize);
        }
        if ((uvlong)size > 0x7fffffff)
        {
            error(exHeap);
        }
        dvm_runtime_raw_channel_new(DVM_R.d, guarded_capacity(), size, movm);
        return;"""),
    (r"OP\(newcmp\)\n\{.*?\n\}",
     "2c512e9f036513e88149c561700a9da4db032c012504a2fe517425d935cfd9ce",
     """WORD index = guarded_channel_word(DVM_R.s);
        Type *type = moduletype(DVM_R.M, index);
        dvm_runtime_channel_new(DVM_R.d, type, guarded_capacity(), movtmp, 0);
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


/* Snapshot native WORD operands before an allocation can call a monitor. */
static WORD guarded_channel_word(void *operand)
{
    WORD value;
    const char *problem = dvm_runtime_bytes_copy(operand, &value, sizeof(value));
    if (problem)
    {
        error((char *)problem);
    }
    return value;
}


/* Dis encodes an absent capacity by aliasing the destination operand. */
static WORD guarded_capacity(void)
{
    return DVM_R.m == DVM_R.d ? 0 : guarded_channel_word(DVM_R.m);
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
    return source[:start] + WAIT_COMPLETION + body + source[end:]


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
    channel_match_head(&c->{direction}, &request, channel_match_{opcode});"""
        prefix = prefix.replace("\tProg *p;\n", "\tProg *p;\n    ChannelMatch request;\n", 1)
        body = body[:start] + callback + prefix + dispatch + body[end:]
    return body


WAIT_COMPLETION = r"""
extern void dvm_runtime_alt_preflight(Alt *alt);
extern void dvm_runtime_channel_with_claim(Prog *peer, int (*match)(void *), void *request);

typedef struct ChannelMatch
{
    Channel *channel; /* Borrowed under NativeWorld. */
    Prog *peer; /* Live waiter held by the current scheduler turn. */
} ChannelMatch;
/* Revalidate the queue head and take the completion hold while the same
 * NativeWorld turn still protects the queue's own Prog reference. */
static void channel_match_head(Progq **queue, ChannelMatch *request,
    int (*match)(void *))
{
    if (vmchanhead(queue)->prog != request->peer)
        error(exHeap);
    dvm_runtime_channel_with_claim(request->peer, match, request);
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
    return source[:start] + declarations + body + source[end:]



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
