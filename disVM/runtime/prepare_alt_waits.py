# Copyright (c) 2026 bhgv
# EN: ALT reservation waits and cancellation preflight.
# RU: Ожидание резерваций ALT и проверки отмены.
import re
from prepare_channel_queues import ALT_EXTERNAL_CANCEL


# EN: Prefer ready guards whose channel has no active buffer transfer.
# source/body: borrowed immutable C text; return adapted text, raise on drift.
# No runtime ownership or locks; generation runs before compilation.
# RU: Учесть активные резервации при выборе ALT.
# source/body: неизменяемый текст C; вернуть адаптацию, при расхождении — исключение.
# Ресурсами VM не владеет; генерация выполняется до компиляции.
def adapt_alt_reservations(source):
    """Prefer ready guards whose channel has no active buffer transfer."""
    changes = (
        ("if(!kprotect(altready, &ready)){", "if(!kprotect(altready_guarded, &ready)){"),
        ("cqadd(&c->recv, p);", "dvm_runtime_alt_register_locked(&c->recv, p);"),
        ("cqadd(&c->send, p);", "dvm_runtime_alt_register_locked(&c->send, p);"),
        ("altmark(Channel *c, Prog *p)", "altmark(Channel *c, Prog *p, int *available)"),
        ("altrdy(Alt *a, Prog *p)", "altrdy(Alt *a, Prog *p, int *available)"),
        ("added = altmark(c, p);", "added = altmark(c, p, available);"),
        (
            "altchoose(Alt *a, int which, AltSelection *selected)",
            "altchoose(Alt *a, int which, AltSelection *selected, int skip_reserved)",
        ),
        (
            "&& which-- == 0",
            "&& (!skip_reserved || !dvm_runtime_channel_buffer_busy(c)) && which-- == 0",
        ),
        (
            "int ready; // Number of ready guard alternatives.",
            "int ready; // Number of ready guard alternatives.\n"
            "    int available; // Number ready without waiting for a reservation.\n"
            "    int skip_reserved; // Immediate selection excludes active transfers.",
        ),
        (
            "r->ready = altrdy(r->alt, r->prog);",
            "r->available = 0;\n    r->ready = altrdy(r->alt, r->prog, &r->available);",
        ),
        (
            "int registered; // Cleanup still owns the guard table until cancellation completes.",
            "int registered; // Cleanup still owns the guard table until cancellation completes.\n"
            "    AltSelection selected;\n"
            "    int transferred;",
        ),
        (
            "static int\naltcommunicate(void *arg)",
            """static int altselect_locked(void *arg)
{
    AltReady *ready = arg;
    altchoose(ready->alt, ready->choice, &ready->selected, ready->skip_reserved);
    return 1;
}

static int
altcommunicate(void *arg)""",
        ),
        (
            "AltSelection selected;\n\n\tr = arg;\n\taltchoose(r->alt, r->choice, &selected);\n"
            "\tif(selected.found){",
            "\n\tr = arg;\n"
            "    dvm_runtime_channel_queue_guard(altselect_locked, r);\n"
            "\tif(r->selected.found){",
        ),
        ("alttransfer(&selected);", "r->transferred = alttransfer(&r->selected);"),
        (
            "if(ready.registered)\n\t\taltdone(a, p, nil, -1);",
            "if(ready.registered){\n"
            "        altdone(a, p, nil, -1);\n"
            "    }\n"
            "    if (!ready.transferred) goto retry_alt;",
        ),
        (
            "altvalidate(a, nsend, nrecv, 0);",
            "altvalidate_guarded(a, nsend, nrecv);\n"
            "    if (block && altwaitshared(a, nsend, nrecv))\n"
            "    {\n"
            "        altvalidate_guarded(a, nsend, nrecv);\n"
            "    }",
        ),
        ("static ulong xrand = 0x20342;", "static _Thread_local ulong xrand = 0x20342;"),
        ("p = currun();\n\n\ta = DVM_R.s;", "p = currun();\nretry_alt:\n\ta = DVM_R.s;"),
        (
            "delrun(Palt);\n\t\t\tp->R.s = DVM_R.s;\n\t\t\tp->R.d = DVM_R.d;\n"
            "\t\t\tDVM_R.IC = 1;\n\t\t\tDVM_R.t = 1;",
            "AltPark park = {p, generation, 0};\n"
            "            dvm_runtime_channel_queue_guard(publish_alt_park, &park);\n"
            "            if (!park.published) {\n"
            "                altdone(a, p, nil, -1);\n"
            "                goto retry_alt;\n"
            "            }\n"
            "            dvm_runtime_channel_waiter_published();",
        ),
        (
            "ready.alt = a;",
            "uint64_t generation = block && dvm_runtime_guarded_copies ? dvm_runtime_channel_snapshot() : 0;\n"
            "    ready.transferred = 0;\n"
            "    ready.alt = a;",
        ),
        (
            "nrdy = ready.ready;",
            "if (block && dvm_runtime_guarded_copies && !ready.available && altpending(a, nsend + nrecv))\n"
            "    {\n"
            "        altdone(a, p, nil, -1);\n"
            "        dvm_runtime_channel_require_wait(dvm_runtime_channel_wait_change(generation));\n"
            "        goto retry_alt;\n"
            "    }\n"
            "    ready.skip_reserved = ready.available != 0 || !block;\n"
            "    nrdy = ready.skip_reserved ? ready.available : ready.ready;",
        ),
    )
    for old, new in changes:
        expected = {"&& which-- == 0": 3, "cqadd(&c->recv, p);": 2}.get(old, 1)
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
    anchor = "static int\naltready(void *arg)"
    if source.count(anchor) != 1:
        raise ValueError("ALT readiness callback boundary drift")
    source = source.replace(
        anchor,
        """static int altready(void *arg);

static int altready_guarded(void *arg)
{
    dvm_runtime_channel_queue_guard(altready, arg);
    return 1;
}

""" + anchor,
        1,
    )
    anchor = "/* Cancel this process's registrations"
    if source.count(anchor) != 1:
        raise ValueError("ALT reservation entry boundary drift")
    return source.replace(anchor, ALT_SHARED_WAIT + anchor, 1)


