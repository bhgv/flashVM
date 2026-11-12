# Copyright (c) 2026 bhgv
# EN: ALT captured selections and guarded results.
# RU: Захваченные варианты ALT и защищённые результаты.


# EN: Fail the detached waiter if publishing its selection is refused.
# source/body: borrowed immutable C text; return adapted text, raise on drift.
# No runtime ownership or locks; generation runs before compilation.
# RU: Обработать отказ публикации результата ALT.
# source/body: неизменяемый текст C; вернуть адаптацию, при расхождении — исключение.
# Ресурсами VM не владеет; генерация выполняется до компиляции.
def adapt_alt_completion(source):
    """Fail the detached waiter if publishing its selection is refused."""
    predicate = "slot == nil || slot == H || (uintptr)slot > (uintptr)-1-sizeof(WORD)"
    anchor = "static void\naltresult(void *slot)"
    old = (
        "\t\taltresult(selected.result);\n\t\tif(selected.array){\n"
        "\t\t\taltresult(selected.tuple);"
    )
    if source.count(anchor) != 1 or source.count(predicate) != 1 or source.count(old) != 1:
        raise ValueError("ALT publication boundary drift")
    source = source.replace(predicate, "!altresultvalid(slot)", 1)
    source = source.replace(anchor, ALT_RESULT_VALID.replace("PREDICATE", predicate) + anchor, 1)
    wake = '\t\tp->kill = "alt channel hungup";\n\t\taddrun(p);'
    if source.count(wake) != 1:
        raise ValueError("ALT hangup wake boundary drift")
    source = source.replace(wake, '\t\tdvm_runtime_channel_wake(p, "alt channel hungup");', 1)
    return source.replace(
        old,
        """        /* Cancellation above has removed every registration. Only now
         * may a failed publication return the peer to the scheduler. */
        if (!altresultvalid(selected.result) ||
            (selected.array && !altresultvalid(selected.tuple)))
        {
            dvm_runtime_channel_wake(p, "channel transfer failed");
            error(exBounds);
        }
        if (selected.array) {""",
        1,
    )


ALT_RESULT_VALID = r"""
extern void dvm_runtime_channel_wake(Prog *peer, char *cause);

/* Apply one geometry check to immediate and detached selection results. */
static int altresultvalid(void *slot)
{
    return !(PREDICATE);
}


"""


# EN: Publish the chosen guard and optional member under one range claim.
# source/body: borrowed immutable C text; return adapted text, raise on drift.
# No runtime ownership or locks; generation runs before compilation.
# RU: Защитить публикацию результата ALT.
# source/body: неизменяемый текст C; вернуть адаптацию, при расхождении — исключение.
# Ресурсами VM не владеет; генерация выполняется до компиляции.
def adapt_alt_result_guards(source):
    """Publish the chosen guard and optional member under one range claim."""
    start = source.index("/* Store a VM selection word")
    end = source.index("/* Capture a possibly", start)
    source = source[:start] + ALT_GUARDED_RESULT + source[end:]
    matched = (
        "\t\taltstore(selected.result, selected.guard);\n"
        "\t\tif(selected.array)\n"
        "\t\t\tmemmove(selected.tuple, &selected.member, sizeof(selected.member));"
    )
    replacements = (
        (
            matched,
            """        const char *problem = altpublish(selected.result, selected.guard,
            selected.array ? selected.tuple : NULL, selected.array ? selected.member : 0);
        if (problem)
        {
            dvm_runtime_channel_wake(p, "channel transfer failed");
            error((char *)problem);
        }""",
        ),
        (
            "\t\taltstore(result, nsend+nrecv);",
            """        const char *problem = altpublish(result, nsend+nrecv, NULL, 0);
        if (problem)
        {
            error((char *)problem);
        }""",
        ),
    )
    start = source.index("static void\nalttransfer(")
    end = source.index("\n}", start) + 2
    source = source[:start] + ALT_TRY_TRANSFER + source[end:]
    for old, new in replacements:
        if source.count(old) != 1:
            raise ValueError("ALT guarded result drift: " + old)
        source = source.replace(old, new, 1)
    return source.replace(
        '#include "interp.h"',
        '#include "interp.h"\n#include "managed_owner.h"\n#include "shared_bytes.h"',
        1,
    )


