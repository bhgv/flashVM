# Copyright (c) 2026 bhgv
# EN: Channel constructors and captured operand replay.
# RU: Конструкторы каналов и повтор захвата операндов.


import hashlib
import re

CHANNELS = (
    (
        r"Channel\*\nnewc\(Type \*t, void \(\*mover\)\(void\)\)\n\{.*?\n\}",
        "c7fc974386282afcc682a2e207059ed1ad0b3769d5968acd8ca3ffcf1390d548",
        """WORD capacity;
        if (!guarded_capacity(dvm_native_execution()->arithmetic_retry_opcode, &capacity))
        {
            return nil;
        }
        return dvm_runtime_channel_new(DVM_R.d, t, capacity, mover, 0);""",
    ),
    (
        r"OP\(newcm\)\n\{.*?\n\}",
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
        return;""",
    ),
    (
        r"OP\(newcmp\)\n\{.*?\n\}",
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
        return;""",
    ),
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
        error("channel constructor requires managed dispatch");
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

/* EN: Complete a typed constructor on its legacy path.
 * RU: Завершить типизированный конструктор по старому пути. */
typedef struct SharedChannelNew
{
    Type *type;              /* Borrowed payload descriptor. */
    void (*mover)(void);     /* Typed mover. */
    Channel *result;         /* Published channel or a scheduled retry. */
} SharedChannelNew;

static int admitted_typed_channel(void *argument)
{
    SharedChannelNew *request = argument;
    WORD capacity;
    if (guarded_capacity(dvm_native_execution()->arithmetic_retry_opcode, &capacity))
    {
        request->result = dvm_runtime_channel_new(DVM_R.d, request->type, capacity,
            request->mover, 0);
    }
    return 1;
}


/* EN: Reuse SHARED for typed channels with an empty destination.
 * RU: Использовать текущий SHARED для канала с пустой целевой ячейкой. */
static Channel *shared_typed_channel(Type *type, void (*mover)(void))
{
    WORD count = 0;
    if (DVM_R.m == DVM_R.d || dvm_runtime_record_shared_index(DVM_R.m, &count))
    {
        Channel *channel = dvm_runtime_channel_try_shared(DVM_R.d, type, count, mover);
        if (channel)
        {
            return channel;
        }
    }
    SharedChannelNew request = {type, mover, nil};
    dvm_runtime_gc_dispatch_finish();
    int completed = kprotect(admitted_typed_channel, &request);
    dvm_runtime_gc_dispatch_restore();
    if (!completed)
    {
        krethrow();
    }
    return request.result;
}

"""


# EN: Preserve legacy entry points and guard against changes to their bodies.
# source/body: borrowed immutable C text; return adapted text, raise on drift.
# No runtime ownership or locks; generation runs before compilation.
# RU: Выбрать проверенный конструктор канала.
# source/body: неизменяемый текст C; вернуть адаптацию, при расхождении — исключение.
# Ресурсами VM не владеет; генерация выполняется до компиляции.
def adapt(source):
    """Preserve legacy entry points and guard against changes to their bodies."""
    for index, (pattern, digest, dispatch) in enumerate(CHANNELS):
        match = re.search(pattern, source, re.S)
        if not match or hashlib.sha256(match.group().encode()).hexdigest() != digest:
            raise ValueError("channel constructor drift: " + pattern)
        body = match.group()
        offset = body.index("\n{") + 2
        selection = (
            "\n    if (guarded_channel_selected())\n    {\n        " + dispatch + "\n    }\n"
        )
        if index == 0:
            selection = """
    if (dvm_runtime_guarded_copies && dvm_runtime_gc_dispatch_is_shared())
    {
        return shared_typed_channel(t, mover);
    }
""" + selection
        helpers = HELPERS if index == 0 else ""
        source = (
            source[: match.start()]
            + helpers
            + body[:offset]
            + selection
            + body[offset:]
            + source[match.end() :]
        )
    return source


# EN: Capture SEND/RECV channel cells before looking up or claiming a peer.
# source/body: borrowed immutable C text; return adapted text, raise on drift.
# No runtime ownership or locks; generation runs before compilation.
# RU: Захватить операнд канала перед поиском пары.
# source/body: неизменяемый текст C; вернуть адаптацию, при расхождении — исключение.
# Ресурсами VM не владеет; генерация выполняется до компиляции.
def adapt_channel_operand_replay(source):
    """Capture SEND/RECV channel cells before looking up or claiming a peer."""
    for operation, opcode, operand in (("isend", "ISEND", "d"), ("irecv", "IRECV", "s")):
        start = source.index(f"OP({operation})")
        end = source.index("\n}", start)
        body = source[start:end]
        capture = f"\tmemmove(&c, DVM_R.{operand}, sizeof(c));"
        if body.count(capture) != 1:
            raise ValueError("channel operand capture drift: " + operation)
        body = body.replace(
            capture,
            f"""    if (guarded_channel_selected())
    {{
        if (!guarded_channel_input({opcode}, DVM_R.{operand}, &c, sizeof(c)))
        {{
            return;
        }}
    }}
    else
    {{
{capture}
    }}""",
            1,
        )
        receive = operation == "irecv"
        mover = "cgetb(c, DVM_R.d)" if receive else "cputb(c, DVM_R.s)"
        value = "DVM_R.d" if receive else "DVM_R.s"
        if body.count(mover) != 1:
            raise ValueError("direct channel buffer route drift: " + operation)
        body = body.replace(
            mover, f"transfer_channel_buffer(c, {value}, {int(receive)}, {opcode}, NULL, NULL)", 1
        )
        source = source[:start] + body + source[end:]
    return source