ALT_SHARED_WAIT = r"""
#include <stdint.h>
extern Prog *dvm_runtime_channel_peer(Channel *channel, int receive, void *owner_cell);
extern int dvm_runtime_channel_wait_state(Channel *channel);
extern uint64_t dvm_runtime_channel_snapshot(void);
extern int dvm_runtime_channel_changed_locked(uint64_t generation);
extern int dvm_runtime_channel_wait_change(uint64_t generation);
extern void dvm_runtime_channel_require_wait(int outcome);
extern void dvm_runtime_channel_waiter_published(void);
extern int dvm_runtime_guarded_copies;

typedef struct AltValidation
{
    Alt *alt;
    int sends;
    int receives;
} AltValidation;

static int altvalidate_locked(void *argument)
{
    AltValidation *check = argument;
    altvalidate(check->alt, check->sends, check->receives, 0);
    return 1;
}

static void altvalidate_guarded(Alt *alt, int sends, int receives)
{
    AltValidation check = {alt, sends, receives};
    dvm_runtime_channel_queue_guard(altvalidate_locked, &check);
}



/* EN: Publish ALT operands before the queue mutex exposes the parked state.
 * RU: Опубликовать операнды ALT до открытия состояния ожидания под мьютексом. */
typedef struct AltPark
{
    Prog *process;
    uint64_t generation;
    int published;
} AltPark;

static int publish_alt_park(void *argument)
{
    AltPark *park = argument;
    Prog *process = park->process;
    if (dvm_runtime_guarded_copies && dvm_runtime_channel_changed_locked(park->generation))
        return 1;
    process->R.s = DVM_R.s;
    process->R.d = DVM_R.d;
    DVM_R.IC = 1;
    DVM_R.t = 1;
    delrun(Palt);
    park->published = 1;
    return 1;
}


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
static int altpending_locked(Alt *alt, int count)
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


/* EN: Borrowed guard scan; all reservation lists require the queue mutex.
 * RU: Заимствованный обход guards; списки резерваций требуют мьютекса очереди. */
typedef struct AltPending
{
    Alt *alt; /* Borrowed live guard table. */
    int count; /* Previously validated guard count. */
    int pending; /* Scan result published before unlocking. */
} AltPending;


/* EN: Scan request while the caller holds the channel queue mutex.
 * RU: Обойти request при удерживаемом мьютексе очереди каналов. */
static int scan_alt_pending(void *argument)
{
    AltPending *request = argument;
    request->pending = altpending_locked(request->alt, request->count);
    return 1;
}


/* EN: Protect reservation membership while scanning borrowed guards.
 * RU: Защитить список резерваций при обходе заимствованных guards. */
static int altpending(Alt *alt, int count)
{
    AltPending request = {alt, count, 0};
    dvm_runtime_channel_queue_guard(scan_alt_pending, &request);
    return request.pending;
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
 * EN: The peer reader tests reservation membership under the queue mutex;
 * there is no unlocked busy-list probe. Revalidate after that observation.
 * RU: Список резерваций проверяет peer reader под мьютексом очереди;
 * чтения списка без блокировки нет. После проверки повторить валидацию. */
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
    if (altobject(channel) || !altguardsmatch(first, count, channel))
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


# EN: Reuse guard traversal with cancellation-specific queue checks.
# source/body: borrowed immutable C text; return adapted text, raise on drift.
# No runtime ownership or locks; generation runs before compilation.
# RU: Проверить регистрации ALT до отмены.
# source/body: неизменяемый текст C; вернуть адаптацию, при расхождении — исключение.
# Ресурсами VM не владеет; генерация выполняется до компиляции.
def adapt_alt_preflight(source):
    """Reuse guard traversal with cancellation-specific queue checks."""
    changes = (
        (
            "altvalidate(Alt *a, int nsend, int nrecv)",
            "altvalidate(Alt *a, int nsend, int nrecv, int cancelling)",
        ),
        ("altvalidate(a, nsend, nrecv);", "altvalidate(a, nsend, nrecv, 0);"),
        (
            "\t\treceive = guard >= nsend;\n\t\tif(!altobject(c)){",
            "\t\treceive = guard >= nsend;\n"
            "\t\tif(cancelling && (c == nil || c == H)) continue;\n"
            "\t\tif(!altobject(c)){",
        ),
        ("altchannel(c, receive);", "altcheckguard(c, receive, cancelling);"),
        (
            "if(member != H)\n\t\t\t\taltchannel(member, 1);",
            "if(member != H && (!cancelling || member != nil))\n"
            "\t\t\t\taltcheckguard(member, 1, cancelling);",
        ),
    )
    for old, new in changes:
        if source.count(old) != 1:
            raise ValueError("ALT preflight boundary drift: " + old)
        source = source.replace(old, new, 1)
    for condition, queue in (
        ("c != H && c != nil && c->recv != nil && c->recv->prog", "recv"),
        ("c->send != nil && c->send->prog", "send"),
        ("c->recv && c->recv->prog", "recv"),
    ):
        old = f"if({condition})"
        start = source.index(old)
        end = source.index(f"cqdelp(&c->{queue}, p);", start) + len(f"cqdelp(&c->{queue}, p);")
        remove = f"dvm_runtime_channel_unmark(&c->{queue}, p);"
        if condition.startswith("c != H"):
            remove = "if(c != H && c != nil)\n\t\t\t" + remove
        source = source[:start] + remove + source[end:]
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
extern void dvm_runtime_channel_queue_guard(int (*operation)(void *), void *argument);
extern void dvm_runtime_channel_unmark(Progq **queue, Prog *peer);
extern void dvm_runtime_alt_register_locked(Progq **queue, Prog *peer);
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

/* Validate queue links while the channel queue mutex is held. */
static int alt_preflight_locked(void *argument)
{
    Alt *alt = argument;
    int sends, receives;
    if (!altheader(alt, &sends, &receives))
        error(exBounds);
    altvalidate(alt, sends, receives, 1);
    return 1;
}

void dvm_runtime_alt_preflight(Alt *alt)
{
    dvm_runtime_channel_queue_guard(alt_preflight_locked, alt);
}

/* Check the closing chain and all ALT siblings before any waiter is detached.
 * The caller holds the channel queue mutex through detachment. */
Progq *dvm_runtime_channel_close_preflight(Progq **queue)
{
    Progq *entry;
    Progq *tail = vmchanend(queue);
    for (entry = *queue; entry; entry = entry->next)
    {
        if (entry->prog && entry->prog->state == Palt)
            alt_preflight_locked(entry->prog->R.s);
    }
    return tail;
}

"""