# EN: Capture both header words before registering a blocking or nonblocking ALT.
# source/body: borrowed immutable C text; return adapted text, raise on drift.
# No runtime ownership or locks; generation runs before compilation.
# RU: Повторить захват заголовка ALT при конфликте.
# source/body: неизменяемый текст C; вернуть адаптацию, при расхождении — исключение.
# Ресурсами VM не владеет; генерация выполняется до компиляции.
def adapt_alt_header_replay(source):
    """Capture both header words before registering a blocking or nonblocking ALT."""
    start = source.index("static int\naltheader(")
    end = source.index("\n}", start) + 2
    body = source[start:end]
    reads = (
        "\tmemmove(&sends, (uchar*)a+offsetof(Alt, nsend), sizeof(sends));\n"
        "\tmemmove(&receives, (uchar*)a+offsetof(Alt, nrecv), sizeof(receives));"
    )
    before, after = body.split(reads)
    validate = after[:-2]  # body ends at the closing newline and brace
    body = (
        """/* Validate counts against the original borrowed guard table address. */
static int altcounts(Alt *a, WORD sends, WORD receives, int *nsend, int *nrecv)
{
    uvlong bytes;"""
        + validate
        + """
}

"""
        + before
        + reads
        + """
    return altcounts(a, sends, receives, nsend, nrecv);
}"""
    )
    body = body.replace("\tuvlong bytes;\n\tWORD sends, receives;", "\tWORD sends, receives;", 1)
    source = source[:start] + body + source[end:]
    start = source.index("void\nxecalt(")
    end = source.index("\n}", start)
    body = source[start:end]
    anchor = "\tif(!altheader(a, &nsend, &nrecv))\n\t\terror(exBounds);"
    if body.count(anchor) != 1:
        raise ValueError("ALT header replay boundary drift")
    replacement = (
        """    if (dvm_runtime_guarded_copies)
    {
        WORD sends, receives;
        DVM_CellResult claim;
        const char *problem;
        if ((uintptr)a > (uintptr)-1 - offsetof(Alt, ac))
        {
            error(exBounds);
        }
        problem = dvm_runtime_bytes_snapshot_pair_attempt((uchar*)a+offsetof(Alt, nsend), sizeof(sends),
            &sends, (uchar*)a+offsetof(Alt, nrecv), sizeof(receives), &receives, &claim);
        if (claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(block ? IALT : INBALT))
        {
            return;
        }
        if (problem)
        {
            error((char *)problem);
        }
        if (!altcounts(a, sends, receives, &nsend, &nrecv))
        {
            error(exBounds);
        }
    }
    else
    {
"""
        + anchor
        + """
    }"""
    )
    return (
        '#include "shared_access.h"\n'
        + source[:start]
        + body.replace(anchor, replacement, 1)
        + source[end:]
    )


# EN: Capture chosen guard operands and array members before detaching queues.
# source/body: borrowed immutable C text; return adapted text, raise on drift.
# No runtime ownership or locks; generation runs before compilation.
# RU: Сохранить выбранный вариант ALT.
# source/body: неизменяемый текст C; вернуть адаптацию, при расхождении — исключение.
# Ресурсами VM не владеет; генерация выполняется до компиляции.
def adapt_alt_selection_capture(source):
    """Capture chosen guard operands and array members before detaching queues."""
    start = source.index("static void\naltchoose(")
    end = source.index("\n}", start)
    body = source[start:end]
    read = "\t\tmemmove(&c, (uchar*)ac+offsetof(Altc, c), sizeof(c));"
    if body.count(read) != 2:
        raise ValueError("ALT selection operand drift")
    body = body.replace(read, "        altcapture(ac, &c, &ptr);")
    for old, new, count in (
        (
            "memmove(&selected->payload, (uchar*)ac+offsetof(Altc, ptr), sizeof(selected->payload));",
            "selected->payload = ptr;",
            2,
        ),
        ("\t\t\t\t\tmemmove(&ptr, (uchar*)ac+offsetof(Altc, ptr), sizeof(ptr));\n", "", 1),
        (
            "memmove(&c, (uchar*)ca+an*sizeof(c), sizeof(c));",
            "altcapturemember((uchar*)ca+an*sizeof(c), &c);",
            1,
        ),
    ):
        if body.count(old) != count:
            raise ValueError("ALT selected payload drift")
        body = body.replace(old, new)
    return source[:start] + ALT_SELECTION_CAPTURE + body + source[end:]


ALT_TRY_TRANSFER = r"""
extern int dvm_runtime_alt_try_transfer(Channel *channel, void *payload, int receive);

/* A stale selection has no channel or result side effects. */
static int
alttransfer(AltSelection *selected)
{
    if (!dvm_runtime_alt_try_transfer(selected->channel, selected->payload, selected->receive))
        return 0;
    const char *problem = altpublish(selected->result, selected->guard,
        selected->array ? selected->tuple : NULL, selected->member);
    if (problem)
        error((char *)problem);
    return 1;
}
"""


ALT_SELECTION_CAPTURE = r"""
/* Capture a borrowed guard into caller-private outputs. Selection already owns
 * registrations, so refusal raises for the surrounding ALT cleanup. */
static void altcapture(Altc *entry, Channel **channel, void **payload)
{
    const void *channel_cell = (uchar*)entry+offsetof(Altc, c);
    const void *payload_cell = (uchar*)entry+offsetof(Altc, ptr);
    if (dvm_runtime_guarded_copies)
    {
        DVM_CellResult claim;
        const char *problem = dvm_runtime_bytes_snapshot_pair_attempt(channel_cell, sizeof(*channel),
            channel, payload_cell, sizeof(*payload), payload, &claim);
        if (problem)
        {
            error((char *)problem);
        }
    }
    else
    {
        memmove(channel, channel_cell, sizeof(*channel));
        memmove(payload, payload_cell, sizeof(*payload));
    }
}


/* Read an array member selected from a validated, live backing array. */
static void altcapturemember(const void *cell, Channel **channel)
{
    if (dvm_runtime_guarded_copies)
    {
        const char *problem = dvm_runtime_bytes_snapshot(cell, channel, sizeof(*channel));
        if (problem)
        {
            error((char *)problem);
        }
    }
    else
    {
        memmove(channel, cell, sizeof(*channel));
    }
}


"""


ALT_GUARDED_RESULT = r"""
/* Write caller-owned selection cells in their original order. A missing tuple
 * selects a single-word result. Return a static diagnostic before any write on
 * refusal. NativeWorld still protects the waiter and captured operands. */
static const char *altpublish(void *slot, WORD guard, void *tuple, WORD member)
{
    DVM_RangeAccess access = {0};
    if (!altresultvalid(slot) || (tuple && !altresultvalid(tuple)))
    {
        return exBounds;
    }
    if (dvm_runtime_guarded_copies &&
        dvm_managed_ranges_begin(tuple, tuple ? sizeof(member) : 0,
            slot, sizeof(guard), &access) != DVM_CELL_ACQUIRED)
    {
        return exHeap;
    }
    memmove(slot, &guard, sizeof(guard));
    if (tuple)
    {
        memmove(tuple, &member, sizeof(member));
    }
    if (dvm_runtime_guarded_copies && !dvm_range_access_end(&access))
    {
        abort();
    }
    return NULL;
}


"""
